#include "hexfile.h"
#include "crc.h"

#define APPLICATION_START 0x9D000000
#define PA_TO_VFA(x)    (x-APPLICATION_START)
#define PA_TO_KVA0(x)   (x|0x80000000)

#define DATA_RECORD             0
#define END_OF_FILE_RECORD      1
#define EXT_SEG_ADRS_RECORD 2
#define EXT_LIN_ADRS_RECORD 4


HexFile::HexFile() :
    HexFilePtr(NULL),
    HexTotalLines(0),
    HexCurrLineNo(0)
{

}

HexFile::~HexFile()
{
    // If hex file is open close it.
    if(HexFilePtr)
    {
        fclose(HexFilePtr);
    }
}

bool HexFile::ResetHexFilePointer()
{
    if(HexFilePtr == NULL)
    {
        return false;
    }
    else
    {
        fseek(HexFilePtr, 0, 0);
        HexCurrLineNo = 0;
        return true;
    }
}

bool HexFile::LoadHexFile(QString fileName)
{
    char HexRec[255];
    // Open file
    HexFilePtr = fopen(fileName.toUtf8(), "r");
    if(HexFilePtr == NULL)
    {
        // Failed to open hex file.
        return false;
    }
    else
    {
        HexTotalLines = 0;
        while(!feof(HexFilePtr))
        {
            fgets(HexRec, sizeof(HexRec), HexFilePtr);
            HexTotalLines++;
        }
    }
    return true;
}

unsigned short HexFile::GetNextHexRecord(char *HexRec, unsigned int BuffLen)
{
    char Ascii[256];
    unsigned short len = 0;

    if(!feof(HexFilePtr))
    {
        fgets(Ascii, BuffLen * 2 + 2, HexFilePtr);

        if(Ascii[0] != ':')
        {
            // Not a valid hex record.
            return 0;
        }
        // Convert rest to hex.
        len = ConvertAsciiToHex((void *)&Ascii[1], (void *)HexRec);

        HexCurrLineNo++;

    }
    return len;
}

unsigned short HexFile::ConvertAsciiToHex(void *VdAscii, void *VdHexRec)
{
    char temp[5] = {'0','x',NULL, NULL, NULL};
    unsigned int i = 0;
    char *Ascii;
    char *HexRec;

    Ascii = (char *)VdAscii;
    HexRec = (char *)VdHexRec;

    while(1)
    {
        temp[2] = Ascii[i++];
        temp[3] = Ascii[i++];
        if((temp[2] == NULL) || (temp[3] == NULL))
        {
            // Not a valid ASCII. Stop conversion and break.
            i -= 2;
            break;
        }
        else
        {
            // Convert ASCII to hex.
            sscanf(temp, "%hhx", HexRec);
            HexRec++;
        }
    }

    return (i/2); // i/2: Because representing Hex in ASCII takes 2 bytes.
}

void HexFile::CalculateFlashCRC(uint32_t *StartAdress, uint32_t *ProgLen, uint16_t *crc)
{
    uint8_t *VirtualFlash;
    unsigned short HexRecLen;
    char HexRec[255];
    T_HEX_RECORD HexRecordSt;
    uint32_t VirtualFlashAdrs;
    uint32_t ProgAddress;
    uint32_t flashBaseAddress;
    uint32_t flashEndAddress;

    flashBaseAddress = *StartAdress;
    flashEndAddress = *ProgLen;
    uint32_t flashLen = flashEndAddress - flashBaseAddress + 1;
    VirtualFlash = new uint8_t[flashLen];
    // Virtual Flash Erase (Set all bytes to 0xFF)
    memset((void*)VirtualFlash, 0xFF, flashLen);


    // Start decoding the hex file and write into virtual flash
    // Reset file pointer.
    fseek(HexFilePtr, 0, 0);

    // Reset max address and min address.
    HexRecordSt.MaxAddress = 0;
    HexRecordSt.MinAddress = 0xFFFFFFFF;

    while((HexRecLen = GetNextHexRecord(&HexRec[0], 255)) != 0)
    {
        HexRecordSt.RecDataLen = HexRec[0];
        HexRecordSt.RecType = HexRec[3];
        HexRecordSt.Data = (unsigned char*)&HexRec[4];

        switch(HexRecordSt.RecType)
        {

        case DATA_RECORD:  //Record Type 00, data record.
            HexRecordSt.Address = (((HexRec[1] << 8) & 0x0000FF00) | (HexRec[2] & 0x000000FF)) & (0x0000FFFF);
            HexRecordSt.Address = HexRecordSt.Address + HexRecordSt.ExtLinAddress + HexRecordSt.ExtSegAddress;

            ProgAddress = PA_TO_KVA0(HexRecordSt.Address);

            if(ProgAddress >= flashBaseAddress && ProgAddress <= flashEndAddress) // Make sure we are not writing boot sector.
            {
                if(HexRecordSt.MaxAddress < (ProgAddress + HexRecordSt.RecDataLen))
                {
                    HexRecordSt.MaxAddress = ProgAddress + HexRecordSt.RecDataLen;
                }

                if(HexRecordSt.MinAddress > ProgAddress)
                {
                    HexRecordSt.MinAddress = ProgAddress;
                }

                VirtualFlashAdrs = PA_TO_VFA(ProgAddress); // Program address to local virtual flash address

                memcpy((void *)&VirtualFlash[VirtualFlashAdrs], HexRecordSt.Data, HexRecordSt.RecDataLen);
            }
            break;

        case EXT_SEG_ADRS_RECORD:  // Record Type 02, defines 4 to 19 of the data address.
            HexRecordSt.ExtSegAddress = ((HexRecordSt.Data[0] << 16) & 0x00FF0000) | ((HexRecordSt.Data[1] << 8) & 0x0000FF00);
            HexRecordSt.ExtLinAddress = 0;
            break;

        case EXT_LIN_ADRS_RECORD:
            HexRecordSt.ExtLinAddress = ((HexRecordSt.Data[0] << 24) & 0xFF000000) | ((HexRecordSt.Data[1] << 16) & 0x00FF0000);
            HexRecordSt.ExtSegAddress = 0;
            break;


        case END_OF_FILE_RECORD:  //Record Type 01
        default:
            HexRecordSt.ExtSegAddress = 0;
            HexRecordSt.ExtLinAddress = 0;
            break;
        }
    }

    HexRecordSt.MinAddress -= HexRecordSt.MinAddress % 4;
    HexRecordSt.MaxAddress += HexRecordSt.MaxAddress % 4;

    *ProgLen = HexRecordSt.MaxAddress - HexRecordSt.MinAddress;
    *StartAdress = HexRecordSt.MinAddress;
    VirtualFlashAdrs = PA_TO_VFA(HexRecordSt.MinAddress);
    *crc = CRC::CalculateCRC(&VirtualFlash[VirtualFlashAdrs], *ProgLen);
    delete[] VirtualFlash;
}


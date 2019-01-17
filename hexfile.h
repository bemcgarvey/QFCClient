#ifndef HEXFILE_H
#define HEXFILE_H

#include <QString>
#include <stdio.h>
#include <stdint.h>

// Virtual Flash.
#define KB (1024)
#define MB (KB*KB)

typedef struct
{
    unsigned char RecDataLen;
    unsigned int Address;
    unsigned int MaxAddress;
    unsigned int MinAddress;
    unsigned char RecType;
    unsigned char* Data;
    unsigned char CheckSum;
    unsigned int ExtSegAddress;
    unsigned int ExtLinAddress;
}T_HEX_RECORD;

class HexFile
{
public:
    HexFile();
    ~HexFile();
    unsigned int HexTotalLines;
    unsigned int HexCurrLineNo;
    bool ResetHexFilePointer(void);
    bool LoadHexFile(QString fileName);
    unsigned short GetNextHexRecord(char *HexRec, unsigned int BuffLen);
    unsigned short ConvertAsciiToHex(void *VdAscii, void *VdHexRec);
    void CalculateFlashCRC(uint32_t *StartAdress, uint32_t *ProgLen, uint16_t *crc);
private:
    FILE *HexFilePtr;
};

#endif // HEXFILE_H

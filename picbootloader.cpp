#include "picbootloader.h"
#include <stdint.h>
#include "crc.h"

PICBootloader::PICBootloader(BootLoaderUSBLink *link) :
    hexFile(nullptr),
    usb(link)
{

}

PICBootloader::~PICBootloader()
{
    delete hexFile;
}

uint16_t PICBootloader::BootloaderVersion()
{
    uint8_t buffer[64];
    uint16_t version;
    buffer[0] = READ_BOOT_INFO;
    WriteToDevice(buffer, 1);
    bool success = ReadFromDevice(buffer);
    if (success) {
        version = *(uint16_t *)&buffer[1];
        return version;
    } else {
        return 0;
    }
}

bool PICBootloader::SetHexFile(QString fileName)
{
    bool success;
    delete hexFile;
    hexFile = new HexFile();
    success = hexFile->LoadHexFile(fileName);
    if (!success) {
        delete hexFile;
        hexFile = nullptr;
    }
    return success;
}

bool PICBootloader::EraseFlash()
{
    uint8_t buffer[64];
    emit Message("Erasing Flash", 0);
    bool success = GetMemoryInfo();
    if (!success) {
        return false;
    }
    int pages = (flashEndAddress - flashBaseAddress) / pageSize;
    buffer[0] = ERASE_FLASH;
    WriteToDevice(buffer, 1);
    emit ProcessStart();
    success = ReadFromDevice(buffer, 50, pages, true);
    if (success) {
        emit ProcessEnd();
        emit Message("Flash Erased", 3000);
    } else {
        emit Message("Erase Failed", 5000);
    }
    return success && buffer[0] == ERASE_FLASH;
}

bool PICBootloader::ProgramFlash()
{
    uint8_t buffer[64];
    buffer[0] = PROGRAM_FLASH;
    hexFile->ResetHexFilePointer();
    emit Message("Programming Flash", 0);
    emit ProcessStart();
    while (hexFile->HexCurrLineNo < hexFile->HexTotalLines) {
        int len = hexFile->GetNextHexRecord((char *)&buffer[1], 60);
        if (len > 0) {
            WriteToDevice(buffer, len + 1);
            bool success = ReadFromDevice(buffer, 50);
            if (!success || buffer[0] != PROGRAM_FLASH) {
                emit Message("Programming Failed", 5000);
                return false;
            }
        } else {
            break;
        }
        emit Progress(hexFile->HexCurrLineNo * 100 / hexFile->HexTotalLines);
    }
    emit ProcessEnd();
    emit Message("Flash Programmed", 3000);
    return true;
}

bool PICBootloader::VerifyFlash()
{
    uint16_t crc;
    uint32_t start;
    uint32_t len;

    emit Message("Verifying Flash", 0);
    emit ProcessStart();
    start = flashBaseAddress;
    len = flashEndAddress;
    hexFile->CalculateFlashCRC(&start, &len, &crc);
    uint8_t buffer[64];
    buffer[0] = READ_CRC;
    *(uint32_t *)&buffer[1] = start;
    *(uint32_t *)&buffer[5] = len;
    WriteToDevice(buffer, 9);
    bool success = ReadFromDevice(buffer);
    if (success && buffer[0] == READ_CRC) {
        uint16_t deviceCrc = *(uint16_t *)&buffer[1];
        if (crc == deviceCrc) {
            emit ProcessEnd();
            emit Message("Flash Verified", 3000);
            return true;
        }
    }
    emit Message("Verification Failed", 5000);
    return false;
}

bool PICBootloader::SignFlash()
{
    uint8_t buffer[64];
    emit Message("Signing Flash", 0);
    emit ProcessStart();
    buffer[0] = SIGN_FLASH;
    WriteToDevice(buffer, 1);
    bool success = ReadFromDevice(buffer);
    if (success) {
        emit ProcessEnd();
        emit Message("Flash Signed", 3000);
    } else {
        emit Message("Signing failed", 5000);
    }
    return success && buffer[0] == SIGN_FLASH;
}

void PICBootloader::JumpToApp()
{
    uint8_t buffer[64];
    buffer[0] = JMP_TO_APP;
    WriteToDevice(buffer, 1);
}

bool PICBootloader::EraseSignature()
{
    uint8_t buffer[64];

    emit Message("Erasing Signature", 0);
    buffer[0] = ERASE_SIGNATURE;
    emit ProcessStart();
    WriteToDevice(buffer, 1);
    bool success = ReadFromDevice(buffer);
    if (success) {
        emit ProcessEnd();
        emit Message("Signature Erased", 3000);
    } else {
        emit Message("Failed to erase signature", 5000);
    }
    return success && buffer[0] == ERASE_SIGNATURE;
}

bool PICBootloader::GetMemoryInfo()
{
    uint8_t buffer[64];
    buffer[0] = MEM_INFO;
    WriteToDevice(buffer, 1);
    bool success = ReadFromDevice(buffer);
    if (success && buffer[0] == MEM_INFO) {
        flashBaseAddress = *(uint32_t *)&buffer[1];
        flashEndAddress = *(uint32_t *)&buffer[5];
        pageSize = *(uint16_t *)&buffer[9];
        return true;
    } else {
        return false;
    }
}

bool PICBootloader::UnlockConfig(bool unlock)
{
    uint8_t buffer[64];
    buffer[0] = UNLOCK_CONFIG;
    if (unlock) {
        buffer[1] = 1;
    } else {
        buffer[1] = 0;
    }
    WriteToDevice(buffer, 2);
    bool success = ReadFromDevice(buffer);
    if (success) {
        if (unlock) {
            emit Message("Config unlocked", 3000);
        } else {
            emit Message("Config locked", 3000);
        }
    } else {
        emit Message("Lock/unlock config is not supported", 5000);
    }
    return success;
}

bool PICBootloader::WriteToDevice(uint8_t *buff, uint8_t len) {
    uint8_t txBuff[64];
    if (len > 61) {
        return false;
    }
    txBuff[0] = len;
    memcpy(&txBuff[1], buff, len);
    uint16_t crc = CRC::CalculateCRC(buff, len);
    ++len;
    txBuff[len] = crc & 0xff;
    ++len;
    txBuff[len] = (crc >> 8) & 0xff;
    ++len;
    int tries = 10;
    bool success;
    do {
        success = usb->WriteDevice(txBuff, len);
        --tries;
    } while (!success && tries > 0);
    return success;
}

int PICBootloader::ReadFromDevice(uint8_t *buff, int wait_ms, int maxRetries, bool signalProgress) {
    uint8_t rxBuff[64];
    bool success;
    int tries = 0;

    do {
        success = usb->ReadDevice(rxBuff, wait_ms);
        ++tries;
        if (signalProgress) {
            emit Progress(tries * 100 / maxRetries);
        }
    } while (!success && tries < maxRetries);
    if (!success) {
        return 0;
    }
    int len = rxBuff[0];
    if (len > 61) {
        return 0;
    }
    memcpy(buff, &rxBuff[1], len);
    uint16_t crc = rxBuff[len + 1] + (rxBuff[len + 2] << 8);
    if (CRC::CalculateCRC(buff, len) == crc) {
        return len;
    } else {
        return 0;
    }
}

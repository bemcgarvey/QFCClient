#ifndef PICBOOTLOADER_H
#define PICBOOTLOADER_H

#include "BootloaderUSBLink.h"
#include "hexfile.h"
#include <QString>
#include <QObject>
#include <stdint.h>

class PICBootloader : public QObject
{
    Q_OBJECT
public:
    PICBootloader(BootLoaderUSBLink *link);
    ~PICBootloader();
    enum {READ_BOOT_INFO = 1, ERASE_FLASH, PROGRAM_FLASH, READ_CRC, JMP_TO_APP, SIGN_FLASH, ERASE_SIGNATURE, MEM_INFO, UNLOCK_CONFIG};
    uint16_t BootloaderVersion();
    bool SetHexFile(QString fileName);
    bool EraseFlash();
    bool ProgramFlash();
    bool VerifyFlash();
    bool SignFlash();
    void JumpToApp();
    bool EraseSignature();
    bool GetMemoryInfo();
    bool UnlockConfig(bool unlock);
signals:
    void ProcessStart();
    void ProcessEnd();
    void Progress(int value);
    void Message(QString msg, int delay);
private:
    BootLoaderUSBLink *usb;
    HexFile *hexFile;
    bool WriteToDevice(uint8_t *buff, uint8_t len);
    int ReadFromDevice(uint8_t *buff, int wait_ms = 100, int maxRetries = 1, bool signalProgress = false);
    uint32_t flashBaseAddress;
    uint32_t flashEndAddress;
    unsigned int pageSize;
};

#endif // PICBOOTLOADER_H

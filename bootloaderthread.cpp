#include "bootloaderthread.h"

BootloaderThread::BootloaderThread(PICBootloader *bl) :
    bootloader(bl),
    success(false),
    task(TASK_NONE)
{

}

void BootloaderThread::run()
{
    switch (task) {
    case TASK_ERASE:
        success = bootloader->EraseSignature();
        if (success) {
            success = bootloader->EraseFlash();
        }
        break;
    case TASK_VERIFY:
        success = bootloader->VerifyFlash();
        break;
    case TASK_PROGRAM:
        success = bootloader->EraseSignature();
        if (success) {
            success = bootloader->EraseFlash();
        }
        if (success) {
            success = bootloader->ProgramFlash();
        }
        if (success) {
            success = bootloader->VerifyFlash();
        }
        if (success) {
            success = bootloader->SignFlash();
        }
        break;
    default:
        success = false;
    }
    emit taskDone(success, task);
}

void BootloaderThread::setTask(int value)
{
    task = value;
}




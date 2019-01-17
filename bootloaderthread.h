#ifndef BOOTLOADERTHREAD_H
#define BOOTLOADERTHREAD_H

#include <QObject>
#include "picbootloader.h"
#include <QThread>

class BootloaderThread : public QThread
{
    Q_OBJECT
public:
    BootloaderThread(PICBootloader *bl);
    void run() Q_DECL_OVERRIDE;
    void setTask(int value);
    enum {TASK_NONE, TASK_ERASE, TASK_PROGRAM, TASK_VERIFY};
private:
    PICBootloader *bootloader;
    int task;
    bool success;
signals:
    void taskDone(bool successful, int lastTask);
};

#endif // BOOTLOADERTHREAD_H

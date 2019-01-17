#include "calibratedialog.h"
#include "ui_calibratedialog.h"

CalibrateDialog::CalibrateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrateDialog), count(0)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    ui->setupUi(this);
    timer = startTimer(100);
}

CalibrateDialog::~CalibrateDialog()
{
    delete ui;
}

void CalibrateDialog::timerEvent(QTimerEvent *) {
    ++count;
    ui->progressBar->setValue(count * 4);
    if (count >= 25) {
        killTimer(timer);
        done(0);
    }
}

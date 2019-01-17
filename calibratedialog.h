#ifndef CALIBRATEDIALOG_H
#define CALIBRATEDIALOG_H

#include <QDialog>

namespace Ui {
class CalibrateDialog;
}

class CalibrateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrateDialog(QWidget *parent = 0);
    ~CalibrateDialog();
    void timerEvent(QTimerEvent *event);

private:
    Ui::CalibrateDialog *ui;
    int timer;
    int count;
};

#endif // CALIBRATEDIALOG_H

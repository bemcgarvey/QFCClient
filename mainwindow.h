#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "HidUSBLink.h"
#include "QFCConfig.h"
#include <QLabel>
#include "BootloaderUSBLink.h"
#include "bootloaderthread.h"
#include "picbootloader.h"
#include "QHidWatcher.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void timerEvent(QTimerEvent *event);

protected:

private slots:
    void on_connectButton_clicked();
    void on_ledButton_clicked();
    void on_actionExit_triggered();
    void on_loadConfigButton_clicked();
    void on_orientationPageRadioButton_toggled(bool checked);
    void on_generalPageRadioButton_toggled(bool checked);
    void on_modePageRadioButton_toggled(bool checked);
    void on_servoSetupRadioButton_toggled(bool checked);
    void on_calibrationRadioButton_toggled(bool checked);
    void on_actionAbout_QFC_triggered();
    void on_normalRB_toggled(bool checked);
    void on_invertedRB_toggled(bool checked);
    void on_sideRB_toggled(bool checked);
    void on_invertedSideRB_toggled(bool checked);
    void on_saveConfigButton_clicked();
    void on_wingNormalRB_toggled(bool checked);
    void on_wingDualAilRB_toggled(bool checked);
    void on_wingDeltaRB_toggled(bool checked);
    void on_tailNormalRB_toggled(bool checked);
    void on_tailVRB_toggled(bool checked);
    void on_ail2ReversedRB_toggled(bool checked);
    void on_differentailSpinBox_valueChanged(int arg1);
    void on_responseRateCB_currentIndexChanged(int index);
    void on_gyroModeCB_currentIndexChanged(int index);
    void on_stickModeCB_currentIndexChanged(int index);
    void on_maxRotationSpin_valueChanged(int arg1);
    void on_maxPitchSpin_valueChanged(int arg1);
    void on_ailEnabledCB_toggled(bool checked);
    void on_ailReverseCB_toggled(bool checked);
    void on_ailGainSpin_valueChanged(int arg1);
    void on_ailDeadbandSpin_valueChanged(int arg1);
    void on_elevEnabledCB_toggled(bool checked);
    void on_elevReverseCB_toggled(bool checked);
    void on_elevGainSpin_valueChanged(int arg1);
    void on_elevDeadbandSpin_valueChanged(int arg1);
    void on_rudEnabledCB_toggled(bool checked);
    void on_rudReverseCB_toggled(bool checked);
    void on_rudGainSpin_valueChanged(int arg1);
    void on_rudDeadbandSpin_valueChanged(int arg1);
    void on_modeCB_currentIndexChanged(int index);
    void on_ailMaxSpin_valueChanged(int arg1);
    void on_ailMinSpin_valueChanged(int arg1);
    void on_frameRateCB_currentIndexChanged(int index);
    void on_elevMaxSpin_valueChanged(int arg1);
    void on_elevMinSpin_valueChanged(int arg1);
    void on_rudMaxSpin_valueChanged(int arg1);
    void on_rudMinSpin_valueChanged(int arg1);
    void on_ail2MaxSpin_valueChanged(int arg1);
    void on_ail2MinSpin_valueChanged(int arg1);
    void on_ailMaxButton_toggled(bool checked);
    void on_ailMinButton_toggled(bool checked);
    void on_ailCenterButton_toggled(bool checked);
    void on_elevMaxButton_toggled(bool checked);
    void on_elevMinButton_toggled(bool checked);
    void on_elevCenterButton_toggled(bool checked);
    void on_rudMaxButton_toggled(bool checked);
    void on_rudMinButton_toggled(bool checked);
    void on_rudCenterButton_toggled(bool checked);
    void on_ail2MaxButton_toggled(bool checked);
    void on_ail2MinButton_toggled(bool checked);
    void on_ail2CenterButton_toggled(bool checked);
    void on_recenterButton_clicked();
    void on_browseFirmwareButton_clicked();
    void on_enableLiveDataCB_toggled(bool checked);
    void on_levelRollOffsetSpin_valueChanged(int arg1);
    void on_levelPitchOffsetSpin_valueChanged(int arg1);
    void on_hoverPitchOffsetSpin_valueChanged(int arg1);
    void on_hoverYawOffsetSpin_valueChanged(int arg1);
    void on_levelCalibrateButton_clicked();
    void on_hoverCalibrateButton_clicked();
    void on_updateFirmwareButton_clicked();
    void on_bootloaderButton_clicked();
    void onStart();
    void onEnd();
    void onProgress(int percent);
    void onThreadDone(bool success, int lastTask);
    void onMessage(QString msg, int delay);
    void on_tabs_currentChanged(int index);
    void on_defaultConfigButton_clicked();
    void on_pitchRateSpin_valueChanged(int arg1);
    void on_rollRateSpin_valueChanged(int arg1);
    void on_yawRateSpin_valueChanged(int arg1);
    void on_actionSave_Configuration_triggered();
    void on_actionLoad_Configuration_triggered();
    void on_savePIDButton_clicked();
    void on_pidIndexComboBox_currentIndexChanged(int index);
    void on_reloadPIDButton_clicked();
    void on_kpSpinBox_valueChanged(double arg1);
    void on_kiSpinBox_valueChanged(double arg1);
    void on_kdSpinBox_valueChanged(double arg1);
    void on_pidTypeComboBox_currentIndexChanged(int index);
    void on_modeNumComboBox_currentIndexChanged(int index);
    void on_ailPIDIndex_valueChanged(int arg1);
    void on_elevPIDIndex_valueChanged(int arg1);
    void on_rudPIDIndex_valueChanged(int arg1);
    void on_defaultPIDButton_clicked();
    void on_noLimitsCheckBox_clicked(bool checked);

private:
    Ui::MainWindow *ui;
    HidUSBLink usb;
    int liveDataTimerId;
    QFCConfig config;
    QLabel *connectLabel;
    void setConfigControls(void);
    Mode *currentMode;
    void setModeControls(void);
    void setModeBoxes(void);
    void setServoControls(void);
    void setCallibrationControls(void);
    void setAdvancedControls(void);
    void loadPIDs(void);
    void setServo(int value, int servo);
    BootLoaderUSBLink bootusb;
    PICBootloader *bootloader;
    BootloaderThread *bootloaderThread;
    uint8_t buffer[64];
    QHidWatcher *hidWatcher;
    QHidWatcher *bootloaderWatcher;
    PID pidList[PID_LIST_LEN];
};

#endif // MAINWINDOW_H

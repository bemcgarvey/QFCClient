
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "USBCommands.h"
#include <stdint.h>
#include "crc.h"
#include "aboutdialog.h"
#include <QFileDialog>
#include <math.h>
#include "calibratedialog.h"
#include <QMessageBox>
#include "quaternion.h"
#include "saveconfig.h"

//#define ATTITUDE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), usb(64, false), bootloader(nullptr) {
    ui->setupUi(this);
    connectLabel = new QLabel();
    connectLabel->setText("Not Connected");
    ui->statusBar->addPermanentWidget(connectLabel);
    hidWatcher = new QHidWatcher(0x0104);
    bootloaderWatcher = new QHidWatcher(0x1104);
    connect(hidWatcher, SIGNAL(connected()), this, SLOT(on_connectButton_clicked()), Qt::QueuedConnection);
    connect(hidWatcher, SIGNAL(removed()), this, SLOT(on_connectButton_clicked()), Qt::QueuedConnection);
    connect(bootloaderWatcher, SIGNAL(connected()), this, SLOT(on_connectButton_clicked()), Qt::QueuedConnection);
    connect(bootloaderWatcher, SIGNAL(removed()), this, SLOT(on_connectButton_clicked()), Qt::QueuedConnection);
}

MainWindow::~MainWindow() {
    delete ui;
    usb.Close();
    bootusb.Close();
    delete bootloader;
    delete hidWatcher;
    delete bootloaderWatcher;
}

void MainWindow::on_connectButton_clicked() {
    if (ui->enableLiveDataCB->isChecked()) {
        ui->enableLiveDataCB->setChecked(false);
    }
    bootusb.Close();
    usb.Close();
    usb.Open(0x0104);
    if (usb.Connected()) {
        connectLabel->setText("Connected");
        buffer[0] = CMD_VERSION;
        usb.SendReport(buffer);
        usb.GetReport(buffer);
        ui->firmwareVersionLabel->setText(QString((char *)&buffer[5]));
        ui->firmwareVersionLabel2->setText(QString((char *)&buffer[5]));
        ui->ledButton->setEnabled(true);
        ui->configTab->setEnabled(true);
        ui->liveDataTab->setEnabled(true);
        ui->advancedTab->setEnabled(true);
        ui->updateFirmwareTab->setEnabled(true);
        ui->updateFirmwareButton->setEnabled(false);
        ui->bootloaderButton->setEnabled(true);
        loadPIDs();
        on_loadConfigButton_clicked();
    } else {
        ui->ledButton->setEnabled(false);
        ui->configTab->setEnabled(false);
        ui->liveDataTab->setEnabled(false);
        ui->firmwareVersionLabel->clear();
        ui->updateFirmwareTab->setEnabled(false);
        bootusb.Open(0x1104);
        if (bootusb.Connected()) {
            // Enable bootloader controls
            ui->updateFirmwareTab->setEnabled(true);
            ui->updateFirmwareButton->setEnabled(true);
            ui->bootloaderProgressBar->setValue(0);
            ui->bootloaderButton->setEnabled(false);
            connectLabel->setText("Bootloader Connected");
            bootloader = new PICBootloader(&bootusb);
            connect(bootloader, &PICBootloader::ProcessStart, this,
                    &MainWindow::onStart);
            connect(bootloader, &PICBootloader::ProcessEnd, this, &MainWindow::onEnd);
            connect(bootloader, &PICBootloader::Progress, this,
                    &MainWindow::onProgress);
            connect(bootloader, &PICBootloader::Message, this,
                    &MainWindow::onMessage);
        } else {
            connectLabel->setText("Not Connected");
        }
    }
}

void MainWindow::on_ledButton_clicked() {
    buffer[0] = CMD_BLUE_TOGGLE;
    usb.SendReport(buffer);
    buffer[0] = CMD_YELLOW_TOGGLE;
    usb.SendReport(buffer);
    buffer[0] = CMD_GREEN_TOGGLE;
    usb.SendReport(buffer);
}

void MainWindow::timerEvent(QTimerEvent *event) {
    if (event->timerId() == liveDataTimerId) {
#ifndef ATTITUDE
        int ax, ay, az;
        buffer[0] = CMD_RAW_DATA;
        usb.SendReport(buffer);
        usb.GetReport(buffer);
        int16_t *n = (int16_t *)&buffer[1];
        QString str;
        ui->Ax->setText(str.setNum(*n));
        ax = *n;
        ++n;
        ui->Ay->setText(str.setNum(*n));
        ay = *n;
        ++n;
        ui->Az->setText(str.setNum(*n));
        az = *n;
        ++n;
        ++n;
        ui->Gx->setText(str.setNum(*n));
        ++n;
        ui->Gy->setText(str.setNum(*n));
        ++n;
        ui->Gz->setText(str.setNum(*n));
        double pitch;
        double roll;
        pitch = atan2(-ax, sqrt(ay * ay + az * az));
        pitch = pitch * 180 / 3.1415;
        ui->horizonWidget->setPitch((int)round(pitch));
        ui->pitchLabel->setText(str.setNum((int)round(pitch)) + "°");
        roll = atan2(ay, az);
        roll = roll * 180 / 3.1415;
        ui->horizonWidget->setRoll((int)round(roll));
        ui->rollLabel->setText(str.setNum((int)round(roll)) + "°");
#else
        buffer[0] = CMD_ATTITUDE;
        usb.SendReport(buffer);
        usb.GetReport(buffer);
        AttitudeData *attitude = reinterpret_cast<AttitudeData *>(&buffer[1]);
        QString str;
        Vector ypr;
        QuaternionToYPR(attitude->qAttitude, ypr);
        ui->horizonWidget->setRoll((int)round(ypr.roll));
        ui->rollLabel->setText(str.setNum(ypr.roll) + "°");
        ui->horizonWidget->setPitch((int)round(ypr.pitch));
        ui->pitchLabel->setText(str.setNum(ypr.pitch) + "°");
        static float min = 1000;
        static float max = 0;
        float time = attitude->qAttitude.time / 24.0;
        if (time < min) {
            min = time;
        }
        if (time > max) {
            max = time;
        }
        ui->Ax->setText(str.setNum(time));
        ui->Ay->setText(str.setNum(attitude->zSign));
        ui->Gx->setText(str.setNum(attitude->gyroRates.rollRate));
        ui->Gy->setText(str.setNum(attitude->gyroRates.pitchRate));
        ui->Gz->setText(str.setNum(attitude->gyroRates.yawRate));
#endif
    }
}

void MainWindow::on_actionExit_triggered() {
    close();
}

void MainWindow::on_loadConfigButton_clicked() {
    unsigned int len = 0;
    uint8_t page = 0;
    uint8_t *p = reinterpret_cast<uint8_t *>(&config);
    unsigned int size;

    while (len < sizeof(QFCConfig)) {
        buffer[0] = CMD_GET_CONFIG;
        buffer[1] = page;
        if (sizeof(QFCConfig) - len < 62) {
            size = sizeof(QFCConfig) - len;
        } else {
            size = 62;
        }
        usb.SendReport(buffer);
        usb.GetReport(buffer);
        memcpy(p, &buffer[2], size);
        p += size;
        len += size;
        ++page;
    }
    uint16_t crc = CRC::CalculateCRC(reinterpret_cast<uint8_t *>(&config), sizeof(QFCConfig) - 2);
    if (crc != config.crc) {
        ui->statusBar->showMessage("Error - Bad Config CRC", 6000);
    } else {
        ui->statusBar->showMessage("Configuration Loaded", 3000);
        setConfigControls();
    }
}

void MainWindow::setConfigControls(void) {
    switch (config.orientation) {
    case ORIENT_NORMAL:
        ui->normalRB->setChecked(true);
        break;
    case ORIENT_INVERT:
        ui->invertedRB->setChecked(true);
        break;
    case ORIENT_SIDE:
        ui->sideRB->setChecked(true);
        break;
    case ORIENT_INVERT_SIDE:
        ui->invertedSideRB->setChecked(true);
        break;
    }
    switch (config.wingType) {
    case WING_NORMAL:
        ui->wingNormalRB->setChecked(true);
        ui->dualAileronGroupBox->setEnabled(false);
        break;
    case WING_DUAL_AIL:
        ui->wingDualAilRB->setChecked(true);
        ui->dualAileronGroupBox->setEnabled(true);
        break;
    case WING_DELTA:
        ui->wingDeltaRB->setChecked(true);
        ui->dualAileronGroupBox->setEnabled(false);
        break;
    }
    switch (config.tailType) {
    case TAIL_NORMAL:
        ui->tailNormalRB->setChecked(true);
        break;
    case TAIL_V:
        ui->tailVRB->setChecked(true);
        break;
    }
    ui->differentailSpinBox->setValue(config.differential);
    ui->ail2ReversedRB->setChecked(config.ail2Direction == DIR_REV);
    ui->responseRateCB->setCurrentIndex(config.responseRate - SPEED_VSLOW);
    ui->modeCB->setCurrentIndex(0);
    setModeControls();
    setServoControls();
    setCallibrationControls();
}

void MainWindow::setServoControls(void) {
    QString str;
    ui->frameRateCB->setCurrentIndex(config.frameRate);
    ui->ailMaxSpin->setValue(config.ailServo.max / config.msConversion);
    ui->ailMinSpin->setValue(config.ailServo.min / config.msConversion);
    ui->ailCenterEdit->setText(str.setNum(config.ailServo.center / config.msConversion) + " µs");
    ui->ail2MaxSpin->setValue(config.ail2Servo.max / config.msConversion);
    ui->ail2MinSpin->setValue(config.ail2Servo.min / config.msConversion);
    ui->ail2CenterEdit->setText(str.setNum(config.ail2Servo.center / config.msConversion) + " µs");
    ui->elevMaxSpin->setValue(config.elevServo.max / config.msConversion);
    ui->elevMinSpin->setValue(config.elevServo.min / config.msConversion);
    ui->elevCenterEdit->setText(str.setNum(config.elevServo.center / config.msConversion) + " µs");
    ui->rudMaxSpin->setValue(config.rudServo.max / config.msConversion);
    ui->rudMinSpin->setValue(config.rudServo.min / config.msConversion);
    ui->rudCenterEdit->setText(str.setNum(config.rudServo.center / config.msConversion) + " µs");
}

void MainWindow::setModeControls(void) {
    currentMode = &config.modes[ui->modeCB->currentIndex()];
    ui->ailReverseCB->setChecked(config.ailGyroDirection == DIR_REV);
    ui->elevReverseCB->setChecked(config.elevGyroDirection == DIR_REV);
    ui->rudReverseCB->setChecked(config.rudGyroDirection == DIR_REV);
    ui->gyroModeCB->setCurrentIndex(currentMode->mode);
    ui->stickModeCB->setCurrentIndex(currentMode->stickMode);
    ui->ailEnabledCB->setChecked(currentMode->aileron.enabled == ENABLED);
    ui->ailGainSpin->setValue(currentMode->aileron.gain);
    ui->ailDeadbandSpin->setValue(currentMode->aileron.deadband);
    ui->elevEnabledCB->setChecked(currentMode->elevator.enabled == ENABLED);
    ui->elevGainSpin->setValue(currentMode->elevator.gain);
    ui->elevDeadbandSpin->setValue(currentMode->elevator.deadband);
    ui->rudEnabledCB->setChecked(currentMode->rudder.enabled == ENABLED);
    ui->rudGainSpin->setValue(currentMode->rudder.gain);
    ui->rudDeadbandSpin->setValue(currentMode->rudder.deadband);
    bool enable = true;
    if (ui->gyroModeCB->currentIndex() == 0) {
        enable = false;
    }
    ui->ailGroupBox->setEnabled(enable);
    ui->elevGroupBox->setEnabled(enable);
    ui->rudGroupBox->setEnabled(enable);
    ui->stickModeCB->setEnabled(enable);
    ui->maxGroupBox->setEnabled(true);
    ui->rateGroupBox->setEnabled(true);
    setModeBoxes();
}

void MainWindow::setModeBoxes(void) {
    switch (currentMode->mode) {
    case 0:
        ui->maxGroupBox->setVisible(false);
        ui->rateGroupBox->setVisible(false);
        break;
    case 1:
    case 2:
    case 4:
        ui->rollRateSpin->setValue(static_cast<int>(currentMode->maxRoll * RAD_TO_DEG));
        ui->pitchRateSpin->setValue(static_cast<int>(currentMode->maxPitch * RAD_TO_DEG));
        ui->yawRateSpin->setValue(static_cast<int>(currentMode->maxYaw * RAD_TO_DEG));
        ui->maxGroupBox->setVisible(false);
        if (currentMode->stickMode == STICK_MODE_AUTO) {
            ui->rateGroupBox->setVisible(true);
        } else {
            ui->rateGroupBox->setVisible(false);
        }
        break;
    case 3:
        ui->maxRotationSpin->setValue(static_cast<int>(currentMode->maxRoll * RAD_TO_DEG));
        ui->maxPitchSpin->setValue(static_cast<int>(currentMode->maxPitch * RAD_TO_DEG));
        ui->rateGroupBox->setVisible(false);
        ui->maxGroupBox->setVisible(true);
        if (currentMode->maxPitch * RAD_TO_DEG > 90 && currentMode->stickMode == STICK_MODE_MANUAL) {
            ui->noLimitsCheckBox->setChecked(true);
            ui->maxRotationSpin->setVisible(false);
            ui->maxPitchSpin->setVisible(false);
        } else {
            ui->noLimitsCheckBox->setChecked(false);
            ui->maxRotationSpin->setVisible(true);
            ui->maxPitchSpin->setVisible(true);
        }
        if (currentMode->stickMode == STICK_MODE_AUTO) {
            ui->noLimitsCheckBox->setVisible(false);
        } else {
            ui->noLimitsCheckBox->setVisible(true);
        }
        break;
    }
}

void MainWindow::setCallibrationControls(void) {
    ui->levelPitchOffsetSpin->setValue(static_cast<int>(config.pitchOffset * RAD_TO_DEG));
    ui->levelRollOffsetSpin->setValue(static_cast<int>(config.rollOffset * RAD_TO_DEG));
    ui->hoverPitchOffsetSpin->setValue(static_cast<int>(config.hoverPitchOffset * RAD_TO_DEG));
    ui->hoverYawOffsetSpin->setValue(static_cast<int>(config.hoverYawOffset * RAD_TO_DEG));
}

void MainWindow::on_orientationPageRadioButton_toggled(bool checked) {
    if (checked) {
        ui->configPages->setCurrentIndex(0);
    }
}

void MainWindow::on_generalPageRadioButton_toggled(bool checked) {
    if (checked) {
        ui->configPages->setCurrentIndex(1);
    }
}

void MainWindow::on_modePageRadioButton_toggled(bool checked) {
    if (checked) {
        ui->configPages->setCurrentIndex(2);
    }
}

void MainWindow::on_servoSetupRadioButton_toggled(bool checked) {
    if (checked) {
        ui->configPages->setCurrentIndex(3);
    }
}

void MainWindow::on_calibrationRadioButton_toggled(bool checked) {
    if (checked) {
        ui->configPages->setCurrentIndex(4);
    }
}

void MainWindow::on_actionAbout_QFC_triggered() {
    AboutDialog *about = new AboutDialog(this);
    about->exec();
    delete about;
}

void MainWindow::on_normalRB_toggled(bool checked) {
    if (checked) {
        ui->normalLabel->setFrameShape(QFrame::WinPanel);
        config.orientation = ORIENT_NORMAL;
    } else {
        ui->normalLabel->setFrameShape(QFrame::NoFrame);
    }
}

void MainWindow::on_invertedRB_toggled(bool checked) {
    if (checked) {
        ui->invertedLabel->setFrameShape(QFrame::WinPanel);
        config.orientation = ORIENT_INVERT;
    } else {
        ui->invertedLabel->setFrameShape(QFrame::NoFrame);
    }
}

void MainWindow::on_sideRB_toggled(bool checked) {
    if (checked) {
        ui->sideLabel->setFrameShape(QFrame::WinPanel);
        config.orientation = ORIENT_SIDE;
    } else {
        ui->sideLabel->setFrameShape(QFrame::NoFrame);
    }
}

void MainWindow::on_invertedSideRB_toggled(bool checked) {
    if (checked) {
        ui->invertedSideLabel->setFrameShape(QFrame::WinPanel);
        config.orientation = ORIENT_INVERT_SIDE;
    } else {
        ui->invertedSideLabel->setFrameShape(QFrame::NoFrame);
    }
}

void MainWindow::on_saveConfigButton_clicked() {
    int len = 0;
    uint8_t page = 0;
    uint8_t *p = (uint8_t *)&config;
    int size;

    config.crc = CRC::CalculateCRC((uint8_t *)&config, sizeof(QFCConfig) - 2);
    while (len < sizeof(QFCConfig)) {
        buffer[0] = CMD_SAVE_CONFIG;
        buffer[1] = page;
        if (sizeof(QFCConfig) - len < 62) {
            size = sizeof(QFCConfig) - len;
        } else {
            size = 62;
        }
        memcpy(&buffer[2], p, size);
        usb.SendReport(buffer);
        p += size;
        len += size;
        ++page;
    }
    usb.GetReport(buffer);
    if (buffer[1]) {
        ui->statusBar->showMessage("Configuration Saved", 2000);
    } else {
        ui->statusBar->showMessage("Configuration Save Error", 5000);
    }
}

void MainWindow::on_wingNormalRB_toggled(bool checked) {
    if (checked) {
        config.wingType = WING_NORMAL;
    }
}

void MainWindow::on_wingDualAilRB_toggled(bool checked) {
    if (checked) {
        config.wingType = WING_DUAL_AIL;
        ui->dualAileronGroupBox->setEnabled(true);
    } else {
        ui->dualAileronGroupBox->setEnabled(false);
    }
}

void MainWindow::on_wingDeltaRB_toggled(bool checked) {
    if (checked) {
        config.wingType = WING_DELTA;
    }
}

void MainWindow::on_tailNormalRB_toggled(bool checked) {
    if (checked) {
        config.tailType = TAIL_NORMAL;
    }
}

void MainWindow::on_tailVRB_toggled(bool checked) {
    if (checked) {
        config.tailType = TAIL_V;
    }
}

void MainWindow::on_ail2ReversedRB_toggled(bool checked) {
    if (checked) {
        config.ail2Direction = DIR_REV;
    } else {
        config.ail2Direction = DIR_NORM;
    }
}

void MainWindow::on_differentailSpinBox_valueChanged(int arg1) {
    config.differential = arg1;
}

void MainWindow::on_responseRateCB_currentIndexChanged(int index) {
    config.responseRate = index + SPEED_VSLOW;
}

void MainWindow::on_modeCB_currentIndexChanged(int index) {
    currentMode = &config.modes[index];
    setModeControls();
}

void MainWindow::on_gyroModeCB_currentIndexChanged(int index) {
    currentMode->mode = index;
    bool enable = true;
    if (index == 0) { // MODE_OFF
        enable = false;
        ui->maxGroupBox->setVisible(false);
        ui->rateGroupBox->setVisible(false);
    }
    ui->ailGroupBox->setEnabled(enable);
    ui->elevGroupBox->setEnabled(enable);
    ui->rudGroupBox->setEnabled(enable);
    ui->stickModeCB->setEnabled(enable);
    setModeBoxes();
}

void MainWindow::on_stickModeCB_currentIndexChanged(int index) {
    currentMode->stickMode = index;
    setModeBoxes();
}

void MainWindow::on_maxRotationSpin_valueChanged(int arg1) {
    currentMode->maxRoll = arg1 * DEG_TO_RAD;
}

void MainWindow::on_maxPitchSpin_valueChanged(int arg1) {
    currentMode->maxPitch = arg1 * DEG_TO_RAD;
}

void MainWindow::on_ailEnabledCB_toggled(bool checked) {
    currentMode->aileron.enabled = checked;
}

void MainWindow::on_ailReverseCB_toggled(bool checked) {
    if (checked) {
        config.ailGyroDirection = DIR_REV;
    } else {
        config.ailGyroDirection = DIR_NORM;
    }
}

void MainWindow::on_ailGainSpin_valueChanged(int arg1) {
    currentMode->aileron.gain = arg1;
}

void MainWindow::on_ailDeadbandSpin_valueChanged(int arg1) {
    currentMode->aileron.deadband = arg1;
}

void MainWindow::on_elevEnabledCB_toggled(bool checked) {
    currentMode->elevator.enabled = checked;
}

void MainWindow::on_elevReverseCB_toggled(bool checked) {
    if (checked) {
        config.elevGyroDirection = DIR_REV;
    } else {
        config.elevGyroDirection = DIR_NORM;
    }
}

void MainWindow::on_elevGainSpin_valueChanged(int arg1) {
    currentMode->elevator.gain = arg1;
}

void MainWindow::on_elevDeadbandSpin_valueChanged(int arg1) {
    currentMode->elevator.deadband = arg1;
}

void MainWindow::on_rudEnabledCB_toggled(bool checked) {
    currentMode->rudder.enabled = checked;
}

void MainWindow::on_rudReverseCB_toggled(bool checked) {
    if (checked) {
        config.rudGyroDirection = DIR_REV;
    } else {
        config.rudGyroDirection = DIR_NORM;
    }
}

void MainWindow::on_rudGainSpin_valueChanged(int arg1) {
    currentMode->rudder.gain = arg1;
}

void MainWindow::on_rudDeadbandSpin_valueChanged(int arg1) {
    currentMode->rudder.deadband = arg1;
}

void MainWindow::on_frameRateCB_currentIndexChanged(int index) {
    config.frameRate = index;
}

void MainWindow::setServo(int value, int servo) {
    buffer[0] = CMD_SET_SERVO;
    buffer[1] = static_cast<uint8_t>(servo);
    *((uint16_t *)&buffer[2]) = static_cast<uint16_t>(value);
    usb.SendReport(buffer);
}

void MainWindow::on_ailMaxSpin_valueChanged(int arg1) {
    config.ailServo.max = arg1 * config.msConversion;
    if (ui->ailMaxButton->isChecked()) {
        setServo(config.ailServo.max, AIL_SERVO);
    }
}

void MainWindow::on_ailMinSpin_valueChanged(int arg1) {
    config.ailServo.min = arg1 * config.msConversion;
    if (ui->ailMinButton->isChecked()) {
        setServo(config.ailServo.min, AIL_SERVO);
    }
}

void MainWindow::on_ail2MaxSpin_valueChanged(int arg1) {
    config.ail2Servo.max = arg1 * config.msConversion;
    if (ui->ail2MaxButton->isChecked()) {
        setServo(config.ail2Servo.max, AIL2_SERVO);
    }
}

void MainWindow::on_ail2MinSpin_valueChanged(int arg1) {
    config.ail2Servo.min = arg1 * config.msConversion;
    if (ui->ail2MinButton->isChecked()) {
        setServo(config.ail2Servo.min, AIL2_SERVO);
    }
}

void MainWindow::on_elevMaxSpin_valueChanged(int arg1) {
    config.elevServo.max = arg1 * config.msConversion;
    if (ui->elevMaxButton->isChecked()) {
        setServo(config.elevServo.max, ELEV_SERVO);
    }
}

void MainWindow::on_elevMinSpin_valueChanged(int arg1) {
    config.elevServo.min = arg1 * config.msConversion;
    if (ui->elevMinButton->isChecked()) {
        setServo(config.elevServo.min, ELEV_SERVO);
    }
}

void MainWindow::on_rudMaxSpin_valueChanged(int arg1) {
    config.rudServo.max = arg1 * config.msConversion;
    if (ui->rudMaxButton->isChecked()) {
        setServo(config.rudServo.max, RUD_SERVO);
    }
}

void MainWindow::on_rudMinSpin_valueChanged(int arg1) {
    config.rudServo.min = arg1 * config.msConversion;
    if (ui->rudMinButton->isChecked()) {
        setServo(config.rudServo.min, RUD_SERVO);
    }
}

void MainWindow::on_ailMaxButton_toggled(bool checked) {
    if (checked) {
        ui->ailCenterButton->setChecked(false);
        ui->ailMinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = AIL_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.ailServo.max;
        usb.SendReport(buffer);
    } else {
        if (!ui->ailMaxButton->isChecked() && !ui->ailMinButton->isChecked() &&
            !ui->ailCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = AIL_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_ailMinButton_toggled(bool checked) {
    if (checked) {
        ui->ailCenterButton->setChecked(false);
        ui->ailMaxButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = AIL_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.ailServo.min;
        usb.SendReport(buffer);
    } else {
        if (!ui->ailMaxButton->isChecked() && !ui->ailMinButton->isChecked() &&
            !ui->ailCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = AIL_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_ailCenterButton_toggled(bool checked) {
    if (checked) {
        ui->ailMaxButton->setChecked(false);
        ui->ailMinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = AIL_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.ailServo.center;
        usb.SendReport(buffer);
    } else {
        if (!ui->ailMaxButton->isChecked() && !ui->ailMinButton->isChecked() &&
            !ui->ailCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = AIL_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_ail2MaxButton_toggled(bool checked) {
    if (checked) {
        ui->ail2CenterButton->setChecked(false);
        ui->ail2MinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = AIL2_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.ail2Servo.max;
        usb.SendReport(buffer);
    } else {
        if (!ui->ail2MaxButton->isChecked() && !ui->ail2MinButton->isChecked() &&
            !ui->ail2CenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = AIL2_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_ail2MinButton_toggled(bool checked) {
    if (checked) {
        ui->ail2CenterButton->setChecked(false);
        ui->ail2MaxButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = AIL2_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.ail2Servo.min;
        usb.SendReport(buffer);
    } else {
        if (!ui->ail2MaxButton->isChecked() && !ui->ail2MinButton->isChecked() &&
            !ui->ail2CenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = AIL2_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_ail2CenterButton_toggled(bool checked) {
    if (checked) {
        ui->ail2MaxButton->setChecked(false);
        ui->ail2MinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = AIL2_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.ail2Servo.center;
        usb.SendReport(buffer);
    } else {
        if (!ui->ail2MaxButton->isChecked() && !ui->ail2MinButton->isChecked() &&
            !ui->ail2CenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = AIL2_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_elevMaxButton_toggled(bool checked) {
    if (checked) {
        ui->elevCenterButton->setChecked(false);
        ui->elevMinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = ELEV_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.elevServo.max;
        usb.SendReport(buffer);
    } else {
        if (!ui->elevMaxButton->isChecked() && !ui->elevMinButton->isChecked() &&
            !ui->elevCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = ELEV_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_elevMinButton_toggled(bool checked) {
    if (checked) {
        ui->elevCenterButton->setChecked(false);
        ui->elevMaxButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = ELEV_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.elevServo.min;
        usb.SendReport(buffer);
    } else {
        if (!ui->elevMaxButton->isChecked() && !ui->elevMinButton->isChecked() &&
            !ui->elevCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = ELEV_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_elevCenterButton_toggled(bool checked) {
    if (checked) {
        ui->elevMaxButton->setChecked(false);
        ui->elevMinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = ELEV_SERVO;
        usb.SendReport(buffer);
        buffer[1] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.elevServo.center;
        usb.SendReport(buffer);
    } else {
        if (!ui->elevMaxButton->isChecked() && !ui->elevMinButton->isChecked() &&
            !ui->elevCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = ELEV_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_rudMaxButton_toggled(bool checked) {
    if (checked) {
        ui->rudCenterButton->setChecked(false);
        ui->rudMinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = RUD_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.rudServo.max;
        usb.SendReport(buffer);
    } else {
        if (!ui->rudMaxButton->isChecked() && !ui->rudMinButton->isChecked() &&
            !ui->rudCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = RUD_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_rudMinButton_toggled(bool checked) {
    if (checked) {
        ui->rudCenterButton->setChecked(false);
        ui->rudMaxButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = RUD_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.rudServo.min;
        usb.SendReport(buffer);
    } else {
        if (!ui->rudMaxButton->isChecked() && !ui->rudMinButton->isChecked() &&
            !ui->rudCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = RUD_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_rudCenterButton_toggled(bool checked) {
    if (checked) {
        ui->rudMaxButton->setChecked(false);
        ui->rudMinButton->setChecked(false);
        buffer[0] = CMD_SERVO_ENABLE;
        buffer[1] = RUD_SERVO;
        usb.SendReport(buffer);
        buffer[0] = CMD_SET_SERVO;
        *((uint16_t *)&buffer[2]) = config.rudServo.center;
        usb.SendReport(buffer);
    } else {
        if (!ui->rudMaxButton->isChecked() && !ui->rudMinButton->isChecked() &&
            !ui->rudCenterButton->isChecked()) {
            buffer[0] = CMD_SERVO_DISABLE;
            buffer[1] = RUD_SERVO;
            usb.SendReport(buffer);
        }
    }
}

void MainWindow::on_recenterButton_clicked() {
    buffer[0] = CMD_SERVO_CENTER;
    usb.SendReport(buffer);
    CalibrateDialog *dlg = new CalibrateDialog(this);
    dlg->exec();
    delete dlg;
    buffer[0] = CMD_GET_SERVO_CENTERS;
    usb.SendReport(buffer);
    usb.GetReport(buffer);
    QString str;
    config.ailServo.center = *(uint16_t *)&buffer[1];
    ui->ailCenterEdit->setText(str.setNum(config.ailServo.center / config.msConversion) + " µs");
    config.elevServo.center = *(uint16_t *)&buffer[3];
    ui->elevCenterEdit->setText(str.setNum(config.elevServo.center / config.msConversion) + " µs");
    config.rudServo.center = *(uint16_t *)&buffer[5];
    ui->rudCenterEdit->setText(str.setNum(config.rudServo.center / config.msConversion) + " µs");
    config.ail2Servo.center = *(uint16_t *)&buffer[7];
    ui->ail2CenterEdit->setText(str.setNum(config.ail2Servo.center / config.msConversion) + " µs");
}

void MainWindow::on_browseFirmwareButton_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select Firmware File",
                                                    ".", "Hex files (*.hex)");
    if (!fileName.isNull()) {
        ui->firmwareFileEdit->setText(fileName);
    }
}

void MainWindow::on_enableLiveDataCB_toggled(bool checked) {
    if (checked) {
        if (usb.Connected()) {
#ifndef ATTITUDE
            buffer[0] = CMD_START_DATA;
#else
            buffer[0] = CMD_START_ATTITUDE;
#endif
            usb.SendReport(buffer);
            liveDataTimerId = startTimer(200);
        }
    } else {
        killTimer(liveDataTimerId);
#ifndef ATTITUDE
        buffer[0] = CMD_STOP_DATA;
#else
        buffer[0] = CMD_STOP_ATTITUDE;
#endif
        usb.SendReport(buffer);
    }
}

void MainWindow::on_levelRollOffsetSpin_valueChanged(int arg1) {
    config.rollOffset = arg1 * DEG_TO_RAD;
}

void MainWindow::on_levelPitchOffsetSpin_valueChanged(int arg1) {
    config.pitchOffset = arg1 * DEG_TO_RAD;
}

void MainWindow::on_hoverPitchOffsetSpin_valueChanged(int arg1) {
    config.hoverPitchOffset = arg1 * DEG_TO_RAD;
}

void MainWindow::on_hoverYawOffsetSpin_valueChanged(int arg1) {
    config.hoverYawOffset = arg1 * DEG_TO_RAD;
}

void MainWindow::on_levelCalibrateButton_clicked() {
    buffer[0] = CMD_H_CALIBRATE;
    usb.SendReport(buffer);
    CalibrateDialog *dlg = new CalibrateDialog(this);
    dlg->exec();
    buffer[0] = CMD_CALIBRATION_DATA;
    usb.SendReport(buffer);
    usb.GetReport(buffer);
    Vector *v;
    v = reinterpret_cast<Vector *>(&buffer[1]);
    ui->levelRollOffsetSpin->setValue(static_cast<int>(v->roll * RAD_TO_DEG));
    ui->levelPitchOffsetSpin->setValue(static_cast<int>(v->pitch * RAD_TO_DEG));
    delete dlg;
}

void MainWindow::on_hoverCalibrateButton_clicked() {
    buffer[0] = CMD_V_CALIBRATE;
    usb.SendReport(buffer);
    CalibrateDialog *dlg = new CalibrateDialog(this);
    dlg->exec();
    buffer[0] = CMD_CALIBRATION_DATA;
    usb.SendReport(buffer);
    usb.GetReport(buffer);
    Vector *v;
    v = reinterpret_cast<Vector *>(&buffer[1]);
    ui->hoverYawOffsetSpin->setValue(static_cast<int>(v->yaw * RAD_TO_DEG));
    ui->hoverPitchOffsetSpin->setValue(static_cast<int>(v->pitch * RAD_TO_DEG));
    delete dlg;
}

void MainWindow::on_updateFirmwareButton_clicked() {
    bool success = bootloader->SetHexFile(ui->firmwareFileEdit->text());
    if (success) {
        ui->updateFirmwareButton->setEnabled(false);
        ui->connectButton->setEnabled(false);
        bootloaderThread = new BootloaderThread(bootloader);
        bootloaderThread->setTask(BootloaderThread::TASK_PROGRAM);
        connect(bootloaderThread, &BootloaderThread::taskDone, this,
                &MainWindow::onThreadDone);
        bootloaderThread->start();
    } else {
        ui->statusBar->showMessage("Unable to open hex file", 5000);
    }
}

void MainWindow::on_bootloaderButton_clicked() {
    buffer[0] = CMD_BOOTLOAD;
    usb.SendReport(buffer);
}

void MainWindow::onStart() {
    ui->bootloaderProgressBar->setValue(0);
}

void MainWindow::onEnd() {
    ui->bootloaderProgressBar->setValue(100);
}

void MainWindow::onProgress(int percent) {
    ui->bootloaderProgressBar->setValue(percent);
}

void MainWindow::onThreadDone(bool success, int) {
    delete bootloaderThread;
    bootloaderThread = nullptr;
    ui->connectButton->setEnabled(true);
    if (success) {
        ui->statusBar->showMessage("Firmware updated successfuly", 3000);
        bootloader->JumpToApp();
    } else {
        ui->statusBar->showMessage("Firmware update failed", 5000);
        ui->updateFirmwareButton->setEnabled(true);
    }
}

void MainWindow::onMessage(QString msg, int delay) {
    ui->statusBar->showMessage(msg, delay);
}

void MainWindow::on_tabs_currentChanged(int index)
{
       if (index != 2) {
           ui->enableLiveDataCB->setChecked(false);
       }
       if (index == 4) {
            QMessageBox::warning(this, "Advanced settings", "These settings should only be changed by advanced users.  Incorrect settings may result in controller failure!",
                             QMessageBox::Ok);
            on_reloadPIDButton_clicked();
       }
}

void MainWindow::on_defaultConfigButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Default Config", "Are you sure you want to reset the config to the default values?  The current config will be lost.",
                                    QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        buffer[0] = CMD_DEFAULT_CONFIG;
        usb.SendReport(buffer);
        on_loadConfigButton_clicked();
    }
}

void MainWindow::on_pitchRateSpin_valueChanged(int arg1)
{
    currentMode->maxPitch = arg1 * DEG_TO_RAD;
}

void MainWindow::on_rollRateSpin_valueChanged(int arg1)
{
    currentMode->maxRoll = arg1 * DEG_TO_RAD;
}

void MainWindow::on_yawRateSpin_valueChanged(int arg1)
{
    currentMode->maxYaw = arg1 * DEG_TO_RAD;
}

void MainWindow::on_actionSave_Configuration_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save config as",
                                                    ".", "qfc files (*.qfc)");
    if (!fileName.isNull()) {
        uint16_t crc = CRC::CalculateCRC(reinterpret_cast<uint8_t *>(&config), sizeof(QFCConfig) - 2);
        config.crc = crc;
        if (!SaveConfigToFile(config, pidList, fileName)) {
            QMessageBox::critical(this, "Save Error", "Error saving config.", QMessageBox::Ok);
        }
    }

}

void MainWindow::on_actionLoad_Configuration_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load config", ".", "qfc files (*.qfc)");
    if (!fileName.isNull()) {
        LoadConfigFromFile(config, pidList, fileName);
        setConfigControls();
        setAdvancedControls();
    }
}

void MainWindow::setAdvancedControls(void) {
    //ui->pidIndexComboBox->clear();
    //TODO WTF ^  causes corruption of hidWatcher pointer
    if (ui->pidIndexComboBox->count() == 0) {
        for (int i = 0; i < PID_LIST_LEN; ++i) {
            ui->pidIndexComboBox->addItem(QString::asprintf("%d", i));
        }
    }
    ui -> modeNumComboBox->clear();
    for (int i = 1; i <= NUM_MODES; ++i) {
        ui -> modeNumComboBox->addItem(QString::asprintf("%d", i));
    }
    ui->pidIndexComboBox->setCurrentIndex(0);
    ui->modeNumComboBox->setCurrentIndex(0);
    ui->kpSpinBox->setValue(pidList[0].baseKp);
    ui->kiSpinBox->setValue(pidList[0].baseKi);
    ui->kdSpinBox->setValue(pidList[0].baseKd);
    ui->pidTypeComboBox->setCurrentIndex(pidList[0].type);
    ui->ailPIDIndex->setMaximum(PID_LIST_LEN - 1);
    ui->elevPIDIndex->setMaximum(PID_LIST_LEN - 1);
    ui->rudPIDIndex->setMaximum(PID_LIST_LEN - 1);
    ui->ailPIDIndex->setValue(config.modes[0].aileron.pidNumber);
    ui->elevPIDIndex->setValue(config.modes[0].elevator.pidNumber);
    ui->rudPIDIndex->setValue(config.modes[0].rudder.pidNumber);
}

void MainWindow::on_savePIDButton_clicked()
{
    on_saveConfigButton_clicked();
    int len = 0;
    uint8_t page = 0;
    uint8_t *p = (uint8_t *)pidList;
    int size;
    while (len < sizeof(PID) * PID_LIST_LEN) {
        buffer[0] = CMD_SAVE_PID;
        buffer[1] = page;
        if (sizeof(PID) * PID_LIST_LEN - len < 62) {
            size = sizeof(PID) * PID_LIST_LEN - len;
        } else {
            size = 62;
        }
        memcpy(&buffer[2], p, size);
        usb.SendReport(buffer);
        p += size;
        len += size;
        ++page;
    }
    usb.GetReport(buffer);
    if (buffer[1]) {
        ui->statusBar->showMessage("PID's Saved", 2000);
    } else {
        ui->statusBar->showMessage("PID Save Error", 5000);
    }
}



void MainWindow::on_pidIndexComboBox_currentIndexChanged(int index)
{
    ui->kpSpinBox->setValue(pidList[index].baseKp);
    ui->kiSpinBox->setValue(pidList[index].baseKi);
    ui->kdSpinBox->setValue(pidList[index].baseKd);
    ui->pidTypeComboBox->setCurrentIndex(pidList[index].type);
}

void MainWindow::on_reloadPIDButton_clicked()
{
    loadPIDs();
    ui->statusBar->showMessage("PID's loaded", 3000);
    setAdvancedControls();
}

void MainWindow::loadPIDs(void) {
    unsigned int len = 0;
    uint8_t page = 0;
    uint8_t *p = reinterpret_cast<uint8_t *>(pidList);
    unsigned int size;

    while (len < sizeof(PID) * PID_LIST_LEN) {
        buffer[0] = CMD_LOAD_PID;
        buffer[1] = page;
        if (sizeof(PID) * PID_LIST_LEN - len < 62) {
            size = sizeof(PID) * PID_LIST_LEN - len;
        } else {
            size = 62;
        }
        usb.SendReport(buffer);
        usb.GetReport(buffer);
        memcpy(p, &buffer[2], size);
        p += size;
        len += size;
        ++page;
    }
}

void MainWindow::on_kpSpinBox_valueChanged(double arg1)
{
    pidList[ui->pidIndexComboBox->currentIndex()].baseKp = static_cast<float>(arg1);
}

void MainWindow::on_kiSpinBox_valueChanged(double arg1)
{
    pidList[ui->pidIndexComboBox->currentIndex()].baseKi = static_cast<float>(arg1);
}

void MainWindow::on_kdSpinBox_valueChanged(double arg1)
{
    pidList[ui->pidIndexComboBox->currentIndex()].baseKd = static_cast<float>(arg1);
}

void MainWindow::on_pidTypeComboBox_currentIndexChanged(int index)
{
    pidList[ui->pidIndexComboBox->currentIndex()].type = index;
}

void MainWindow::on_modeNumComboBox_currentIndexChanged(int index)
{
    const Mode &mode = config.modes[index];
    ui->ailPIDIndex->setValue(mode.aileron.pidNumber);
    ui->elevPIDIndex->setValue(mode.elevator.pidNumber);
    ui->rudPIDIndex->setValue(mode.rudder.pidNumber);
}

void MainWindow::on_ailPIDIndex_valueChanged(int arg1)
{
    Mode &mode = config.modes[ui->modeNumComboBox->currentIndex()];
    mode.aileron.pidNumber = arg1;
}

void MainWindow::on_elevPIDIndex_valueChanged(int arg1)
{
    Mode &mode = config.modes[ui->modeNumComboBox->currentIndex()];
    mode.elevator.pidNumber = arg1;
}

void MainWindow::on_rudPIDIndex_valueChanged(int arg1)
{
    Mode &mode = config.modes[ui->modeNumComboBox->currentIndex()];
    mode.rudder.pidNumber = arg1;
}

void MainWindow::on_defaultPIDButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Default PIDs", "Are you sure you want to reset the PIDs to the default values?  The current PIDs will be lost.",
                                    QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        buffer[0] = CMD_DEFAULT_PID;
        usb.SendReport(buffer);
        on_reloadPIDButton_clicked();
    }
}

void MainWindow::on_noLimitsCheckBox_clicked(bool checked)
{
    ui->maxRotationSpin->setVisible(!checked);
    ui->maxPitchSpin->setVisible(!checked);
    ui->maxRotationSpin->setValue(181);
    ui->maxPitchSpin->setValue(91);
}

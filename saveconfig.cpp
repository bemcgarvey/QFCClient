#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include "QFCConfig.h"

QJsonObject modeToJson(const Mode &mode);
QJsonObject servoToJson(const Servo &servo);
QJsonObject channelToJson(const Channel &channel);
Channel jsonToChannel(const QJsonObject &json);
Servo jsonToServo(const QJsonObject &json);
Mode jsonToMode(const QJsonObject &json);
QJsonObject pidToJson(const PID &pid);
PID jsonToPID(const QJsonObject &json);


bool SaveConfigToFile(const QFCConfig &config, const PID *pidList, QString fileName) {
    QFile saveFile(fileName);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    QJsonObject json;
    json["wingType"] = config.wingType;
    json["tailType"] = config.tailType;
    json["differential"] = config.differential;
    json["ail2Direction"] = config.ail2Direction;
    json["frameRate"] =config.frameRate;
    json["orientation"] = config.orientation;
    json["rollOffset"] = config.rollOffset;
    json["pitchOffset"] = config.pitchOffset;
    json["hoverPitchOffset"] = config.hoverPitchOffset;
    json["hoverYawOffset"] = config.hoverYawOffset;
    json["responseRate"] = config.responseRate;
    json["ailGyroDirection"] = config.ailGyroDirection;
    json["elevGyroDirection"] = config.elevGyroDirection;
    json["rudGyroDirection"] = config.rudGyroDirection;
    json["msConversion"] = config.msConversion;
    json["crc"] = config.crc;
    QJsonArray modeArray;
    for (int i = 0; i < NUM_MODES; ++i) {
        modeArray.append(modeToJson(config.modes[i]));
    }
    json["modes"] = modeArray;
    json["ailServo"] = servoToJson(config.ailServo);
    json["elevServo"] = servoToJson(config.elevServo);
    json["rudServo"] = servoToJson(config.rudServo);
    json["ail2Servo"] = servoToJson(config.ail2Servo);
    QJsonArray pidArray;
    for (int i = 0; i < PID_LIST_LEN; ++i) {
        pidArray.append(pidToJson(pidList[i]));
    }
    json["pidList"] = pidArray;
    QJsonDocument doc(json);
    saveFile.write(doc.toJson());
    saveFile.close();
    return true;
}

bool LoadConfigFromFile(QFCConfig &config, PID *pidList, QString fileName) {
    QFile loadFile(fileName);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray data = loadFile.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));
    QJsonObject json = doc.object();
    config.wingType = json["wingType"].toInt();
    config.tailType = json["tailType"].toInt();
    config.differential = json["differential"].toInt();
    config.ail2Direction = json["ail2Direction"].toInt();
    config.frameRate = json["frameRate"].toInt();
    config.orientation = json["orientation"].toInt();
    config.rollOffset = static_cast<float>(json["rollOffset"].toDouble());
    config.pitchOffset = static_cast<float>(json["pitchOffset"].toDouble());
    config.hoverPitchOffset = static_cast<float>(json["hoverPitchOffset"].toDouble());
    config.hoverYawOffset = static_cast<float>(json["hoverYawOffset"].toDouble());
    config.responseRate = json["responseRate"].toInt();
    config.ailGyroDirection = json["ailGyroDirection"].toInt();
    config.elevGyroDirection = json["elevGyroDirection"].toInt();
    config.rudGyroDirection = json["rudGyroDirection"].toInt();
    config.msConversion = json["msConversion"].toInt();
    config.crc = json["crc"].toInt();
    QJsonArray modeArray = json["modes"].toArray();
    for (int i = 0; i < NUM_MODES; ++i) {
        config.modes[i] = jsonToMode(modeArray[i].toObject());
    }
    QJsonObject servo;
    servo = json["ailServo"].toObject();
    config.ailServo = jsonToServo(servo);
    servo = json["elevServo"].toObject();
    config.elevServo = jsonToServo(servo);
    servo = json["rudServo"].toObject();
    config.rudServo = jsonToServo(servo);
    servo = json["ail2Servo"].toObject();
    config.ail2Servo = jsonToServo(servo);
    QJsonArray pidArray = json["pidList"].toArray();
    for (int i = 0; i < PID_LIST_LEN; ++i) {
        pidList[i] = jsonToPID(pidArray[i].toObject());
    }
    return true;
}

QJsonObject servoToJson(const Servo &servo) {
    QJsonObject json;
    json["min"] = servo.min;
    json["max"] = servo.max;
    json["center"] = servo.center;
    return json;
}

Servo jsonToServo(const QJsonObject &json) {
    Servo servo;
    servo.min = json["min"].toInt();
    servo.max = json["max"].toInt();
    servo.center = json["center"].toInt();
    return servo;
}

QJsonObject modeToJson(const Mode &mode) {
    QJsonObject json;
    json["mode"] = mode.mode;
    json["stickMode"] = mode.stickMode;
    json["maxRoll"] = mode.maxRoll;
    json["maxPitch"] = mode.maxPitch;
    json["maxYaw"] = mode.maxYaw;
    json["aileron"] = channelToJson(mode.aileron);
    json["elevator"] = channelToJson(mode.elevator);
    json["rudder"] = channelToJson(mode.rudder);
    return json;
}

Mode jsonToMode(const QJsonObject &json) {
    Mode mode;
    mode.mode = json["mode"].toInt();
    mode.stickMode = json["stickMode"].toInt();
    mode.maxRoll = static_cast<float>(json["maxRoll"].toDouble());
    mode.maxPitch = static_cast<float>(json["maxPitch"].toDouble());
    mode.maxYaw = static_cast<float>(json["maxYaw"].toDouble());
    mode.aileron = jsonToChannel(json["aileron"].toObject());
    mode.elevator = jsonToChannel(json["elevator"].toObject());
    mode.rudder = jsonToChannel(json["rudder"].toObject());
    return mode;
}

QJsonObject channelToJson(const Channel &channel) {
    QJsonObject json;
    json["enabled"] = channel.enabled;
    json["gain"] = channel.gain;
    json["deadband"] = channel.deadband;
    json["pidNumber"] = channel.pidNumber;
    return json;
}

Channel jsonToChannel(const QJsonObject &json) {
    Channel channel;
    channel.enabled = json["enabled"].toInt();
    channel.gain = json["gain"].toInt();
    channel.deadband = json["deadband"].toInt();
    channel.pidNumber =json["pidNumber"].toInt();
    return channel;
}

QJsonObject pidToJson(const PID &pid) {
    QJsonObject json;
    json["baseKp"] = pid.baseKp;
    json["baseKi"] = pid.baseKi;
    json["baseKd"] = pid.baseKd;
    json["type"] = pid.type;
    return json;
}

PID jsonToPID(const QJsonObject &json) {
    PID pid;
    pid.baseKp = static_cast<float>(json["baseKp"].toDouble());
    pid.baseKi = static_cast<float>(json["baseKi"].toDouble());
    pid.baseKd = static_cast<float>(json["baseKd"].toDouble());
    pid.type = json["type"].toInt();
    return pid;
}

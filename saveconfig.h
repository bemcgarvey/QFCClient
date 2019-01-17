#ifndef SAVECONFIG_H
#define SAVECONFIG_H

#include <QString>
#include <QFCConfig.h>

bool SaveConfigToFile(const QFCConfig &config, const PID *pidList, QString fileName);
bool LoadConfigFromFile(QFCConfig &config, PID *pidList, QString fileName);


#endif // SAVECONFIG_H

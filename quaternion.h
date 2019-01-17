#ifndef QUATERNION_H
#define QUATERNION_H

#include <stdint.h>

typedef union {
    struct {
        float x;
        float y;
        float z;
    };
    struct {
        float roll;
        float pitch;
        float yaw;
    };
    struct {
        float rollRate;
        float pitchRate;
        float yawRate;
    };
} Vector;

typedef struct {
    float w;
    float x;
    float y;
    float z;
} Quaternion;

typedef struct {
    Quaternion qAttitude;
    Vector gyroRates;
    int zSign;  //sign of z axis acceleration: 1 = upright, -1 = inverted
} AttitudeData;

void QuaternionToYPR(Quaternion &q, Vector& ypr);

#define RAD_TO_DEG  (static_cast<float>(180.0 / 3.141592654))
#define DEG_TO_RAD  (static_cast<float>(3.141592654 / 180.0))

#endif // QUATERNION_H

#include "quaternion.h"
#include "math.h"

void QuaternionToYPR(Quaternion &q, Vector &ypr) {
        ypr.roll = static_cast<float>(atan2(q.y * q.z + q.w * q.x, 0.5f - (q.x * q.x + q.y * q.y)));
        ypr.roll *= RAD_TO_DEG;
        ypr.pitch = static_cast<float>(asin(-2.0f * (q.x * q.z - q.w * q.y)));
        ypr.pitch *= RAD_TO_DEG;
        ypr.yaw = static_cast<float>(atan2(q.x * q.y + q.w * q.z, 0.5f - (q.y * q.y + q.z * q.z)));
        ypr.yaw *= RAD_TO_DEG;
}

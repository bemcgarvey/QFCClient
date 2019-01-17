#ifndef CRC_H
#define CRC_H

#include <stdint.h>

class CRC
{
public:
    CRC();
    static uint16_t CalculateCRC(uint8_t *data, unsigned int len);
};

#endif // CRC_H

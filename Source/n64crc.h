#ifndef _n64crc_h_
#define _n64crc_h_

#include "C_Base.h"

namespace N64CRC
{
    void UpdateCRC(void* aRomBuff, uint32 aRomSize);
}

#endif // _n64crc_h_

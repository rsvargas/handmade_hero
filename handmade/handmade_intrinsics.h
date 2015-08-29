#pragma once

#include <math.h>

inline int32 RoundReal32ToInt32(real32 val)
{
    int32 Result = (int32)roundf(val);
    return Result;
}

inline uint32 RoundReal32ToUInt32(real32 val)
{
    uint32 Result = (uint32)roundf(val);
    return Result;
}

inline int32 FloorReal32ToInt32(real32 val)
{
    return (int32)floorf(val);
}


inline int32 TruncateReal32ToInt32(real32 val)
{
    return (int32)val;
}

inline real32 Sin(real32 Angle)
{
    return sinf(Angle);
}

inline real32 Cos(real32 Angle)
{
    return cosf(Angle);
}

inline real32 ATan2(real32 X, real32 Y)
{
    return atan2f(X, Y);
}


struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};
inline bit_scan_result FindLeastSignificantSetBit(uint32 Value)
{
    bit_scan_result Result = {};
#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long*)&Result.Index, Value);
#else
    for (uint32 Test = 0; Test < 32; ++Test)
    {
        if (Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif
    return Result;
}


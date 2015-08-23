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

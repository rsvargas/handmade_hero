#pragma once

#include <math.h>

#if COMPILER_MSVC
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier(); 
#else
//TODO: define these on GCC/LLVM?
#define CompletePreviousWritesBeforeFutureWrites
#endif

inline int32 SignOf(int32 Value)
{
    int32 Result = (Value >= 0)? 1 : -1;
    return Result;
}

inline real32 SquareRoot(real32 Value)
{
    real32 Result = sqrtf(Value);
    return Result;
}

inline real32 AbsoluteValue(real32 val)
{
    real32 Result = (real32)fabs(val);
    return Result;
}

inline uint32 RotateLeft(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
    uint32 Result = _rotl(Value, Amount);
#else
    Amount &= 31;
    uint32 Result = (Value << Amount) | (Value >> (32 - Amount));
#endif
    return Result;
}

inline uint32 RotateRight(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
    uint32 Result = _rotr(Value, Amount);
#else
    Amount &= 31;
    uint32 Result = (Value << Amount) | (Value >> (32 - Amount));
#endif
    return Result;
}

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

inline int32 CeilReal32ToInt32(real32 val)
{
    return (int32)ceilf(val);
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


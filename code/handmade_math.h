#pragma once

union v2
{
    struct
    {
        real32 x, y;
    };
    real32 E[2];
};

union v3
{
    struct
    {
        real32 x, y, z;
    };
    struct
    {
        real32 r, g, b;
    };
    struct
    {
        v2 xy;
        real32 Ignored0_;
    };
    struct
    {
        real32 Ignored1_;
        v2 yz;
    };
    real32 E[3];
};

union v4
{
    struct
    {
        real32 x, y, z, w;
    };
    struct
    {
        real32 r, g, b, a;
    };
    real32 E[4];
};

struct rectangle2
{
    v2 Min;
    v2 Max;
};

struct rectangle3
{
    v3 Min;
    v3 Max;
};

inline v2 V2i(int32 X, int32 Y)
{
    v2 Result ={(real32)X, (real32)Y};
    return Result;
}

inline v2 V2i(uint32 X, uint32 Y)
{
    v2 Result ={(real32)X, (real32)Y};
    return Result;
}

inline v2 V2(real32 X, real32 Y)
{
    v2 Result;

    Result.x = X;
    Result.y = Y;

    return Result;
}

inline v3 V3(real32 X, real32 Y, real32 Z)
{
    v3 Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;

    return Result;
}

inline v3 V3(v2 XY, real32 Z)
{
    v3 Result;

    Result.x = XY.x;
    Result.y = XY.y;
    Result.z = Z;

    return Result;
}

inline v4 V4(real32 X, real32 Y, real32 Z, real32 W)
{
    v4 Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    Result.w = W;

    return Result;
}

//
// Scalar Operations
//

inline real32 Square(real32 A)
{
    real32 Result = A*A;
    return Result;
}

inline real32 Lerp(real32 A, real32 t, real32 B)
{
    real32 Result = (1.0f - t)*A + t*B;
    return Result;
}

inline real32 Clamp(real32 Min, real32 Value, real32 Max)
{
    real32 Result = Value;

    if (Result < Min)
    {
        Result = Min;
    }
    else if (Result > Max)
    {
        Result = Max;
    }

    return Result;
}

inline real32 Clamp01(real32 Value)
{
    real32 Result = Clamp(0.0f, Value, 1.0f);
    return Result;
}

inline real32 SafeRatioN(real32 Numerator, real32 Divisor, real32 N)
{
    real32 Result = N;

    if (Divisor != 0.0f)
    {
        Result = Numerator / Divisor;
    }
    return Result;
}

inline real32 SafeRatio0(real32 Numerator, real32 Divisor)
{
    real32 Result = SafeRatioN(Numerator, Divisor, 0.0f);
    return Result;
}

inline real32 SafeRatio1(real32 Numerator, real32 Divisor)
{
    real32 Result = SafeRatioN(Numerator, Divisor, 1.0f);
    return Result;
}



//
// V2 Operations
//


inline v2 operator*(real32 A, v2 B)
{
    v2 Result;

    Result.x = A*B.x;
    Result.y = A*B.y;

    return Result;
}

inline v2 operator*(v2 B, real32 A)
{
    v2 Result = A*B;

    return Result;
}

inline v2& operator*=(v2& A, real32 B)
{
    A = B * A;

    return A;
}

inline v2 operator-(v2 A)
{
    v2 Result;

    Result.x = -A.x;
    Result.y = -A.y;

    return Result;
}

inline v2 operator+(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return Result;
}

inline v2& operator+=(v2& A, v2 B)
{
    A = A + B;

    return A;
}

inline v2 operator-(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return Result;
}

inline v2 Hadamard(v2 A, v2 B)
{
    v2 Result = { A.x*B.x, A.y*B.y };

    return Result;
}


inline real32 Inner(v2 A, v2 B)
{
    real32 Result = A.x*B.x + A.y*B.y;
    return Result;
}

//Length is the square root of the inner product!
inline real32 LengthSq(v2 A)
{
    real32 Result = Inner(A, A);
    return Result;
}

inline real32 Length(v2 A)
{
    real32 Result = SquareRoot(LengthSq(A));
    return Result;
}

inline v2 Clamp01(v2 Value)
{
    v2 Result;

    Result.x = Clamp01(Value.x);
    Result.y = Clamp01(Value.y);

    return Result;
}


//
// V3 Operations
//

inline v3 operator*(real32 A, v3 B)
{
    v3 Result;

    Result.x = A*B.x;
    Result.y = A*B.y;
    Result.z = A*B.z;

    return Result;
}

inline v3 operator*(v3 B, real32 A)
{
    v3 Result = A*B;

    return Result;
}

inline v3& operator*=(v3& A, real32 B)
{
    A = B * A;

    return A;
}

inline v3 operator-(v3 A)
{
    v3 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;

    return Result;
}

inline v3 operator+(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;

    return Result;
}

inline v3& operator+=(v3& A, v3 B)
{
    A = A + B;

    return A;
}

inline v3 operator-(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;

    return Result;
}

inline v3 Hadamard(v3 A, v3 B)
{
    v3 Result = { A.x*B.x, A.y*B.y, A.z*B.z };

    return Result;
}

inline real32 Inner(v3 A, v3 B)
{
    real32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
    return Result;
}

//Length is the square root of the inner product!
inline real32 LengthSq(v3 A)
{
    real32 Result = Inner(A, A);
    return Result;
}

inline real32 Length(v3 A)
{
    real32 Result = SquareRoot(LengthSq(A));
    return Result;
}

inline v3 Clamp01(v3 value)
{
    v3 Result;
    Result.x = Clamp01(value.x);
    Result.y = Clamp01(value.y);
    Result.z = Clamp01(value.z);

    return Result;
}

//
//  Rectangle 2
//


inline v2 GetMinCorner(rectangle2 Rect)
{
    v2 Result = Rect.Min;
    return Result;
}

inline v2 GetMaxCorner(rectangle2 Rect)
{
    v2 Result = Rect.Max;
    return Result;
}

inline v2 GetCenter (rectangle2 Rect)
{
    v2 Result = 0.5f*(Rect.Min + Rect.Max);
    return Result;
}

inline rectangle2 RectMinMax(v2 Min, v2 Max)
{
    rectangle2 Result;

    Result.Min = Min;
    Result.Max = Max;

    return Result;
}

inline rectangle2 RectMinDim(v2 Min, v2 Dim)
{
    rectangle2 Result;

    Result.Min = Min;
    Result.Max = Min + Dim;

    return Result;
}

inline rectangle2 RectCenterHalfDim(v2 Center, v2 HalfDim)
{
    rectangle2 Result;

    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;

    return Result;
}


inline rectangle2 AddRadiusTo(rectangle2 A, v2 Radius)
{
    rectangle2 Result;

    Result.Min = A.Min - Radius;
    Result.Max = A.Max + Radius;

    return Result;
}

inline rectangle2 RectCenterDim(v2 Center, v2 Dim)
{
    rectangle2 Result = RectCenterHalfDim(Center, 0.5f * Dim);

    return Result;
}


inline bool32 IsInRectangle(rectangle2 Rectangle, v2 Test)
{
    bool32 Result = ((Test.x >= Rectangle.Min.x) &&
        (Test.y >= Rectangle.Min.y) &&
        (Test.x < Rectangle.Max.x) &&
        (Test.y < Rectangle.Max.y));

    return Result;
}

inline v2 GetBarycentric(rectangle2 A, v2 P)
{
    v2 Result;

    Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
    Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);

    return Result;
}

//
//  Rectangle 3
//
inline v3 GetMinCorner(rectangle3 Rect)
{
    v3 Result = Rect.Min;
    return Result;
}

inline v3 GetMaxCorner(rectangle3 Rect)
{
    v3 Result = Rect.Max;
    return Result;
}

inline v3 GetCenter(rectangle3 Rect)
{
    v3 Result = 0.5f*(Rect.Min + Rect.Max);
    return Result;
}

inline rectangle3 RectMinMax(v3 Min, v3 Max)
{
    rectangle3 Result;

    Result.Min = Min;
    Result.Max = Max;

    return Result;
}

inline rectangle3 RectMinDim(v3 Min, v3 Dim)
{
    rectangle3 Result;

    Result.Min = Min;
    Result.Max = Min + Dim;

    return Result;
}

inline rectangle3 RectCenterHalfDim(v3 Center, v3 HalfDim)
{
    rectangle3 Result;

    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;

    return Result;
}


inline rectangle3 AddRadiusTo(rectangle3 A, v3 Radius)
{
    rectangle3 Result;

    Result.Min = A.Min - Radius;
    Result.Max = A.Max + Radius;

    return Result;
}

inline rectangle3 Offset(rectangle3 A, v3 Offset)
{
    rectangle3 Result;
    
    Result.Min = A.Min + Offset;
    Result.Max = A.Max + Offset;

    return Result;
}

inline rectangle3 RectCenterDim(v3 Center, v3 Dim)
{
    rectangle3 Result = RectCenterHalfDim(Center, 0.5f * Dim);

    return Result;
}


inline bool32 IsInRectangle(rectangle3 Rectangle, v3 Test)
{
    bool32 Result = ((Test.x >= Rectangle.Min.x) &&
        (Test.y >= Rectangle.Min.y) &&
        (Test.z >= Rectangle.Min.z) &&
        (Test.x < Rectangle.Max.x) &&
        (Test.y < Rectangle.Max.y) &&
        (Test.z < Rectangle.Max.z));

    return Result;
}

inline bool32 RectanglesIntersect(rectangle3 A, rectangle3 B)
{
    bool32 Result = !(
        (B.Max.x <= A.Min.x) ||
        (B.Min.x >= A.Max.x) ||
        (B.Max.y <= A.Min.y) ||
        (B.Min.y >= A.Max.y) ||
        (B.Max.z <= A.Min.z) ||
        (B.Min.z >= A.Max.z));
    return Result;
}

inline v3 GetBarycentric(rectangle3 A, v3 P)
{
    v3 Result;
        
    Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
    Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);
    Result.z = SafeRatio0(P.z - A.Min.z, A.Max.z - A.Min.z);

    return Result;
}

inline rectangle2 ToRectangleXY(rectangle3 A)
{
    rectangle2 Result;

    Result.Min = A.Min.xy;
    Result.Max = A.Max.xy;

    return Result;
}

#pragma once

union v2
{
    struct
    {
        real32 X, Y;
    };
    real32 E[2];
};

union v3
{
    struct
    {
        real32 X, Y, Z;
    };
    struct
    {
        real32 R, G, B;
    };
    real32 E[3];
};

union v4
{
    struct
    {
        real32 X, Y, Z, W;
    };
    struct
    {
        real32 R, G, B, A;
    };
    real32 E[4];
};

inline v2 V2(real32 X, real32 Y)
{
    v2 Result;

    Result.X = X;
    Result.Y = Y;

    return Result;
}

inline v3 V3(real32 X, real32 Y, real32 Z)
{
    v3 Result;

    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;

    return Result;
}
inline v4 V4(real32 X, real32 Y, real32 Z, real32 W)
{
    v4 Result;

    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;

    return Result;
}


inline v2 operator*(real32 A, v2 B)
{
    v2 Result;

    Result.X = A*B.X;
    Result.Y = A*B.Y;

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

    Result.X = -A.X;
    Result.Y = -A.Y;

    return Result;
}

inline v2 operator+(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

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

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

inline real32 Square(real32 A)
{
    real32 Result = A*A;
    return Result;
}

inline real32 Inner(v2 A, v2 B)
{
    real32 Result = A.X*B.X + A.Y*B.Y;
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

struct rectangle2
{
    v2 Min;
    v2 Max;
};

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


inline rectangle2 AddRadiusTo(rectangle2 A, real32 RadiusW, real32 RadiusH)
{
    rectangle2 Result;

    Result.Min = A.Min - V2(RadiusW, RadiusH);
    Result.Max = A.Max + V2(RadiusW, RadiusH);

    return Result;
}

inline rectangle2 RectCenterDim(v2 Center, v2 Dim)
{
    rectangle2 Result = RectCenterHalfDim(Center, 0.5f * Dim);

    return Result;
}


inline bool32 IsInRectangle(rectangle2 Rectangle, v2 Test)
{
    bool32 Result = ((Test.X >= Rectangle.Min.X) &&
        (Test.Y >= Rectangle.Min.Y) &&
        (Test.X < Rectangle.Max.X) &&
        (Test.Y < Rectangle.Max.Y));

    return Result;
}

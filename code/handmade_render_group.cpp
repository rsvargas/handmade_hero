
inline v4 Unpack4x8(uint32 Packed)
{
    v4 Result = {(real32)((Packed >> 16) & 0xFF),
                 (real32)((Packed >> 8) & 0xFF),
                 (real32)((Packed >> 0) & 0xFF),
                 (real32)((Packed >> 24) & 0xFF)};
    return Result;
}

inline v4 SRGB255ToLinear1(v4 C)
{
    v4 Result;

    real32 Inv255 = 1.0f / 255.0f;

    Result.r = Square(Inv255 * C.r);
    Result.g = Square(Inv255 * C.g);
    Result.b = Square(Inv255 * C.b);
    Result.a = Inv255 * C.a;

    return Result;
}

inline v4 Linear1ToSRGB255(v4 C)
{
    v4 Result;

    real32 One255 = 255.0f;

    Result.r = One255 * SquareRoot(C.r);
    Result.g = One255 * SquareRoot(C.g);
    Result.b = One255 * SquareRoot(C.b);
    Result.a = One255 * C.a;

    return Result;
}

inline v4 UnscaledAndBiasNormal(v4 Normal)
{
    v4 Result;
    real32 Inv255 = 1.0f / 255.0f;

    Result.x = -1.0f + 2.0f * (Inv255 * Normal.x);
    Result.y = -1.0f + 2.0f * (Inv255 * Normal.y);
    Result.z = -1.0f + 2.0f * (Inv255 * Normal.z);

    Result.w = Inv255 * Normal.w;

    return (Result);
}

internal void DrawRectangle(loaded_bitmap *Buffer,
                            v2 vMin, v2 vMax, v4 Color)
{
    real32 R = Color.r;
    real32 G = Color.g;
    real32 B = Color.b;
    real32 A = Color.a;
    int32 MinX = RoundReal32ToInt32(vMin.x);
    int32 MinY = RoundReal32ToInt32(vMin.y);
    int32 MaxX = RoundReal32ToInt32(vMax.x);
    int32 MaxY = RoundReal32ToInt32(vMax.y);

    if (MinX < 0)
    {
        MinX = 0;
    }

    if (MinY < 0)
    {
        MinY = 0;
    }

    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32 Color32 = ((RoundReal32ToUInt32(A * 255.0f) << 24) |
                      (RoundReal32ToUInt32(R * 255.0f) << 16) |
                      (RoundReal32ToUInt32(G * 255.0f) << 8) |
                      (RoundReal32ToUInt32(B * 255.0f) << 0));

    uint8 *Row = (((uint8 *)Buffer->Memory) +
                  MinX * BITMAP_BYTES_PER_PIXEL +
                  MinY * Buffer->Pitch);

    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color32;
        }
        Row += Buffer->Pitch;
    }
}

struct bilinear_sample
{
    uint32 A, B, C, D;
};

inline bilinear_sample BilinearSample(loaded_bitmap *Texture, int32 X, int32 Y)
{
    bilinear_sample Result;

    uint8 *TexelPtr = (((uint8 *)Texture->Memory) + X * BITMAP_BYTES_PER_PIXEL + Y * Texture->Pitch);
    Result.A = *(uint32 *)(TexelPtr);
    Result.B = *(uint32 *)(TexelPtr + BITMAP_BYTES_PER_PIXEL);
    Result.C = *(uint32 *)(TexelPtr + Texture->Pitch);
    Result.D = *(uint32 *)(TexelPtr + Texture->Pitch + BITMAP_BYTES_PER_PIXEL);

    return Result;
}

inline v4 SRGBBilinearBlend(bilinear_sample TexelSample, real32 fX, real32 fY)
{
    v4 TexelA = Unpack4x8(TexelSample.A);
    v4 TexelB = Unpack4x8(TexelSample.B);
    v4 TexelC = Unpack4x8(TexelSample.C);
    v4 TexelD = Unpack4x8(TexelSample.D);

    //NOTE: Go from sRGB to "linear" brightness space
    TexelA = SRGB255ToLinear1(TexelA);
    TexelB = SRGB255ToLinear1(TexelB);
    TexelC = SRGB255ToLinear1(TexelC);
    TexelD = SRGB255ToLinear1(TexelD);

    //NOTE: Bilinear Blend
    v4 Texel = Lerp(Lerp(TexelA, fX, TexelB),
                    fY,
                    Lerp(TexelC, fX, TexelD));

    return Texel;
}

inline v3 SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map,
                               real32 DistanceFromMapInZ)
{
    /* 
        ScreenSpaceUV tells is where the way is being cast _from_ in
        normalized screencoordinates.

        SampleDirection tells us what direction the cast is going - it 
        does not have to be normalized.

        Roughness says which LODs of Map we samle from.
      
        DistanceFromMapInZ says how far the map is from the sample poijnt in Z, given
        in meters.
    */

    //NOTE: Puck which LOD to sample from
    uint32 LODIndex = (uint32)(Roughness * (real32)(ArrayCount(Map->LOD) - 1) + 0.5f);
    Assert(LODIndex < ArrayCount(Map->LOD));

    loaded_bitmap *LOD = &Map->LOD[LODIndex];

    //NOTE: Compute the distance to the map and the scaling
    // factor for meters-to-UVs
    real32 UVsPerMeter = 0.1f;
    real32 C = (UVsPerMeter * DistanceFromMapInZ) / SampleDirection.y;
    v2 Offset = C * V2(SampleDirection.x, SampleDirection.z);

    // NOTE: Find the intecsectino point
    v2 UV = ScreenSpaceUV + Offset;

    // NOTE: Clamp to the valid range
    UV.x = Clamp01(UV.x);
    UV.y = Clamp01(UV.y);

    //NOTE Bilinear smaple
    real32 tX = ((UV.x * (real32)(LOD->Width - 2)));
    real32 tY = ((UV.y * (real32)(LOD->Height - 2)));

    int32 X = (int32)(tX);
    int32 Y = (int32)(tY);

    real32 fX = tX - (real32)X;
    real32 fY = tY - (real32)Y;

    Assert((X >= 0.0f) && (X < LOD->Width));
    Assert((Y >= 0.0f) && (Y < LOD->Height));

#if 0
    // Note: Turn this on to see where in the map we are sampling
    uint8* TexelPtr = (((uint8*)LOD->Memory) + X * sizeof(uint32)+  Y * LOD->Pitch);
    *(uint32*)TexelPtr = 0xffffffff;
#endif

    bilinear_sample Sample = BilinearSample(LOD, X, Y);
    v3 Result = SRGBBilinearBlend(Sample, fX, fY).xyz;

    return Result;
}

internal void DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                  loaded_bitmap *Texture, loaded_bitmap *NormalMap,
                                  environment_map *Top,
                                  environment_map *Middle,
                                  environment_map *Bottom,
                                  real32 PixelsToMeters)
{
    BEGIN_TIMED_BLOCK(DrawRectangleSlowly);
    //Note: Premultiply  color up front
    Color.rgb *= Color.a;

    real32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
    real32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);

    real32 XAxisLength = Length(XAxis);
    real32 YAxisLength = Length(YAxis);

    // NOTE: NzScale could be a parameter if we want people to
    // have control over the amount of scaling in the Z direction
    // that the normals appear to have.
    real32 NzScale = 0.5f * (XAxisLength + YAxisLength);

    v2 NxAxis = (YAxisLength / XAxisLength) * XAxis;
    v2 NyAxis = (XAxisLength / YAxisLength) * YAxis;

    uint32 Color32 = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
                      (RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
                      (RoundReal32ToUInt32(Color.g * 255.0f) << 8) |
                      (RoundReal32ToUInt32(Color.b * 255.0f) << 0));

    int WidthMax = Buffer->Width - 1;
    int HeightMax = Buffer->Height - 1;

    real32 InvWidthMax = 1.0f / (real32)WidthMax;
    real32 InvHeightMax = 1.0f / (real32)HeightMax;

    real32 OriginZ = 0.0f;
    real32 OriginY = (Origin + 0.5f * XAxis + 0.5f * YAxis).y;
    real32 FixedCastY = InvHeightMax * OriginY;

    int XMin = WidthMax;
    int XMax = 0;
    int YMin = HeightMax;
    int YMax = 0;

    v2 P[4] = {Origin, Origin + XAxis, Origin + XAxis + YAxis, Origin + YAxis};
    for (int PIndex = 0;
         PIndex < ArrayCount(P);
         ++PIndex)
    {
        v2 TestP = P[PIndex];
        int FloorX = FloorReal32ToInt32(TestP.x);
        int CeilX = CeilReal32ToInt32(TestP.x);
        int FloorY = FloorReal32ToInt32(TestP.y);
        int CeilY = CeilReal32ToInt32(TestP.y);

        if (XMin > FloorX)
        {
            XMin = FloorX;
        }
        if (YMin > FloorY)
        {
            YMin = FloorY;
        }
        if (XMax < CeilX)
        {
            XMax = CeilX;
        }
        if (YMax < CeilY)
        {
            YMax = CeilY;
        }
    }

    if (XMin < 0)
    {
        XMin = 0;
    }
    if (YMin < 0)
    {
        YMin = 0;
    }
    if (XMax > WidthMax)
    {
        XMax = WidthMax;
    }
    if (YMax > HeightMax)
    {
        YMax = HeightMax;
    }

    uint8 *Row = (((uint8 *)Buffer->Memory) +
                  XMin * BITMAP_BYTES_PER_PIXEL +
                  YMin * Buffer->Pitch);
    for (int Y = YMin;
         Y <= YMax;
         ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = XMin;
             X <= XMax;
             ++X)
        {
#if 1
            v2 PixelP = V2i(X, Y);
            v2 d = PixelP - Origin;
            real32 Edge0 = Inner(d, -Perp(XAxis));
            real32 Edge1 = Inner(d - XAxis, -Perp(YAxis));
            real32 Edge2 = Inner(d - XAxis - YAxis, Perp(XAxis));
            real32 Edge3 = Inner(d - YAxis, Perp(YAxis));

            if ((Edge0 < 0) &&
                (Edge1 < 0) &&
                (Edge2 < 0) &&
                (Edge3 < 0))
            {
#if 1
                v2 ScreenSpaceUV = {InvWidthMax * (real32)X, FixedCastY};
                real32 ZDiff = PixelsToMeters * ((real32)Y - OriginY);
#else
                v2 ScreenSpaceUV = {InvWidthMax * (real32)X, InvHeightMax * (real32)Y};
                real32 ZDiff = 0.0f;
#endif

                real32 U = Clamp01(InvXAxisLengthSq * Inner(d, XAxis));
                real32 V = Clamp01(InvYAxisLengthSq * Inner(d, YAxis));

#if 0
                Assert((U >= 0.0f) && (U<=1.0f));
                Assert((V >= 0.0f) && (V<=1.0f));
#endif

                real32 texX = ((U * (real32)(Texture->Width - 2)));
                real32 texY = ((V * (real32)(Texture->Height - 2)));

                int32 TX = (int32)(texX);
                int32 TY = (int32)(texY);

                real32 fx = texX - (real32)TX;
                real32 fy = texY - (real32)TY;

                Assert((TX >= 0.0f) && (TX < Texture->Width));
                Assert((TY >= 0.0f) && (TY < Texture->Height));

                bilinear_sample TexelSample = BilinearSample(Texture, TX, TY);
                v4 Texel = SRGBBilinearBlend(TexelSample, fx, fy);

#if 0
                if (NormalMap)
                {
                    bilinear_sample NormalSample = BilinearSample(NormalMap, TX, TY);

                    v4 NormalA = Unpack4x8(NormalSample.A);
                    v4 NormalB = Unpack4x8(NormalSample.B);
                    v4 NormalC = Unpack4x8(NormalSample.C);
                    v4 NormalD = Unpack4x8(NormalSample.D);

                    v4 Normal = Lerp(Lerp(NormalA, fx, NormalB),
                                     fy,
                                     Lerp(NormalC, fx, NormalD));

                    Normal = UnscaledAndBiasNormal(Normal);
                    //TODO: Do we really need to do this?

                    Normal.xy = Normal.x * NxAxis + Normal.y * NyAxis;
                    Normal.z *= NzScale;
                    Normal.xyz = Normalize(Normal.xyz);

                    //NOTE: The eye vector is always assumed to be [0,0,1]
                    // This is just the simplified version of the reflection -e + 2e^T N N
                    v3 BounceDirection = 2.0f * Normal.z * Normal.xyz;
                    BounceDirection.z -= 1.0f;

                    // TODO: Eventually we need to support two mappings,
                    // one for top-down view (which we don't do now) and one
                    // for sideways, which what's happening here.
                    BounceDirection.z = -BounceDirection.z;

                    environment_map *FarMap = 0;
                    real32 Pz = OriginZ + ZDiff;
                    real32 MapZ = 2.0f;
                    real32 tEnvMap = BounceDirection.y;
                    real32 tFarMap = 0.0f;
                    if (tEnvMap < -0.5f)
                    {
                        FarMap = Bottom;
                        tFarMap = -1.0f - 2.0f * tEnvMap;
                    }
                    else if (tEnvMap > 0.5f)
                    {
                        FarMap = Top;
                        tFarMap = 2.0f * (tEnvMap - 0.5f);
                    }

                    tFarMap *= tFarMap;

                    v3 LightColor = {0, 0, 0}; // SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);
                    if (FarMap)
                    {
                        real32 DistanceFromMapInZ = FarMap->Pz - Pz;
                        v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection, Normal.w, FarMap,
                                                              DistanceFromMapInZ);
                        LightColor = Lerp(LightColor, tFarMap, FarMapColor);
                    }

                    Texel.rgb = Texel.rgb + Texel.a * LightColor;
#if 0
                    //NOTE: Draws the bounce direction
                    Texel.rgb = V3(0.5f, 0.5f, 0.5f) + 0.5f*BounceDirection;
                    Texel.rgb *= Texel.a;
#endif
                }
#endif

                Texel = Hadamard(Texel, Color);
                Texel.r = Clamp01(Texel.r);
                Texel.g = Clamp01(Texel.g);
                Texel.b = Clamp01(Texel.b);

                v4 Dest = {(real32)((*Pixel >> 16) & 0xFF),
                           (real32)((*Pixel >> 8) & 0xFF),
                           (real32)((*Pixel >> 0) & 0xFF),
                           (real32)((*Pixel >> 24) & 0xFF)};

                //NOTE: Go from sRGB to "linear" brightness space
                Dest = SRGB255ToLinear1(Dest);

                v4 Blended = (1.0f - Texel.a) * Dest + Texel;

                //NOTE: Go from "linear" brightness space to sRGB
                v4 Blended255 = Linear1ToSRGB255(Blended);

                *Pixel = (((uint32)(Blended255.a + 0.5f) << 24) |
                          ((uint32)(Blended255.r + 0.5f) << 16) |
                          ((uint32)(Blended255.g + 0.5f) << 8) |
                          ((uint32)(Blended255.b + 0.5f) << 0));
            }
#else
            *Pixel = Color32;
#endif
            ++Pixel;
        }
        Row += Buffer->Pitch;
    }
    END_TIMED_BLOCK(DrawRectangleSlowly);
}

internal void DrawRectangleHopefullyQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                            loaded_bitmap *Texture, real32 PixelsToMeters)
{
    BEGIN_TIMED_BLOCK(DrawRectangleHopefullyQuickly);

    //Note: Premultiply  color up front
    Color.rgb *= Color.a;

    real32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
    real32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);

    real32 XAxisLength = Length(XAxis);
    real32 YAxisLength = Length(YAxis);

    // NOTE: NzScale could be a parameter if we want people to
    // have control over the amount of scaling in the Z direction
    // that the normals appear to have.
    real32 NzScale = 0.5f * (XAxisLength + YAxisLength);

    v2 NxAxis = (YAxisLength / XAxisLength) * XAxis;
    v2 NyAxis = (XAxisLength / YAxisLength) * YAxis;

    //TODO: IMPORTANT: STOP DOIND THIS
    int WidthMax = (Buffer->Width - 1) - 3;
    int HeightMax = (Buffer->Height - 1) - 3;

    real32 InvWidthMax = 1.0f / (real32)WidthMax;
    real32 InvHeightMax = 1.0f / (real32)HeightMax;

    real32 OriginZ = 0.0f;
    real32 OriginY = (Origin + 0.5f * XAxis + 0.5f * YAxis).y;
    real32 FixedCastY = InvHeightMax * OriginY;

    int XMin = WidthMax;
    int XMax = 0;
    int YMin = HeightMax;
    int YMax = 0;

    v2 P[4] = {Origin, Origin + XAxis, Origin + XAxis + YAxis, Origin + YAxis};
    for (int PIndex = 0;
         PIndex < ArrayCount(P);
         ++PIndex)
    {
        v2 TestP = P[PIndex];
        int FloorX = FloorReal32ToInt32(TestP.x);
        int CeilX = CeilReal32ToInt32(TestP.x);
        int FloorY = FloorReal32ToInt32(TestP.y);
        int CeilY = CeilReal32ToInt32(TestP.y);

        if (XMin > FloorX)
        {
            XMin = FloorX;
        }
        if (YMin > FloorY)
        {
            YMin = FloorY;
        }
        if (XMax < CeilX)
        {
            XMax = CeilX;
        }
        if (YMax < CeilY)
        {
            YMax = CeilY;
        }
    }

    if (XMin < 0)
    {
        XMin = 0;
    }
    if (YMin < 0)
    {
        YMin = 0;
    }
    if (XMax > WidthMax)
    {
        XMax = WidthMax;
    }
    if (YMax > HeightMax)
    {
        YMax = HeightMax;
    }

    v2 nXAxis = InvXAxisLengthSq * XAxis;
    v2 nYAxis = InvYAxisLengthSq * YAxis;
    real32 Inv255 = 1.0f / 255.0f;
    __m128 Inv255_4x = _mm_set1_ps(Inv255);
    real32 One255 = 255.0f;
    __m128 One255_4x = _mm_set1_ps(One255);

    __m128 One = _mm_set1_ps(1.0f);
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 Colorr_4x = _mm_set1_ps(Color.r);
    __m128 Colorg_4x = _mm_set1_ps(Color.g);
    __m128 Colorb_4x = _mm_set1_ps(Color.b);
    __m128 Colora_4x = _mm_set1_ps(Color.a);

    __m128 Originx_4x = _mm_set1_ps(Origin.x);
    __m128 Originy_4x = _mm_set1_ps(Origin.y);

    __m128 nXAxisx_4x = _mm_set1_ps(nXAxis.x);
    __m128 nXAxisy_4x = _mm_set1_ps(nXAxis.y);
    __m128 nYAxisx_4x = _mm_set1_ps(nYAxis.x);
    __m128 nYAxisy_4x = _mm_set1_ps(nYAxis.y);

    uint8 *Row = (((uint8 *)Buffer->Memory) +
                  XMin * BITMAP_BYTES_PER_PIXEL +
                  YMin * Buffer->Pitch);
    BEGIN_TIMED_BLOCK(ProcessPixel);
    for (int Y = YMin;
         Y <= YMax;
         ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int XI = XMin;
             XI <= XMax;
             XI += 4)
        {
            __m128 TexelAr = _mm_set1_ps(0.0f);
            __m128 TexelAg = _mm_set1_ps(0.0f);
            __m128 TexelAb = _mm_set1_ps(0.0f);
            __m128 TexelAa = _mm_set1_ps(0.0f);

            __m128 TexelBr = _mm_set1_ps(0.0f);
            __m128 TexelBg = _mm_set1_ps(0.0f);
            __m128 TexelBb = _mm_set1_ps(0.0f);
            __m128 TexelBa = _mm_set1_ps(0.0f);

            __m128 TexelCr = _mm_set1_ps(0.0f);
            __m128 TexelCg = _mm_set1_ps(0.0f);
            __m128 TexelCb = _mm_set1_ps(0.0f);
            __m128 TexelCa = _mm_set1_ps(0.0f);

            __m128 TexelDr = _mm_set1_ps(0.0f);
            __m128 TexelDg = _mm_set1_ps(0.0f);
            __m128 TexelDb = _mm_set1_ps(0.0f);
            __m128 TexelDa = _mm_set1_ps(0.0f);

            __m128 Destr = _mm_set1_ps(0.0f);
            __m128 Destg = _mm_set1_ps(0.0f);
            __m128 Destb = _mm_set1_ps(0.0f);
            __m128 Desta = _mm_set1_ps(0.0f);

            __m128 fX = _mm_set1_ps(0.0f);
            __m128 fY = _mm_set1_ps(0.0f);

            __m128 Blendedr = _mm_set1_ps(0.0f);
            __m128 Blendedg = _mm_set1_ps(0.0f);
            __m128 Blendedb = _mm_set1_ps(0.0f);
            __m128 Blendeda = _mm_set1_ps(0.0f);

            __m128 PixelPx = _mm_set_ps((real32)(XI + 3), (real32)(XI + 2), (real32)(XI + 1), (real32)(XI + 0));
            __m128 PixelPy = _mm_set1_ps((real32)Y);
            __m128 dx = _mm_sub_ps(PixelPx, Originx_4x);
            __m128 dy = _mm_sub_ps(PixelPy, Originy_4x);

            __m128 U = _mm_add_ps(_mm_mul_ps(dx, nXAxisx_4x), _mm_mul_ps(dy, nXAxisy_4x));
            __m128 V = _mm_add_ps(_mm_mul_ps(dx, nYAxisx_4x), _mm_mul_ps(dy, nYAxisy_4x));

            bool32 ShouldFill[4];

#define mmSquare(a) _mm_mul_ps(a, a)
#define M(a, i) ((float *)&(a))[i]

            for (int I = 0;
                 I < 4;
                 ++I)
            {
                ShouldFill[I] = ((M(U, I) >= 0.0f) &&
                                 (M(U, I) <= 1.0f) &&
                                 (M(V, I) >= 0.0f) &&
                                 (M(V, I) <= 1.0f));
                if (ShouldFill[I])
                {

                    real32 texX = ((M(U, I) * (real32)(Texture->Width - 2)));
                    real32 texY = ((M(V, I) * (real32)(Texture->Height - 2)));

                    int32 TX = (int32)(texX);
                    int32 TY = (int32)(texY);

                    M(fX, I) = texX - (real32)TX;
                    M(fY, I) = texY - (real32)TY;

                    Assert((TX >= 0.0f) && (TX < Texture->Width));
                    Assert((TY >= 0.0f) && (TY < Texture->Height));

                    //bilinear sample
                    uint8 *TexelPtr = (((uint8 *)Texture->Memory) + TX * BITMAP_BYTES_PER_PIXEL + TY * Texture->Pitch);
                    uint32 SampleA = *(uint32 *)(TexelPtr);
                    uint32 SampleB = *(uint32 *)(TexelPtr + BITMAP_BYTES_PER_PIXEL);
                    uint32 SampleC = *(uint32 *)(TexelPtr + Texture->Pitch);
                    uint32 SampleD = *(uint32 *)(TexelPtr + Texture->Pitch + BITMAP_BYTES_PER_PIXEL);

                    //v4 Texel = SRGBBilinearBlend(TexelSample, fx, fy);
                    M(TexelAr, I) = (real32)((SampleA >> 16) & 0xFF);
                    M(TexelAg, I) = (real32)((SampleA >> 8) & 0xFF);
                    M(TexelAb, I) = (real32)((SampleA >> 0) & 0xFF);
                    M(TexelAa, I) = (real32)((SampleA >> 24) & 0xFF);

                    M(TexelBr, I) = (real32)((SampleB >> 16) & 0xFF);
                    M(TexelBg, I) = (real32)((SampleB >> 8) & 0xFF);
                    M(TexelBb, I) = (real32)((SampleB >> 0) & 0xFF);
                    M(TexelBa, I) = (real32)((SampleB >> 24) & 0xFF);

                    M(TexelCr, I) = (real32)((SampleC >> 16) & 0xFF);
                    M(TexelCg, I) = (real32)((SampleC >> 8) & 0xFF);
                    M(TexelCb, I) = (real32)((SampleC >> 0) & 0xFF);
                    M(TexelCa, I) = (real32)((SampleC >> 24) & 0xFF);

                    M(TexelDr, I) = (real32)((SampleD >> 16) & 0xFF);
                    M(TexelDg, I) = (real32)((SampleD >> 8) & 0xFF);
                    M(TexelDb, I) = (real32)((SampleD >> 0) & 0xFF);
                    M(TexelDa, I) = (real32)((SampleD >> 24) & 0xFF);

                    //NOTE: Load destination
                    M(Destr, I) = (real32)((*(Pixel + I) >> 16) & 0xFF);
                    M(Destg, I) = (real32)((*(Pixel + I) >> 8) & 0xFF);
                    M(Destb, I) = (real32)((*(Pixel + I) >> 0) & 0xFF);
                    M(Desta, I) = (real32)((*(Pixel + I) >> 24) & 0xFF);
                }
            }

            //NOTE: Convert texture from sRGB to "linear" brightness space
            TexelAr = mmSquare(_mm_mul_ps(Inv255_4x, TexelAr));
            TexelAg = mmSquare(_mm_mul_ps(Inv255_4x, TexelAg));
            TexelAb = mmSquare(_mm_mul_ps(Inv255_4x, TexelAb));
            TexelAa = _mm_mul_ps(Inv255_4x, TexelAa);

            TexelBr = mmSquare(_mm_mul_ps(Inv255_4x, TexelBr));
            TexelBg = mmSquare(_mm_mul_ps(Inv255_4x, TexelBg));
            TexelBb = mmSquare(_mm_mul_ps(Inv255_4x, TexelBb));
            TexelBa = _mm_mul_ps(Inv255_4x, TexelBa);

            TexelCr = mmSquare(_mm_mul_ps(Inv255_4x, TexelCr));
            TexelCg = mmSquare(_mm_mul_ps(Inv255_4x, TexelCg));
            TexelCb = mmSquare(_mm_mul_ps(Inv255_4x, TexelCb));
            TexelCa = _mm_mul_ps(Inv255_4x, TexelCa);

            TexelDr = mmSquare(_mm_mul_ps(Inv255_4x, TexelDr));
            TexelDg = mmSquare(_mm_mul_ps(Inv255_4x, TexelDg));
            TexelDb = mmSquare(_mm_mul_ps(Inv255_4x, TexelDb));
            TexelDa = _mm_mul_ps(Inv255_4x, TexelDa);

            //NOTE: Bilinear texture Blend
            __m128 ifX = _mm_sub_ps(One, fX);
            __m128 ifY = _mm_sub_ps(One, fY);

            __m128 l0 = _mm_mul_ps(ifY, ifX);
            __m128 l1 = _mm_mul_ps(ifY, fX);
            __m128 l2 = _mm_mul_ps(fY, ifX);
            __m128 l3 = _mm_mul_ps(fY, fX);

            __m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAr), _mm_mul_ps(l1, TexelBr)), _mm_mul_ps(l2, TexelCr)), _mm_mul_ps(l3, TexelDr));
            __m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAg), _mm_mul_ps(l1, TexelBg)), _mm_mul_ps(l2, TexelCg)), _mm_mul_ps(l3, TexelDg));
            __m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAb), _mm_mul_ps(l1, TexelBb)), _mm_mul_ps(l2, TexelCb)), _mm_mul_ps(l3, TexelDb));
            __m128 Texela = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAa), _mm_mul_ps(l1, TexelBa)), _mm_mul_ps(l2, TexelCa)), _mm_mul_ps(l3, TexelDa));

            Texelr = _mm_mul_ps(Texelr, Colorr_4x);
            Texelg = _mm_mul_ps(Texelg, Colorg_4x);
            Texelb = _mm_mul_ps(Texelb, Colorb_4x);
            Texela = _mm_mul_ps(Texela, Colora_4x);

            Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), One);
            Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), One);
            Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), One);

            //NOTE: Go from sRGB to "linear" brightness space
            Destr = mmSquare(_mm_mul_ps(Inv255_4x, Destr));
            Destg = mmSquare(_mm_mul_ps(Inv255_4x, Destg));
            Destb = mmSquare(_mm_mul_ps(Inv255_4x, Destb));
            Desta = _mm_mul_ps(Inv255_4x, Desta);

            //NOTE: Destination blend
            __m128 InvTexelA = _mm_sub_ps(One, Texela);
            Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
            Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
            Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
            Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);

            //NOTE: Go from "linear" brightness space to sRGB
            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
            Blendeda = _mm_mul_ps(One255_4x, Blendeda);

            for (int I = 0;
                 I < 4;
                 ++I)
            {
                if (ShouldFill[I])
                {
                    //NOTE: Repack
                    *(Pixel + I) = (((uint32)(M(Blendeda, I) + 0.5f) << 24) |
                                    ((uint32)(M(Blendedr, I) + 0.5f) << 16) |
                                    ((uint32)(M(Blendedg, I) + 0.5f) << 8) |
                                    ((uint32)(M(Blendedb, I) + 0.5f) << 0));
                }
            }

            Pixel += 4;
        }
        Row += Buffer->Pitch;
    }
    END_TIMED_BLOCK_COUNTED(ProcessPixel, (XMax - XMin + 1) * (YMax - YMin + 1));
    END_TIMED_BLOCK(DrawRectangleHopefullyQuickly);
}

internal void DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
                         real32 RealX, real32 RealY, real32 CAlpha = 1.0f)
{
    int32 MinX = RoundReal32ToInt32(RealX);
    int32 MinY = RoundReal32ToInt32(RealY);
    int32 MaxX = MinX + Bitmap->Width;
    int32 MaxY = MinY + Bitmap->Height;

    int32 SourceOffsetX = 0;
    if (MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }

    int32 SourceOffsetY = 0;
    if (MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8 *SourceRow = (uint8 *)Bitmap->Memory + SourceOffsetY * Bitmap->Pitch + BITMAP_BYTES_PER_PIXEL * SourceOffsetX;
    uint8 *DestRow = (((uint8 *)Buffer->Memory) +
                      MinX * BITMAP_BYTES_PER_PIXEL +
                      MinY * Buffer->Pitch);
    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Dest = (uint32 *)DestRow;
        uint32 *Source = (uint32 *)SourceRow;
        for (int X = MinX; X < MaxX; ++X)
        {
            v4 Texel = {(real32)((*Source >> 16) & 0xFF),
                        (real32)((*Source >> 8) & 0xFF),
                        (real32)((*Source >> 0) & 0xFF),
                        (real32)((*Source >> 24) & 0xFF)};

            Texel = SRGB255ToLinear1(Texel);
            Texel *= CAlpha;

            v4 D = {(real32)((*Dest >> 16) & 0xFF),
                    (real32)((*Dest >> 8) & 0xFF),
                    (real32)((*Dest >> 0) & 0xFF),
                    (real32)((*Dest >> 24) & 0xFF)};

            D = SRGB255ToLinear1(D);

            v4 Result = (1.0f - Texel.a) * D + Texel;

            Result = Linear1ToSRGB255(Result);

            *Dest = (((uint32)(Result.a + 0.5f) << 24) |
                     ((uint32)(Result.r + 0.5f) << 16) |
                     ((uint32)(Result.g + 0.5f) << 8) |
                     ((uint32)(Result.b + 0.5f) << 0));

            ++Dest;
            ++Source;
        }
        DestRow += Buffer->Pitch;
        SourceRow += Bitmap->Pitch;
    }
}

internal void ChangeSaturation(loaded_bitmap *Buffer, real32 Level)
{
    uint8 *DestRow = (uint8 *)Buffer->Memory;
    for (int Y = 0;
         Y < Buffer->Height;
         ++Y)
    {
        uint32 *Dest = (uint32 *)DestRow;
        for (int X = 0;
             X < Buffer->Width;
             ++X)
        {
            v4 D = {(real32)((*Dest >> 16) & 0xFF),
                    (real32)((*Dest >> 8) & 0xFF),
                    (real32)((*Dest >> 0) & 0xFF),
                    (real32)((*Dest >> 24) & 0xFF)};

            D = SRGB255ToLinear1(D);

            real32 Avg = (1.0f / 3.0f) * (D.r + D.g + D.b);
            v3 Delta = V3(D.r - Avg, D.g - Avg, D.b - Avg);

            v4 Result = V4(V3(Avg, Avg, Avg) + Level * Delta, D.a);

            Result = Linear1ToSRGB255(Result);

            *Dest = (((uint32)(Result.a + 0.5f) << 24) |
                     ((uint32)(Result.r + 0.5f) << 16) |
                     ((uint32)(Result.g + 0.5f) << 8) |
                     ((uint32)(Result.b + 0.5f) << 0));

            ++Dest;
        }

        DestRow += Buffer->Pitch;
    }
}

inline void DrawRectangleOutline(loaded_bitmap *Buffer, v2 vMin, v2 vMax,
                                 v3 Color, real32 T = 2.0f)
{

    //Top and bottom
    DrawRectangle(Buffer, V2(vMin.x - T, vMin.y - T), V2(vMax.x + T, vMin.y + T), V4(Color, 1.0f));
    DrawRectangle(Buffer, V2(vMin.x - T, vMax.y - T), V2(vMax.x + T, vMax.y + T), V4(Color, 1.0f));

    //left and right
    DrawRectangle(Buffer, V2(vMin.x - T, vMin.y - T), V2(vMin.x + T, vMax.y + T), V4(Color, 1.0f));
    DrawRectangle(Buffer, V2(vMax.x - T, vMin.y - T), V2(vMax.x + T, vMax.y + T), V4(Color, 1.0f));
}

internal void DrawMatte(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
                        real32 RealX, real32 RealY, real32 CAlpha = 1.0f)
{
    int32 MinX = RoundReal32ToInt32(RealX);
    int32 MinY = RoundReal32ToInt32(RealY);
    int32 MaxX = MinX + Bitmap->Width;
    int32 MaxY = MinY + Bitmap->Height;

    int32 SourceOffsetX = 0;
    if (MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }

    int32 SourceOffsetY = 0;
    if (MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8 *SourceRow = (uint8 *)Bitmap->Memory + SourceOffsetY * Bitmap->Pitch + BITMAP_BYTES_PER_PIXEL * SourceOffsetX;
    uint8 *DestRow = (((uint8 *)Buffer->Memory) +
                      MinX * BITMAP_BYTES_PER_PIXEL +
                      MinY * Buffer->Pitch);
    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Dest = (uint32 *)DestRow;
        uint32 *Source = (uint32 *)SourceRow;
        for (int X = MinX; X < MaxX; ++X)
        {

            real32 SA = CAlpha * (real32)((*Source >> 24) & 0xFF);
            real32 SR = CAlpha * (real32)((*Source >> 16) & 0xFF);
            real32 SG = CAlpha * (real32)((*Source >> 8) & 0xFF);
            real32 SB = CAlpha * (real32)((*Source >> 0) & 0xFF);
            real32 RSA = (SA / 255.0f) * CAlpha;

            real32 DA = (real32)((*Dest >> 24) & 0xFF);
            real32 DR = (real32)((*Dest >> 16) & 0xFF);
            real32 DG = (real32)((*Dest >> 8) & 0xFF);
            real32 DB = (real32)((*Dest >> 0) & 0xFF);
            real32 RDA = (DA / 255.0f);

            real32 InvRSA = (1.0f - RSA);
            real32 A = InvRSA * DA;
            real32 R = InvRSA * DR;
            real32 G = InvRSA * DG;
            real32 B = InvRSA * DB;

            *Dest = (((uint32)(A + 0.5f) << 24) |
                     ((uint32)(R + 0.5f) << 16) |
                     ((uint32)(G + 0.5f) << 8) |
                     ((uint32)(B + 0.5f) << 0));

            ++Dest;
            ++Source;
        }
        DestRow += Buffer->Pitch;
        SourceRow += Bitmap->Pitch;
    }
}

struct entity_basis_p_result
{
    v2 P;
    real32 Scale;
    bool32 Valid;
};

inline entity_basis_p_result GetRenderEntityBasisP(render_group *RenderGroup, render_entity_basis *EntityBasis,
                                                   v2 ScreenDim)
{
    v2 ScreenCenter = 0.5f * ScreenDim;

    entity_basis_p_result Result = {};

    v3 EntityBaseP = EntityBasis->Basis->P;

    real32 DistanceToPZ = (RenderGroup->RenderCamera.DistanceAboveTarget - EntityBaseP.z);
    real32 NearClipPlane = 0.2f;

    v3 RawXY = V3(EntityBaseP.xy + EntityBasis->Offset.xy, 1.0f);

    if (DistanceToPZ > NearClipPlane)
    {
        v3 ProjectedXY = (1.0f / DistanceToPZ) * RenderGroup->RenderCamera.FocalLength * RawXY;
        Result.P = ScreenCenter + RenderGroup->MetersToPixels * ProjectedXY.xy;
        Result.Scale = RenderGroup->MetersToPixels * ProjectedXY.z;
        Result.Valid = true;
    }

    return Result;
}

internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
{
    BEGIN_TIMED_BLOCK(RenderGroupToOutput);
    v2 ScreenDim = {(real32)OutputTarget->Width,
                    (real32)OutputTarget->Height};

    real32 PixelsToMeters = 1.0f / RenderGroup->MetersToPixels;

    for (uint32 BaseAddress = 0;
         BaseAddress < RenderGroup->PushBufferSize;)
    {
        render_group_entry_header *Header = (render_group_entry_header *)(RenderGroup->PushBufferBase + BaseAddress);
        BaseAddress += sizeof(*Header);
        void *Data = ((uint8 *)Header) + sizeof(*Header);

        switch (Header->Type)
        {
        case RenderGroupEntryType_render_entry_clear:
        {
            render_entry_clear *Entry = (render_entry_clear *)Data;

            DrawRectangle(OutputTarget, V2(0.0f, 0.0f),
                          V2((real32)OutputTarget->Width, (real32)OutputTarget->Height),
                          Entry->Color);

            BaseAddress += sizeof(render_entry_clear);
        }
        break;

        case RenderGroupEntryType_render_entry_bitmap:
        {
            render_entry_bitmap *Entry = (render_entry_bitmap *)Data;

            entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
            Assert(Entry->Bitmap);
#if 0
            DrawBitmap(OutputTarget, Entry->Bitmap, P.x, P.y, Entry->Color.a);
#else
            DrawRectangleHopefullyQuickly(OutputTarget, Basis.P,
                                          Basis.Scale * V2(Entry->Size.x, 0.0f),
                                          Basis.Scale * V2(0.0f, Entry->Size.y), Entry->Color,
                                          Entry->Bitmap, PixelsToMeters);
#endif
            BaseAddress += sizeof(*Entry);
        }
        break;

        case RenderGroupEntryType_render_entry_rectangle:
        {
            render_entry_rectangle *Entry = (render_entry_rectangle *)Data;

            entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
            DrawRectangle(OutputTarget, Basis.P, Basis.P + Basis.Scale * Entry->Dim, Entry->Color);

            BaseAddress += sizeof(*Entry);
        }
        break;

        case RenderGroupEntryType_render_entry_coordinate_system:
        {
            render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;

            v2 vMax = (Entry->Origin + Entry->XAxis + Entry->YAxis);
            DrawRectangleSlowly(OutputTarget,
                                Entry->Origin,
                                Entry->XAxis,
                                Entry->YAxis,
                                Entry->Color,
                                Entry->Texture,
                                Entry->NormalMap,
                                Entry->Top,
                                Entry->Middle,
                                Entry->Bottom,
                                PixelsToMeters);

            v4 Color = {1, 1, 0, 1};
            v2 Dim = {2, 2};
            v2 P = Entry->Origin;
            DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);

            P = Entry->Origin + Entry->XAxis;
            DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);

            P = Entry->Origin + Entry->YAxis;
            DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);

            DrawRectangle(OutputTarget, vMax - Dim, vMax + Dim, Color);

#if 0
            for(uint32 PIndex = 0;
                PIndex < ArrayCount(Entry->Points);
                ++PIndex)
            {
                P = Entry->Points[PIndex];
                P = Entry->Origin + P.x * Entry->XAxis + P.y * Entry->YAxis;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Entry->Color.r, Entry->Color.g, Entry->Color.b);

            }
#endif
            BaseAddress += sizeof(*Entry);
        }
        break;

            INVALID_DEFAULT_CASE;
        }
    }
    END_TIMED_BLOCK(RenderGroupToOutput);
}

internal render_group *AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize,
                                           uint32 ResolutionPixelsX, uint32 ResolutionPixelsY)
{
    render_group *Result = PushStruct(Arena, render_group);
    Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);

    Result->DefaultBasis = PushStruct(Arena, render_basis);
    Result->DefaultBasis->P = V3(0, 0, 0);

    Result->MaxPushBufferSize = MaxPushBufferSize;
    Result->PushBufferSize = 0;

    real32 WidthOfMonitor = 0.635f;        // NOTE: Horizontal measurement of monitor in meters
    Result->GameCamera.FocalLength = 0.6f; // NOTE: Mwrwea the person is sitting from their monitor
    Result->GameCamera.DistanceAboveTarget = 9.0f;
    Result->RenderCamera = Result->GameCamera;
    //Result->RenderCamera.DistanceAboveTarget = 50.0f;

    Result->GlobalAlpha = 1.0f;

    // TODO: Need to adjust this based on buffer size
    Result->MetersToPixels = (real32)ResolutionPixelsX * WidthOfMonitor;

    real32 PixelsToMeters = 1.0f / Result->MetersToPixels;
    Result->MonitorHalfDimInMeters = {0.5f * ResolutionPixelsX * PixelsToMeters,
                                      0.5f * ResolutionPixelsY * PixelsToMeters};

    return Result;
}

#define PushRenderElement(Group, Type) (Type *)PushRenderElement_(Group, sizeof(Type), RenderGroupEntryType_##Type)
inline void *PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type)
{
    void *Result = 0;

    Size += sizeof(render_group_entry_header);

    if ((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
        render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
        Header->Type = Type;
        Result = (render_group_entry_header *)(((uint8 *)Header) + sizeof(*Header));
        Group->PushBufferSize += Size;
    }
    else
    {
        INVALID_CODE_PATH;
    }

    return Result;
}

inline void PushBitmap(render_group *Group, loaded_bitmap *Bitmap, real32 Height, v3 Offset,
                       v4 Color = V4(1, 1, 1, 1))
{
    render_entry_bitmap *Piece = PushRenderElement(Group, render_entry_bitmap);
    if (Piece)
    {
        Piece->EntityBasis.Basis = Group->DefaultBasis;
        Piece->Bitmap = Bitmap;
        v2 Size = V2(Height * Bitmap->WidthOverHeight, Height);
        v2 Align = Hadamard(Bitmap->AlignPercentage, Size);
        Piece->EntityBasis.Offset = Offset - V3(Align, 0);
        Piece->Color = Group->GlobalAlpha * Color;
        Piece->Size = Size;
    }
}

inline void PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color = V4(1, 1, 1, 1))
{
    render_entry_rectangle *Piece = PushRenderElement(Group, render_entry_rectangle);
    if (Piece)
    {
        Piece->EntityBasis.Basis = Group->DefaultBasis;
        Piece->EntityBasis.Offset = (Offset - V3(0.5f * Dim, 0));
        Piece->Color = Color;
        Piece->Dim = Dim;
    }
}

inline void PushRectOutline(render_group *Group, v3 Offset, v2 Dim, v4 Color)
{
    real32 Thickness = 0.1f;

    //Top and bottom
    PushRect(Group, Offset - V3(0, 0.5f * Dim.y, 0), V2(Dim.x, Thickness), Color);
    PushRect(Group, Offset + V3(0, 0.5f * Dim.y, 0), V2(Dim.x, Thickness), Color);

    //left and right
    PushRect(Group, Offset - V3(0.5f * Dim.x, 0, 0), V2(Thickness, Dim.y), Color);
    PushRect(Group, Offset + V3(0.5f * Dim.x, 0, 0), V2(Thickness, Dim.y), Color);
}

inline void Clear(render_group *Group, v4 Color)
{
    render_entry_clear *Entry = PushRenderElement(Group, render_entry_clear);
    if (Entry)
    {
        Entry->Color = Color;
    }
}

inline render_entry_coordinate_system *CoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                                        loaded_bitmap *Texture, loaded_bitmap *NormalMap,
                                                        environment_map *Top, environment_map *Middle, environment_map *Bottom)
{
    render_entry_coordinate_system *Entry = PushRenderElement(Group, render_entry_coordinate_system);
    if (Entry)
    {
        Entry->Origin = Origin;
        Entry->XAxis = XAxis;
        Entry->YAxis = YAxis;
        Entry->Color = Color;
        Entry->Texture = Texture;
        Entry->NormalMap = NormalMap;
        Entry->Top = Top;
        Entry->Middle = Middle;
        Entry->Bottom = Bottom;
    }

    return Entry;
}

inline v2 Unproject(render_group *Group, v2 ProjectedXY, real32 AtDistanceFromCamera)
{
    v2 WorldXY = (AtDistanceFromCamera / Group->GameCamera.FocalLength) * ProjectedXY;

    return WorldXY;
}

inline rectangle2 GetCameraRectangleAtDistance(render_group *Group, real32 DistanceFromCamera)
{
    v2 RawXY = Unproject(Group, Group->MonitorHalfDimInMeters, DistanceFromCamera);

    rectangle2 Result = RectCenterHalfDim(V2(0, 0), RawXY);

    return Result;
}

inline rectangle2 GetCameraRectangleAtTarget(render_group *Group)
{
    rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->GameCamera.DistanceAboveTarget);
    return Result;
}
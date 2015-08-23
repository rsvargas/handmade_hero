#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

/*
NOTE:

HANDMADE_INTERNAL
0 = Build for public release
1 = Build for developer only

HANDMADE_SLOW
0 = No slow code allowed
1 = Slow code allowed!
*/

#include "handmade_platform.h"

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if HANDMADE_SLOW == 1
#define ASSERT(X) if(!(X)) { *(int*)0 = 0;}
#else
#define ASSERT(X)
#endif

#define KILOBYTES(V) ((V)*1024LL)
#define MEGABYTES(V) (KILOBYTES(V)*1024LL)
#define GIGABYTES(V) (MEGABYTES(V)*1024LL)
#define TERABYTES(V) (GIGABYTES(V)*1024LL)

#define ARRAY_COUNT(A) (sizeof(A)/sizeof((A)[0]))

inline uint32 SafeTruncateUInt64(uint64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    uint32 Result = (uint32)value;
    return Result;
}

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
    ASSERT(ControllerIndex < ARRAY_COUNT(Input->Controllers));
    return &Input->Controllers[ControllerIndex];
}

//
//
//

struct canonical_position
{
    int32 TileMapX;
    int32 TileMapY;

    int32 TileX;
    int32 TileY;

    //NOTE: X and Y relative to the tile we are in in this tilemap
    real32 X;
    real32 Y;
};

struct raw_position
{
    int32 TileMapX;
    int32 TileMapY;

    //NOTE: X and Y relative to this tilemap
    real32 TileRelX;
    real32 TileRelY;
};


struct tile_map
{
    uint32* Tiles;
};

struct world
{
    real32 TileSideInMeters;
    int32 TileSizeInPixels;
    int32 CountX;
    int32 CountY;

    real32 UpperLeftX;
    real32 UpperLeftY;

    int32 TileMapCountX;
    int32 TileMapCountY;
    tile_map *TileMaps;
};

struct game_state
{
    int32 PlayerTileMapX;
    int32 PlayerTileMapY;

    real32 PlayerX;
    real32 PlayerY;
};





#endif// HANDMADE_H
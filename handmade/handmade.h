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

#include "handmade_intrinsics.h"
#include "handmade_tile.h"

struct memory_arena
{
    memory_index Size;
    uint8* Base;
    memory_index Used;
};

internal void InitializeArena(memory_arena* Arena, memory_index Size, uint8* Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, Type) (Type*)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type*)PushSize_(Arena, (Count)*sizeof(Type))
internal void* PushSize_(memory_arena* Arena, memory_index Size)
{
    ASSERT((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return Result;
}


struct world
{
    tile_map* TileMap;
};

struct game_state
{
    memory_arena WorldArena;
    world * World;
    tile_map_position PlayerP;

    uint32* PixelPointer;
};

#endif// HANDMADE_H

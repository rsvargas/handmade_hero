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

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

//
//
//

struct memory_arena
{
    memory_index Size;
    uint8* Base;
    memory_index Used;

    int32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};


inline void InitializeArena(memory_arena* Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (uint8*)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

inline memory_index GetAlignmentOffset(memory_arena* Arena, memory_index Alignment)
{
    memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment-1;

    memory_index AlignmentOffset = 0;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }
    return AlignmentOffset;
}

inline memory_index GetArenaSizeRemaining(memory_arena* Arena, memory_index Alignment = 4)
{
    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment) );
    
    return Result;
}

#define PushStruct(Arena, Type, ...) (Type*)PushSize_(Arena, sizeof(Type), ## __VA_ARGS__ )
#define PushArray(Arena, Count, Type, ...) (Type*)PushSize_(Arena, (Count)*sizeof(Type), ## __VA_ARGS__ )
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__  )
inline void* PushSize_(memory_arena* Arena, memory_index SizeInit, memory_index Alignment = 4)
{
    memory_index Size = SizeInit;
    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;

    Assert((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;

    Assert(Size >= SizeInit);
    return Result;
}

inline temporary_memory BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.Arena = Arena;
    Result.Used = Arena->Used;
    ++Arena->TempCount;

    return Result;

}

inline void EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    Assert(Arena->Used >= TempMem.Used);
    Arena->Used = TempMem.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline void CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline void SubArena( memory_arena* Result, memory_arena * Arena, memory_index Size, 
                      memory_index Alignment = 16)
{
    Result->Size = Size;
    Result->Base = (uint8*)PushSize(Arena, Size, Alignment);
    Result->Used = 0;
    Result->TempCount = 0;
}


#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void ZeroSize(memory_index Size, void* Ptr)
{
    uint8 *Byte = (uint8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_world.h"
#include "handmade_sim_region.h"
#include "handmade_entity.h"
#include "handmade_render_group.h"
#include "handmade_asset.h"



struct low_entity
{
    world_position P;
    sim_entity Sim;
};

struct controlled_hero
{
    uint32 EntityIndex;
    v2 ddP;
    v2 dSword;
    real32 dZ;
};

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32 StorageIndexA;
    uint32 StorageIndexB;

    pairwise_collision_rule *NextInHash;
};
struct game_state;
internal void ClearCollisionRulesFor(game_state *GameState, uint32 StorageIndex);
internal void AddCollisionRule(game_state *GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool32 CanCollide);


struct ground_buffer
{
    //Note: An invalid P tells us that this ground_buffer has not been filled
    world_position P;
    loaded_bitmap Bitmap;
};

struct game_state
{
    bool32 IsInitialized;

    memory_arena WorldArena;
    world * World;

    real32 TypicalFloorHeight;

    uint32 CameraFollowingEntityIndex;
    world_position CameraP;

    controlled_hero ControlledHeroes[ArrayCount(((game_input*)0)->Controllers)];

    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    //TODO: Must be power of two
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
    sim_entity_collision_volume_group *SwordCollision;
    sim_entity_collision_volume_group *StairCollision;
    sim_entity_collision_volume_group *PlayerCollision;
    sim_entity_collision_volume_group *MonstarCollision;
    sim_entity_collision_volume_group *WallCollision;
    sim_entity_collision_volume_group *FamiliarCollision;
    sim_entity_collision_volume_group *StandardRoomCollision;

    real32 Time;

    loaded_bitmap TestDiffuse; //TODO: fill this guy with gray
    loaded_bitmap TestNormal;
};

struct task_with_memory
{
    bool32 BeingUsed;
    memory_arena Arena;

    temporary_memory MemoryFlush;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TranArena;

    task_with_memory Tasks[4];

    game_assets *Assets;

    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;
    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;

    uint32 EnvMapWidth;
    uint32 EnvMapHeight;
    //NOTE: 0 - bottom, 1 is middle, 2 is top;
    environment_map EnvMaps[3];

};

inline low_entity* GetLowEntity(game_state *GameState, uint32 Index)
{
    low_entity* Result = 0;

    if ((Index > 0) && (Index < GameState->LowEntityCount))
    {
        Result = GameState->LowEntities + Index;
    }

    return Result;
}

global_variable platform_add_entry *PlatformAddEntry;
global_variable platform_complete_all_work *PlatformCompleteAllWork;
global_variable debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;

internal task_with_memory *BeginTaskWithMemory(transient_state *TranState);
internal void EndTaskWithMemory(task_with_memory* Task);


#endif// HANDMADE_H

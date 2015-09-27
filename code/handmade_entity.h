#pragma once

#define InvalidP V2(100000.0f, 100000.0f)

inline bool32 IsSet(sim_entity *Entity, uint32 Flag)
{
    bool32 Result = (Entity->Flags & Flag);

    return Result;
}

inline void AddFlag(sim_entity *Entity, uint32 Flag)
{
    Entity->Flags |= Flag;
}

inline void ClearFlag(sim_entity *Entity, uint32 Flag)
{
    Entity->Flags &= ~Flag;
}

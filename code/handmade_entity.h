#pragma once

#define InvalidP V3(100000.0f, 100000.0f, 100000.0f)

inline bool32 IsSet(sim_entity *Entity, uint32 Flag)
{
    bool32 Result = (Entity->Flags & Flag);

    return Result;
}

inline void AddFlags(sim_entity *Entity, uint32 Flag)
{
    Entity->Flags |= Flag;
}

inline void ClearFlags(sim_entity *Entity, uint32 Flag)
{
    Entity->Flags &= ~Flag;
}

inline void MakeEntityNonSpatial(sim_entity *Entity)
{
    AddFlags(Entity, EntityFlag_Nonspatial);
    Entity->P = InvalidP;
}

inline void MakeEntitySpatial(sim_entity *Entity, v3 P, v3 dP)
{
    ClearFlags(Entity, EntityFlag_Nonspatial);
    Entity->P = P;
    Entity->dP = dP;
}

inline v3 GetEntityGroundPoint(sim_entity *Entity)
{
    v3 Result = Entity->P + V3(0, 0, -0.5f*Entity->Dim.Z);

    return Result;
}



inline real32 GetStairGround(sim_entity *Entity, v3 AtGroundPoint)
{
    Assert(Entity->Type == EntityType_Stairwell);
    rectangle3 RegionRect = RectCenterDim(Entity->P, Entity->Dim);
    v3 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint));
    real32 Ground = RegionRect.Min.Z + Bary.Y * Entity->WalkableHeight;

    return Ground;

}


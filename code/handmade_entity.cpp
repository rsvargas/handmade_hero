inline move_spec DefaultMoveSpec()
{
    move_spec Result;
    Result.UnitMaxAccelVector = false;
    Result.Speed = 1.0f;
    Result.Drag = 0.0f;

    return Result;
}

inline void UpdateFamiliar(sim_region *SimRegion, sim_entity *Entity, real32 dt)
{
    sim_entity *ClosestHero = 0;
    real32 ClosestHeroDSq = Square(10.0f); //NOTE: Ten meter maximum Search

    sim_entity *TestEntity = SimRegion->Entities;
    for (uint32 TestEntityIndex = 1;
        TestEntityIndex < SimRegion->EntityCount;
        ++TestEntityIndex)
    {
        if (TestEntity->Type == EntityType_Hero)
        {
            real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
            if (TestEntity->Type == EntityType_Hero)
            {
                TestDSq *= 0.5f;
            }
            if (ClosestHeroDSq > TestDSq)
            {
                ClosestHero = TestEntity;
                ClosestHeroDSq = TestDSq;
            }
        }
    }

    v2 ddP = {};
    if (ClosestHero && ClosestHeroDSq > Square(3.0f) )
    {
        real32 Acceleration = 0.5f;
        real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
        ddP = OneOverLength *(ClosestHero->P - Entity->P);
    }


    move_spec MoveSpec = DefaultMoveSpec();
    MoveSpec.UnitMaxAccelVector = true;
    MoveSpec.Speed = 50.0f;
    MoveSpec.Drag = 8.0f;
    MoveEntity(SimRegion, Entity, dt, &MoveSpec, ddP);
}

inline void UpdateMonster(sim_region *SimRegion, sim_entity *Entity, real32 dt)
{
}

inline void UpdateSword(sim_region *SimRegion, sim_entity *Entity, real32 dt)
{
    move_spec MoveSpec = DefaultMoveSpec();
    MoveSpec.UnitMaxAccelVector = false;
    MoveSpec.Speed = 0.0f;
    MoveSpec.Drag = 0.0f;

    v2 OldP = Entity->P;
    MoveEntity(SimRegion, Entity, dt, &MoveSpec, V2(0, 0));
    real32 DistanceTraveled = Length(Entity->P - OldP);

    Entity->DistanceRemaining -= DistanceTraveled;
    if(Entity->DistanceRemaining < 0.0f)
    {
        Assert(!"need to make entities to be able no th be there!");
    }
}


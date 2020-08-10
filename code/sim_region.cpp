internal sim_entity_hash *
GetHashFromStorageIndex(sim_region *SimRegion, u32 StorageIndex) 
{
    sim_entity_hash *Result = 0;
    u32 HashValue = StorageIndex;
    for (u32 Offset = 0; Offset < ArrayCount(SimRegion->Hash); ++Offset)
    {
        u32 HashMask = ArrayCount(SimRegion->Hash) - 1;
        u32 HashIndex = (HashValue + Offset) & HashMask;
        sim_entity_hash *Entry = SimRegion->Hash + HashIndex;
        if ((Entry->Index == StorageIndex) || (Entry->Ptr == 0))
        {
            Result = Entry;
            break;
        }
    }
    return(Result);
}

internal v2
MapIntoSimSpace(sim_region *SimRegion, entity *Entity)
{
    v2 Result = SubtractPosition(SimRegion->TileMap, Entity->P, SimRegion->Origin);

    return(Result);
}

internal sim_entity *
AddEntityRaw(sim_region *SimRegion, entity *StoredEntity)
{
    Assert(StoredEntity);
    
    sim_entity *SimEntity = 0;
    u32 StorageIndex = StoredEntity->Sim.StorageIndex;
    sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
    if(Entry->Ptr == 0)
    {
        if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
        {
           SimEntity = SimRegion->Entities + SimRegion->EntityCount++;
           *SimEntity = StoredEntity->Sim;
           
           Entry->Ptr = SimEntity;
           Entry->Index = StorageIndex;
        }
    }
    else
    {
        SimEntity = Entry->Ptr;
    }

    return(SimEntity);
}

internal sim_entity *
AddEntity(sim_region *SimRegion, entity *StoredEntity, v2 *P = 0)
{
    sim_entity *SimEntity = AddEntityRaw(SimRegion, StoredEntity);
    Assert(SimEntity);
    Assert(!IsSet(SimEntity, EntityFlag_SimSpace));
    AddFlag(SimEntity, EntityFlag_SimSpace);
           
    if (SimEntity)
    { 
        if (P)
        {
            SimEntity->P = *P;    
        }
        else
        {
            SimEntity->P = MapIntoSimSpace(SimRegion, StoredEntity);
        }
         SimEntity->Updatable = IsInRectangle(SimRegion->UpdatableBounds, SimEntity->P);
        
    }
    return(SimEntity);
}


inline void
LoadEntityReference(sim_region *SimRegion, game_state *GameState, entity_reference *Ref)
{
    if (Ref->Index)
    {
        sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, Ref->Index);
        if (Entry->Ptr == 0)
        {
            Entry->Index = Ref->Index;
            entity *Entity = GameState->Entities + Ref->Index;
            Entry->Ptr = AddEntity(SimRegion, Entity);
        }
        Ref->Ptr = Entry->Ptr;
    }
}

inline void
StoreEntityReference(entity_reference *Ref)
{
    if (Ref->Ptr)
    {
        Ref->Index = Ref->Ptr->StorageIndex;
    }
}

internal sim_region *
BeginSim(memory_arena *SimArena, game_state *GameState, 
         tile_map_position Origin, rectangle2 UpdatableBounds)
{
    sim_region *SimRegion = PushStruct(SimArena, sim_region);
    SimRegion->TileMap = &GameState->TileMap; 
    SimRegion->Origin = Origin;
    SimRegion->UpdatableBounds = UpdatableBounds;

    r32 UpdatableSafetyMargin = 5.0f;
    SimRegion->Bounds = AddRadiusTo(UpdatableBounds, UpdatableSafetyMargin, UpdatableSafetyMargin);
    
    ZeroArray(SimRegion->Hash);

    SimRegion->EntityCount = 0;
    SimRegion->MaxEntityCount = 1024;
    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);

    tile_map *TileMap = &GameState->TileMap;
    tile_map_position MinTile = MapIntoTileSpace(TileMap, Origin, SimRegion->Bounds.Min);
    tile_map_position MaxTile = MapIntoTileSpace(TileMap, Origin, SimRegion->Bounds.Max);

    for (s32 Y = MinTile.Y; Y <= MaxTile.Y; ++Y)
    {
        for (s32 X = MinTile.X; X <= MaxTile.X; ++X)
        {
            tile *Tile = GetTile(TileMap, X, Y);
            if (Tile)
            {
                for (entity_block *Block = &Tile->FirstBlock; Block; Block = Block->Next)
                {
                    for (u32 Index = 0; Index < Block->EntityCount; ++Index)
                    {
                        u32 EntityIndex = Block->EntityIndex[Index];
                        entity *Entity = GameState->Entities + EntityIndex;
                        v2 P = MapIntoSimSpace(SimRegion, Entity);
                        if (IsInRectangle(SimRegion->Bounds, P))
                        {
                            AddEntity(SimRegion, Entity, &P);
                        }
                    }
                }
            }
        }
    }
    return(SimRegion);
}



internal void
EndSim(sim_region *Region, game_state *GameState)
{
    tile_map *TileMap = &GameState->TileMap;
    for (u32 Index = 0; Index < Region->EntityCount; ++Index)
    {
        sim_entity *SimEntity = Region->Entities + Index;
        Assert(IsSet(SimEntity, EntityFlag_SimSpace));
        ClearFlag(SimEntity, EntityFlag_SimSpace);
        
        tile_map_position P = MapIntoTileSpace(TileMap, Region->Origin, SimEntity->P);
        ChangeEntityLocation(GameState, SimEntity->StorageIndex, P);

        entity *StoredEntity = GameState->Entities + SimEntity->StorageIndex;
        Assert(!IsSet(&StoredEntity->Sim, EntityFlag_SimSpace));
        
        StoredEntity->P = P;
        StoredEntity->Sim = *SimEntity;

        if (SimEntity->StorageIndex == GameState->CameraFollowingEntityIndex)
        {

            GameState->CameraPosition = P;
        }
    }
}


internal bool32
HitsWall(r32 WallX, r32 DeltaX, r32 DeltaY,
         r32 RelativeX, r32 RelativeY,
         r32 MinY, r32 MaxY, r32 *MinT)
{
    bool32 Result = false;
    
    if (DeltaX != 0.0f)
    {
        r32 T = (WallX - RelativeX)/DeltaX;
        if ((T >= 0.0f) && (T < *MinT))
        {
            r32 Y = RelativeY + T * DeltaY;
            if ((Y > MinY) && (Y < MaxY))
            {
                Result = true;
                *MinT = T;
            }
        }   
    }

    return(Result);
}

internal void
MoveEntity(sim_region * SimRegion, sim_entity *Entity, r32 dt,
           move_spec *MoveSpec,  v2 ddP)
{
    if(MoveSpec->UnitMaxAccelVector)
    {
        r32 ddPLength = LengthSquared(ddP);
        if(ddPLength > 1.0f)
        {
            ddP *= 1.0f / SquareRoot(ddPLength);
        }
    }
    ddP *= MoveSpec->Speed;
    ddP += -MoveSpec->Drag* Entity->dP;
    
    tile_map *TileMap = SimRegion->TileMap;
    v2 PositionDelta = Square(dt)* 0.5f * ddP + dt * Entity->dP;

    v2 NewPosition = Entity->P + PositionDelta;
    Entity->dP += dt * ddP;


    v2 StartPosition = Entity->P;
 
    v2 WallNormal = {};
    r32 Epsilon = 0.0001f;
    for (s32 Iteration = 0; Iteration < 4; Iteration++)
    {
        r32 T = 1.0f;
        bool32 Hit = false;
        r32 DeltaLength = Length(PositionDelta);
        if (DeltaLength < Epsilon)
        {
            break;
        }
            
        if (DeltaLength > MoveSpec->DistanceRemaining)
        {
            T = MoveSpec->DistanceRemaining / DeltaLength;
        }
        for (u32 Index = 0; Index < SimRegion->EntityCount; ++Index)
        {
            sim_entity *TestEntity = SimRegion->Entities + Index;

            if (TestEntity == Entity)
            {
                continue;
            }
            if(!IsSet(TestEntity, EntityFlag_Collides) &&
               IsSet(TestEntity, EntityFlag_NonSpatial))
            {
                continue;
            }

            r32 RadiusW = 0.5f*(Entity->Width + TestEntity->Width);
            r32 RadiusH = 0.5f*(Entity->Height + TestEntity->Height);

            v2 Relative = StartPosition - TestEntity->P;
            if (HitsWall(-RadiusW, PositionDelta.X, PositionDelta.Y,
                         Relative.X, Relative.Y, -RadiusH, RadiusH, &T))
            {
                Hit = true;
                WallNormal = v2{1.0f,0.0f};
            }
            if (HitsWall(RadiusW, PositionDelta.X, PositionDelta.Y,
                         Relative.X, Relative.Y, -RadiusH, RadiusH, &T))
            {
                Hit = true;
                WallNormal = v2{-1.0f,0.0f};
            }
            if (HitsWall(-RadiusH, PositionDelta.Y, PositionDelta.X,
                         Relative.Y, Relative.X, -RadiusW, RadiusW, &T))
            {
                Hit = true;
                WallNormal = v2{0.0f,1.0f};
            }
            if (HitsWall(RadiusH, PositionDelta.Y, PositionDelta.X,
                         Relative.Y, Relative.X, -RadiusW, RadiusW, &T))
            {
                Hit = true;
                WallNormal = v2{0.0f,-1.0f};
            }

            if (Hit)
            {
                
            }
        }
        // TODO IMPORTANT BUG move even its hit other entity
        T = T < 1.0f ? Maximum(0.0f, T-Epsilon) : T;
        Entity->P = StartPosition + PositionDelta * T;
        MoveSpec->DistanceRemaining -= DeltaLength * T;
        if (Hit)
        {
            StartPosition += PositionDelta * T;
            Entity->dP -= (Entity->dP * WallNormal) * WallNormal;
            //TODO Player.P - StartPosition???
            PositionDelta -= T * PositionDelta;
            PositionDelta -= (PositionDelta * WallNormal) * WallNormal;
        }
        else
        {
            break;
        }
    }
    if (Entity->dP.X > 0)
    {
        Entity->FacingDirection = 1;
    }
    else
    {
        Entity->FacingDirection = 0;
    }
    
    
}

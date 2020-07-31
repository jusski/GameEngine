internal tile_map_position
NullPosition()
{
    tile_map_position Result = {INT32_MAX, INT32_MAX};

    return(Result);
}

internal bool32
IsValid(tile_map_position P)
{
    bool32 Result = P.X != INT32_MAX;

    return(Result);
}

internal bool32
IsInSameTile(tile_map_position A, tile_map_position B)
{
    bool32 Result =
        ((A.X == B.X) &&
         (A.Y == B.Y));

    return(Result);
}


internal void
NormaliseTileMapPosition(tile_map *TileMap, tile_map_position *Position)
{
    s32 TilesDiv = RoundR32ToS32(Position->Offset.X / TileMap->TileSizeInMeters);
    r32 TilesMod = Position->Offset.X - ((r32)TilesDiv * TileMap->TileSizeInMeters);

    Position->X += TilesDiv;
    Position->Offset.X = TilesMod;

    TilesDiv = RoundR32ToS32(Position->Offset.Y / TileMap->TileSizeInMeters);
    TilesMod = Position->Offset.Y - ((r32)TilesDiv * TileMap->TileSizeInMeters);

    Position->Y += TilesDiv;
    Position->Offset.Y = TilesMod;
    
}

internal tile *
GetTile(tile_map *TileMap, u32 X, u32 Y, memory_arena *Arena = 0)
{
    u32 HashValue = 13 * X + 3 * Y;
    u32 HashSlot = HashValue & (ArrayCount(TileMap->ChunkHashMap) - 1);
    Assert(HashSlot < ArrayCount(TileMap->ChunkHashMap));
    
    tile *Tile = TileMap->ChunkHashMap[HashSlot];

    while(Tile)
    {
      
        if ((Tile->X == X) &&
            (Tile->Y == Y))
        {
            break;
        }
        Tile = Tile->NextInHash;
    }
    
    if (Tile == 0 && Arena)
    {
        Tile = PushStruct(Arena, tile);
        Tile->X = X;
        Tile->Y = Y;

        Tile->NextInHash = 0;

        
        TileMap->ChunkHashMap[HashSlot] = Tile;
    }
    return Tile;    
}

internal tile *
GetTile(tile_map *TileMap, tile_map_position Position, memory_arena *Arena = 0)
{
    tile *Result = GetTile(TileMap, Position.X, Position.Y, Arena);
   
    return Result;
}

internal tile_map_position
Offset(tile_map *TileMap, tile_map_position Position, tile_map_position Delta)
{
    tile_map_position Result = Position;

    Result.X += Delta.X;
    Result.Y += Delta.Y;
    Result.Offset += Delta.Offset;
    NormaliseTileMapPosition(TileMap, &Result);

    return(Result); 
}


internal tile_map_position
Offset(tile_map *TileMap, tile_map_position Position, v2 Delta)
{
    tile_map_position Result = Position;

    Result.Offset += Delta;
    NormaliseTileMapPosition(TileMap, &Result);

    return(Result); 
}

internal v2
SubtractPosition(tile_map *TileMap, tile_map_position &A, tile_map_position &B)
{
    v2 Result;
    
    Result.X = (A.X - B.X)*TileMap->TileSizeInMeters;
    Result.Y = (A.Y - B.Y)*TileMap->TileSizeInMeters;
    Result += A.Offset - B.Offset;
    
    return(Result);
}

internal tile_map_position
TileCenterPosition(u32 X, u32 Y)
{
    tile_map_position Result = {};

    Result.X = X;
    Result.Y = Y;

    return(Result);
}

internal tile_map_position
MapIntoTileSpace(tile_map *TileMap, tile_map_position Position, v2 Delta)
{
    tile_map_position Result = Position;

    Result = Offset(TileMap, Result, Delta);
    
    return(Result);
}

internal void
ChangeEntityLocationRaw(game_state *GameState, u32 EntityIndex,
                        tile_map_position OldP, tile_map_position NewP)
{
    Assert(EntityIndex);
    memory_arena *Arena = &GameState->Arena;
    tile_map *TileMap = &GameState->TileMap;

    if (IsInSameTile(OldP, NewP))
    {
        return;
    }

    entity *Entity = GameState->Entities + EntityIndex;
    // Remove Entity from old Tile
    if (IsValid(OldP))
    {
        tile *Tile = GetTile(TileMap, OldP.X, OldP.Y);
        entity_block *FirstBlock = &Tile->FirstBlock;
        bool32 NotFound = true;
        for (entity_block *Block = FirstBlock; Block && NotFound; Block = Block->Next)
        {
        
            for (u32 Index = 0; Index < Block->EntityCount; ++Index)
            {
                if (EntityIndex == Block->EntityIndex[Index])
                {
                    Assert(FirstBlock->EntityCount > 0);
                    Block->EntityIndex[Index] = FirstBlock->EntityIndex[--FirstBlock->EntityCount];

                    if (FirstBlock->EntityCount == 0)
                    {
                        if (FirstBlock->Next)
                        {
                            entity_block *NextBlock = FirstBlock->Next;
                            *FirstBlock = *NextBlock;

                            NextBlock->Next = TileMap->FirstFree;
                            TileMap->FirstFree = NextBlock;
                        }
                    }
                    NotFound = false;
                    break;
                }
                    
            }
        }
        
        Entity->P = NullPosition();
    }

    // Add Entity to new Tile position
    if (IsValid(NewP))
    {
        tile *Tile = GetTile(TileMap, NewP.X, NewP.Y, Arena);
        Assert(Tile);
        entity_block *FirstBlock = &Tile->FirstBlock;

        if (FirstBlock->EntityCount < ArrayCount(FirstBlock->EntityIndex))
        {
            
        }
        else
        {
            // add new block
            entity_block *FreeBlock = TileMap->FirstFree;
            if (FreeBlock)
            {
                TileMap->FirstFree = FreeBlock->Next;
                
            }
            else
            {
                FreeBlock = PushStruct(Arena, entity_block);
            }
            
            *FreeBlock = *FirstBlock;
            FirstBlock->EntityCount = 0;
            FirstBlock->Next = FreeBlock;
        }
        
        FirstBlock->EntityIndex[FirstBlock->EntityCount++] = EntityIndex;

        Entity->P = NewP;
    }
    
}

internal void
ChangeEntityLocation(game_state *GameState, u32 EntityIndex,
                        tile_map_position NewP)
{

    entity *StoredEntity = GameState->Entities + EntityIndex;
    tile_map_position OldP = StoredEntity->P;
    
    ChangeEntityLocationRaw(GameState, EntityIndex, OldP, NewP);
}

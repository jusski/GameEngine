#include "game.h"

static game_memory *GlobalMemory;

#include "entity.cpp"
#include "tile_map.cpp"
#include "sim_region.cpp"
#include "render_group.cpp"

internal move_spec
DefaultMoveSpec()
{
    move_spec Result;

    Result.UnitMaxAccelVector = false;
    Result.Speed = 1.0f;
    Result.Drag = 0.0f;
    Result.DistanceRemaining = 1000.0f;

    return Result;
}

internal void
InitializeArena(memory_arena *Arena, u32 Size, void *Memory)
{
    Arena->Memory = (u8 *)Memory;
    Arena->AvailableSize = Size;
    Arena->Index = 0;
}

internal void
WeirdSound(game_state *State, game_sound_output_buffer *SoundData)
{
    s16 *Memory = (s16 *) SoundData->Buffer;
    float t = State->t;
    for (u32 i =0; i < SoundData->Samples; i++)
    {
        float Value = sinf(t) * 2000;
        *Memory++ = (s16) Value;
        *Memory++ = (s16) Value;

        t += 2.0f * 3.14f * 1.0f / 210.0f;
    }
    State->t = t;
}

internal void
DrawLine(game_offscreen_bitmap *Bitmap, s32 x, s32 y, s32 Length, u32 Color)
{
    for (s32 i = 0; i < Length; i++)
    { 
        if (x >= (Bitmap->Width))
        {
            x = x % (Bitmap->Width);
        }
        u8 *Memory = (u8 *)Bitmap->Memory +
            x * Bitmap->Stride + y * Bitmap->Pitch;
        *(u32 *)Memory = Color;
        x += 1;
    }    
}

internal inline u32
ColorShift(u32 Mask)
{
    u32 Result =
        Mask & 0xFF000000 ? 24 :
        Mask & 0x00FF0000 ? 16 :
        Mask & 0x0000FF00 ? 8 : 0;
    return(Result);
}

internal loaded_bitmap
LoadBitmap(read_file_to_memory ReadFileToMemory, const char *FileName)
{
    loaded_bitmap Result = {};
    
    loaded_file LoadedFile = ReadFileToMemory(FileName);
    bitmap_format *BitmapFormat;

    if (LoadedFile.Size != 0)
    {
        BitmapFormat = (bitmap_format *) LoadedFile.Bytes;
        Assert(BitmapFormat->BitsPerPixel == (BITMAP_BYTES_PER_PIXEL * 8));
        
        Result.Width = BitmapFormat->Width;
        Result.Height = BitmapFormat->Height;
        Result.Pitch = Result.Width * BITMAP_BYTES_PER_PIXEL;
        
        Result.Bytes = (u8 *)LoadedFile.Bytes + BitmapFormat->BitmapOffset;

        u32 Length = Result.Width * Result.Height;
        u32 *Pixel = (u32 *)Result.Bytes;

        u32 RedShift = ColorShift(BitmapFormat->RedMask);
        u32 GreenShift = ColorShift(BitmapFormat->GreenMask);
        u32 BlueShift = ColorShift(BitmapFormat->BlueMask);
        u32 AlphaShift = ColorShift(BitmapFormat->AlphaMask);
        while(Length--)
        {
            u32 R = (*Pixel >> RedShift) & 0xFF;
            u32 G = (*Pixel >> GreenShift) & 0xFF;
            u32 B = (*Pixel >> BlueShift) & 0xFF;
            u32 A = (*Pixel >> AlphaShift) & 0xFF;

            v4 Texel = V4(R, G, B, A);
            Texel = SRGBA255ToLinear1(Texel);
            Texel.rgb *= Texel.a;
            Texel = Linear1ToSRGBA255(Texel);

            *Pixel++ = PackBGRA(Texel);
        }
    }
    return(Result);
}

internal void
ClearBitmap(loaded_bitmap *Bitmap)
{
    if (Bitmap->Bytes)
    {
        u32 TotalBitmapSize = Bitmap->Width * Bitmap->Height * BITMAP_BYTES_PER_PIXEL;
        ZeroMemory(TotalBitmapSize, Bitmap->Bytes);
    }
}

internal loaded_bitmap *
MakeEmptyBitmap(memory_arena *Arena, u32 Width, u32 Height, bool32 ClearToZero = true)
{
    loaded_bitmap *Result = PushStruct(Arena, loaded_bitmap);
    
    Result->Width = Width;
    Result->Height = Height;
    Result->Pitch = Width * BITMAP_BYTES_PER_PIXEL;
    u32 TotalBitmapSize = Width * Height * BITMAP_BYTES_PER_PIXEL;
    Result->Bytes = PushArray(Arena, TotalBitmapSize, u8);

    if(ClearToZero)
    {
        ClearBitmap(Result);
    }
    
    return(Result);
}

internal normal_map *
MakeEmptyNormalMap(memory_arena *Arena, u32 Width, u32 Height)
{
    normal_map *Result = PushStruct(Arena, normal_map);
    
    Result->Width = Width;
    Result->Height = Height;
    
    u32 TotalSize = Width * Height;
    Result->Bytes = PushArray(Arena, TotalSize, v4);
    
    return(Result);
}

internal entity *
AddEntity(game_state *GameState, entity_type Type, tile_map_position P)
{
    Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
    u32 EntityIndex = ++GameState->EntityCount; 
    
    entity *Result = GameState->Entities + EntityIndex;
    *Result = {};
    Result->Sim.Type = Type;
    Result->Sim.StorageIndex = EntityIndex;
    Result->P = NullPosition();
    
    ChangeEntityLocation(GameState, EntityIndex, P);
    
    return(Result);
}

internal entity *
AddTree(game_state *GameState, tile_map_position P)
{
    entity *Entity = AddEntity(GameState, EntityType_Tree, P);

    Entity->Sim.Width = 1.0f * GameState->WallSize;
    Entity->Sim.Height = Entity->Sim.Width;
    AddFlag(&Entity->Sim, EntityFlag_Collides);

    return(Entity);
}

internal entity *
AddWall(game_state *GameState, tile_map_position P)
{
    entity *Entity = AddEntity(GameState, EntityType_Wall, P);

    Entity->Sim.Width = GameState->WallSize;
    Entity->Sim.Height = Entity->Sim.Width;
    AddFlag(&Entity->Sim, EntityFlag_Collides);

    return(Entity);
}


internal entity *
AddHero(game_state *GameState)
{
    tile_map_position P = GameState->CameraPosition;
    entity *Entity = AddEntity(GameState, EntityType_Hero, P);

    Entity->Sim.Width = 0.8f * GameState->WallSize;
    Entity->Sim.Height = Entity->Sim.Width;
    AddFlag(&Entity->Sim, EntityFlag_Collides | EntityFlag_Moves);

    GameState->CameraFollowingEntityIndex = Entity->Sim.StorageIndex;

    return(Entity);
}

internal void
ClearScreen(loaded_bitmap *Buffer)
{
    DrawRectangle(Buffer, 0, 0, (r32)Buffer->Width, (r32)Buffer->Height,
                  V4(1, 0, 1, 1));
}

internal void
CreateTileGround(game_state *GameState, loaded_bitmap *DrawBuffer, tile_map_position Position)
{
    r32 PixelsPerMeter = GameState->PixelsPerMeter;
    s32 Width = DrawBuffer->Width;
    s32 Height = DrawBuffer->Height;
    loaded_bitmap *Stamp = 0;

    for (s32 OffsetY = -1; OffsetY < 2; ++OffsetY)
    {
        for (s32 OffsetX = -1; OffsetX < 2; ++OffsetX)
        {
            s32 PositionX = Position.X + OffsetX;
            s32 PositionY = Position.Y + OffsetY;
            //TODO Seed by wong integer hash?
            random_series Series = RandomSeed(131*PositionX + 592*PositionY);

            v2 Center = V2(Width * OffsetX, Height * OffsetY);
            for(u32 Index = 0; Index < 5; ++Index)
            {
                Stamp = &GameState->Ground[RandomChoice(&Series, 4)];
                v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

                v2 Offset = V2(Width * RandomUnilateral(&Series),
                               Height * RandomUnilateral(&Series));
        
                v2 P = Center + Offset - BitmapCenter;
        
                CompositeBitmap(DrawBuffer, Stamp, P);    
            }        
        }
    }
    
    for (s32 OffsetY = -1; OffsetY < 2; ++OffsetY)
    {
        for (s32 OffsetX = -1; OffsetX < 2; ++OffsetX)
        {
            s32 PositionX = Position.X + OffsetX;
            s32 PositionY = Position.Y + OffsetY;
            random_series Series = RandomSeed(131*PositionX + 592*PositionY);
     
            v2 Center = V2(Width * OffsetX, Height * OffsetY);    
            for(u32 Index = 0; Index < 5; ++Index)
            {
                Stamp = &GameState->Grass[RandomChoice(&Series, 2)];
                v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

                v2 Offset = V2(Width * RandomUnilateral(&Series),
                               Height * RandomUnilateral(&Series));
        
                v2 P = Center + Offset - BitmapCenter;
        
                CompositeBitmap(DrawBuffer, Stamp, P);    
            }
        }
    }
    
    for (s32 OffsetY = -1; OffsetY < 2; ++OffsetY)
    {
        for (s32 OffsetX = -1; OffsetX < 2; ++OffsetX)
        {
            s32 PositionX = Position.X + OffsetX;
            s32 PositionY = Position.Y + OffsetY;
            random_series Series = RandomSeed(131*PositionX + 592*PositionY);
     
            v2 Center = V2(Width * OffsetX, Height * OffsetY);    

            for(u32 Index = 0; Index < 15; ++Index)
            {
                Stamp = &GameState->Tuft[RandomChoice(&Series, 3)];
                v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

                v2 Offset = V2(Width * RandomUnilateral(&Series),
                               Height * RandomUnilateral(&Series));
        
                v2 P = Center + Offset - BitmapCenter;
        
                CompositeBitmap(DrawBuffer, Stamp, P);    
                
            }
        }
    }

    for (s32 OffsetY = -1; OffsetY < 2; ++OffsetY)
    {
        for (s32 OffsetX = -1; OffsetX < 2; ++OffsetX)
        {
            s32 PositionX = Position.X + OffsetX;
            s32 PositionY = Position.Y + OffsetY;
            random_series Series = RandomSeed(131*PositionX + 592*PositionY);
     
            v2 Center = V2(Width * OffsetX, Height * OffsetY);    
            
            for(u32 Index = 0; Index < 1; ++Index)
            {
                Stamp = &GameState->Rock[RandomChoice(&Series, 4)];
                v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

                v2 Offset = V2(Width * RandomUnilateral(&Series),
                               Height * RandomUnilateral(&Series));
        
                v2 P = Center + Offset - BitmapCenter;
        
                CompositeBitmap(DrawBuffer, Stamp, P);    
            }
        }
    }
    
}

internal void
DrawRectangleOutline(loaded_bitmap *Bitmap, v2 TopLeft, v2 BottomRight, v4 Color)
{
    r32 T = 1.0f;
    
    // Top and Bottom lines
    DrawRectangle(Bitmap, TopLeft, v2{BottomRight.x, TopLeft.y + T}, Color);
    DrawRectangle(Bitmap, v2{TopLeft.x, BottomRight.y - T}, BottomRight, Color);

    // Right and Left lines
    DrawRectangle(Bitmap, TopLeft, v2{TopLeft.x + T, BottomRight.y}, Color);
    DrawRectangle(Bitmap, v2{BottomRight.x - T, TopLeft.y}, BottomRight, Color);
    

}

internal void
DrawTilesOutline(loaded_bitmap *Bitmap, game_state *GameState, tile_map_position Origin)
{
    tile_map *TileMap = &GameState->TileMap;
    u32 TileSizeInPixels = TileMap->TileSizeInPixels;
    r32 PixelsPerMeter = GameState->PixelsPerMeter;

    v2 ScreenCenter = 0.5f * V2(Bitmap->Width, Bitmap->Height);
    
    v2 ScreenDimInMeters = GameState->MetersPerPixel * V2(Bitmap->Width, Bitmap->Height);
    rectangle2 ScreenBounds = RectCentDim(V2(0,0), ScreenDimInMeters);

    tile_map_position MinTile = MapIntoTileSpace(TileMap, Origin, ScreenBounds.Min);
    tile_map_position MaxTile = MapIntoTileSpace(TileMap, Origin, ScreenBounds.Max);

    for (s32 Y = MinTile.Y; Y <= MaxTile.Y; ++Y)
    {
        for (s32 X = MinTile.X; X <= MaxTile.X; ++X)
        {
            tile_map_position Position = {X, Y};
            v2 RelP = SubtractPosition(TileMap, Position, Origin);
            v2 ScreenP = ScreenCenter + PixelsPerMeter * V2(RelP.x, -RelP.y);
            v2 TopLeft = ScreenP - 0.5f*V2(TileSizeInPixels, TileSizeInPixels);
            v2 BottomRight = ScreenP + 0.5f*V2(TileSizeInPixels, TileSizeInPixels);
            v4 Color = {0,0,0,1};
            DrawRectangleOutline(Bitmap, TopLeft, BottomRight, Color );
        }
    }
}

internal void
DrawGroundBuffers(loaded_bitmap *Bitmap, game_state *GameState,
                  transient_state *TranState, tile_map_position Origin)
{
    
    tile_map *TileMap = &GameState->TileMap;
    ground_buffer *Buffers = TranState->GroundBuffers;
    
    u32 TileSizeInPixels = TileMap->TileSizeInPixels;
    r32 PixelsPerMeter = GameState->PixelsPerMeter;

    v2 ScreenCenter = 0.5f * V2(Bitmap->Width, Bitmap->Height);
    
    v2 ScreenDimInMeters = GameState->MetersPerPixel * V2(Bitmap->Width, Bitmap->Height);
    rectangle2 ScreenBounds = RectCentDim(V2(0,0), ScreenDimInMeters);

    tile_map_position MinTile = MapIntoTileSpace(TileMap, Origin, ScreenBounds.Min);
    tile_map_position MaxTile = MapIntoTileSpace(TileMap, Origin, ScreenBounds.Max);

    
    for (s32 Y = MinTile.Y; Y <= MaxTile.Y; ++Y)
    {
        for (s32 X = MinTile.X; X <= MaxTile.X; ++X)
        {
            tile_map_position Position = {X, Y};
            ground_buffer *FarthestBuffer = 0;
            r32 FarthestBufferDistance = 0.0f;
            for (u32 Index = 0; Index < TranState->GroundBufferCount; ++Index)
            {
                ground_buffer *GroundBuffer = Buffers + Index;
                
                if (IsInSameTile(Position, GroundBuffer->P))
                {
                    FarthestBuffer = 0;
                    break;
                }
                else if (!IsValid(GroundBuffer->P))
                {
                    FarthestBuffer = GroundBuffer;
                    FarthestBufferDistance = 99999.0f;
                }
                else
                {
                    v2 RelP = SubtractPosition(TileMap, Origin, GroundBuffer->P);
                    r32 BufferDistance = LengthSquared(RelP);
                    if (BufferDistance > FarthestBufferDistance)
                    {
                        FarthestBufferDistance = BufferDistance;
                        FarthestBuffer = GroundBuffer;
                    }
                }
            }
            if (FarthestBuffer)
            {
                FarthestBuffer->P = Position;
                
                ClearBitmap(FarthestBuffer->Bitmap);
                CreateTileGround(GameState, FarthestBuffer->Bitmap, Position);
            }
        }
    }
}


internal void
MakeRect(loaded_bitmap *Bitmap)
{
    u32 Width = Bitmap->Width;
    u32 Height = Bitmap->Height;
    
    for (u32 y = 0; y < Height; ++y)
    {
        for (u32 x = 0; x < Width; ++x)
        {
            u32 *Pixel = (u32 *)Bitmap->Bytes + y * Width + x;

            *Pixel = (u32) 0xFF000000;
            if ((x == 0) ||
                (x == (Width - 1)) ||
                (y == 0) ||
                (y == (Height - 1)))
            {
                *Pixel = (u32) 0xFFFFFFFF;
            }
        }
    }
}
    
internal void
InitializeGameState(game_state *GameState, loaded_bitmap *Video, game_memory *Memory)
{
    
        loaded_bitmap HeroFacingRight = LoadBitmap(Memory->ReadFileToMemory, "car_right.bmp");
        GameState->HeroFacingRight.Body = HeroFacingRight;
        GameState->HeroFacingRight.Body.Align = {20, 33};
        loaded_bitmap HeroFacingLeft = LoadBitmap(Memory->ReadFileToMemory, "car_left.bmp");
        GameState->HeroFacingLeft.Body = HeroFacingLeft;
        GameState->HeroFacingLeft.Body.Align = {20, 33};
        
        GameState->Grass[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/grass00.bmp");
        GameState->Grass[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/grass01.bmp");
        
        GameState->Ground[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground00.bmp");
        GameState->Ground[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground01.bmp");
        GameState->Ground[2] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground02.bmp");
        GameState->Ground[3] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground03.bmp");
        
        GameState->Tree[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/tree00.bmp");
        GameState->Tree[0].Align = {45,85};
        GameState->Tree[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/tree01.bmp");
        GameState->Tree[1].Align = {45,85};
        GameState->Tree[2] = LoadBitmap(Memory->ReadFileToMemory, "test2/tree02.bmp");
        GameState->Tree[2].Align = {45,85};
        
        GameState->Tuft[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/tuft00.bmp");
        GameState->Tuft[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/tuft01.bmp");
        GameState->Tuft[2] = LoadBitmap(Memory->ReadFileToMemory, "test2/tuft02.bmp");
        
        GameState->Rock[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/rock00.bmp");
        GameState->Rock[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/rock01.bmp");
        GameState->Rock[2] = LoadBitmap(Memory->ReadFileToMemory, "test2/rock02.bmp");
        GameState->Rock[3] = LoadBitmap(Memory->ReadFileToMemory, "test2/rock03.bmp");

        InitializeArena(&GameState->Arena,
                        Memory->PersistentStorageSize - sizeof(*GameState),
                        (u8 *)Memory->PersistentStorage + sizeof(*GameState));
        
        GameState->TileMap = {};
        tile_map *TileMap = &GameState->TileMap;
        
        GameState->PixelsPerMeter = 40.0f;
        GameState->MetersPerPixel = 1.0f/GameState->PixelsPerMeter;
        
        TileMap->TileSizeInPixels = 256;
        TileMap->TileSizeInMeters = TileMap->TileSizeInPixels * GameState->MetersPerPixel;

        GameState->CameraPosition = {};

#define Columns 10
#define Rows 10
        uint32 TileMap1[Rows][Columns] =
            {
                {0, 0, 2, 2, 2, 2, 2, 2, 2, 2},
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                {1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
                {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                {1, 0, 2, 0, 1, 0, 1, 0, 0, 1},
                {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                {1, 0, 0, 0, 0, 0, 0, 0, 2, 1},
                {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                {1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
            
            };
#if 1
        GameState->WallSize = 1.0f;
        s32 Y = INT32_MAX / 2;
        for (s32 i = 0; i < Rows; i++, Y++)
        {
            u32 X = INT32_MAX / 2;
            for (s32 j = 0; j < Columns; j++, X++)
            {
                if(TileMap1[i][j] == 1)
                {
                    tile_map_position P = {};
                    v2 Delta = {GameState->WallSize * j, GameState->WallSize * i};
                    P = MapIntoTileSpace(TileMap, P, Delta);
                    AddWall(GameState, P);
                }
                else if (TileMap1[i][j] == 2)
                {
                    tile_map_position P = {};
                    v2 Delta = 1.5f * V2(GameState->WallSize * j,
                                         GameState->WallSize * i);
                    P = MapIntoTileSpace(TileMap, P, Delta);
                    AddTree(GameState, P);
                }
                else
                {
                    
                }
            }
        }
#else
        tile_map_position P = {0,0};
        AddWall(GameState, &P);
#endif
        tile_map_position HeroStartingP = {-2,-2};
        GameState->Player = {};
        entity *Entity = AddHero(GameState);
        GameState->Player.EntityIndex = Entity->Sim.StorageIndex;


        GameState->IsInitialized = true;
}


internal void
InitializeTranState(transient_state *TranState, game_state *GameState,
                    loaded_bitmap *Video, game_memory *Memory)
{
    InitializeArena(&TranState->Arena,
                    Memory->TransientStorageSize - sizeof(transient_state),
                    (u8 *)Memory->TransientStorage + sizeof(transient_state));

    TranState->GroundBufferCount = 64;
    TranState->GroundBuffers = PushArray(&TranState->Arena, TranState->GroundBufferCount, ground_buffer);

    u32 GroundWidth = GameState->TileMap.TileSizeInPixels;
    u32 GroundHeight = GroundWidth;
        
    for(u32 Index = 0; Index < TranState->GroundBufferCount; ++Index)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + Index;

        loaded_bitmap *Bitmap = MakeEmptyBitmap(&TranState->Arena, GroundWidth, GroundHeight, false);
        Bitmap->Align = 0.5f * V2(Bitmap->Width, Bitmap->Height);

        GroundBuffer->Bitmap = Bitmap;
        GroundBuffer->P = NullPosition();
    }
    
    normal_map *Template = MakeEmptyNormalMap(&GameState->Arena, 100, 100);
    GameState->SphereNormal = *Template;
    MakeSphereNormalMap(&GameState->SphereNormal);

    Template = MakeEmptyNormalMap(&GameState->Arena, 100, 100);
    GameState->TestNormal = *Template;
    MakeTestNormalMap(&GameState->TestNormal);

    loaded_bitmap *Sky = MakeEmptyBitmap(&TranState->Arena, 200, 200);
    u32 Height = Sky->Height / 21;
    u32 Width = Sky->Width / 21;
    bool32 SwapColor = false;
    v4 SkyColor = V4(0.5f,0.0f,0.0f,1.0f);
    //v4 SkyColor = V4(0.52f, 0.82f, 0.92f, 1.0f);
    v4 DarkColor = V4(0.0f,0.99f,0.0f,1.0f);
    
    for (u32 j = 0; j < Sky->Height / Height; ++j)
    {
        for (u32 i = 0; i < Sky->Width / Width; ++i)
        {
            v2 Offset = V2(i * Width, j * Height);
            v4 Color = SwapColor ? SkyColor : DarkColor;
            DrawRectangle(Sky, Offset, Offset + V2(Width, Height),
                          Color);
            SwapColor = !SwapColor;
                
        }
        SwapColor = !SwapColor;
        
    }
    
    loaded_bitmap *Ground = MakeEmptyBitmap(&TranState->Arena, 100, 100);
    Height = Ground->Height / 11;
    Width = Ground->Width / 11;
    SwapColor = false;
    v4 GroundColor = V4(0.486f, 0.988f, 0.0f, 1.0f);
    DarkColor = V4(0,1,0,1);
    for (u32 j = 0; j < Height; ++j)
    {
        for (u32 i = 0; i < Width; ++i)
        {
            v2 Offset = V2(i * Width, j * Height);
            v4 Color = SwapColor ? GroundColor : DarkColor;
            DrawRectangle(Ground, Offset, Offset + V2(Width, Height),
                          Color);
            SwapColor = !SwapColor;
                
        }
    }

    loaded_bitmap *Reflection = MakeEmptyBitmap(&GameState->Arena, 100, 100);
    GameState->Reflection = *Reflection;
    v4 Color = 0.5f * V4(1.0f, 1.0f, 1.0f, 1.0f);
    DrawRectangle(Reflection, V2(0,0), V2(Reflection->Width, Reflection->Height), Color);    
    
    //TranState->Sky.LOD[0] = &GameState->Tree[0];
    TranState->Sky.LOD[0] = Sky;
    TranState->Ground.LOD[0] = Ground;
    
    TranState->IsInitialized = true;    
}

extern "C" void __declspec(dllexport) 
GameEngine(game_memory *Memory, game_input *Input,
           game_sound_output_buffer *Sound,
           game_offscreen_bitmap *OutputBitmap)
{
    GlobalMemory = Memory;

    
    loaded_bitmap Video_ = {};
    loaded_bitmap *Video = &Video_;
    Video->Bytes = (u8 *)OutputBitmap->Memory;
    Video->Width = OutputBitmap->Width;
    Video->Height = OutputBitmap->Height;
    Video->Pitch = OutputBitmap->Pitch;
    
    Assert(sizeof(game_state) <= Memory->PersistentStorageSize);
    game_state *GameState = (game_state *)Memory->PersistentStorage;
    
    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
    transient_state *TranState = (transient_state *)Memory->TransientStorage;

    game_controller_input *Keyboard = &Input->Keyboard;

    if (!GameState->IsInitialized)
    {
        InitializeGameState(GameState, Video, Memory);
    }
    

    if(!TranState->IsInitialized)
    {
        InitializeTranState(TranState, GameState, Video, Memory);
    }
                
    controlled_hero *ConHero = &GameState->Player;
    ConHero->ddP = {};
    
    v2 Accelaration = {0.0f, 0.0f};
    if (Keyboard->Right.IsDown)
    {
        ConHero->ddP.x += 1.0f;               
    }
    if (Keyboard->Left.IsDown)
    {
        ConHero->ddP.x += -1.0f;
    }
    if (Keyboard->Down.IsDown)
    {
        ConHero->ddP.y += -1.0f;
    }
    if (Keyboard->Up.IsDown)
    {
        ConHero->ddP.y += 1.0f;
    }


    v2 ScreenCenter = 0.5f * V2(Video->Width, Video->Height);

        
    memory_arena *TempArena = &TranState->Arena;

    temporary_memory RenderMemory = BeginTemporaryMemory(TempArena);
    render_group *RenderGroup = AllocateRenderGroup(TempArena, GameState, MegaBytes(4));

    PushClear(RenderGroup, V4(0.3f,0.3f,0.3f,0.0f));
    
    
    for (u32 Index = 0; Index < TranState->GroundBufferCount; ++Index)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + Index;
        
        if (IsValid(GroundBuffer->P))
        {
            v2 Offset = SubtractPosition(&GameState->TileMap,
                                         GroundBuffer->P, GameState->CameraPosition);
            
            //TODO Make CompositeBitmap 
            PushBitmap(RenderGroup, GroundBuffer->Bitmap, Offset);
            //CompositeBitmap(Video, Template, P.X, P.Y, 1.0f);
        }
    
    }
    
    tile_map *TileMap = &GameState->TileMap;
    v2 CameraDim = TileMap->TileSizeInMeters * v2{(r32)Video->Width, (r32)Video->Height};
    rectangle2 CameraBounds = RectCentDim(v2{0,0}, CameraDim);


    
    temporary_memory SimMemory = BeginTemporaryMemory(TempArena);
    sim_region *SimRegion = BeginSim(TempArena, GameState, GameState->CameraPosition, CameraBounds);
    
    sim_entity *Entity = SimRegion->Entities;
    for (uint32 EntityIndex = 0;
         EntityIndex < SimRegion->EntityCount;
         ++EntityIndex, ++Entity)
    {
        if (!Entity->Updatable)
        {
            continue;
        }

        real32 dt = Input->dtForFrame;
        move_spec MoveSpec = DefaultMoveSpec();
        v2 ddP = {};
        switch (Entity->Type)
        {
            case EntityType_Hero:
            {
                hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
            
                MoveSpec.UnitMaxAccelVector = true;
                MoveSpec.Speed = 50.0f;
                MoveSpec.Drag = 10.0f;
                ddP = ConHero->ddP;
                
                if((ConHero->dSword.x != 0.0f) || (ConHero->dSword.y != 0.0f))
                {
                    sim_entity *Sword = Entity->Sword.Ptr;
                    if(Sword )
                    {
                        Sword->P = Entity->P;
                        Sword->DistanceRemaining = 5.0f;
                        Sword->dP = 5.0f*ConHero->dSword;
                    }
                }
        
                v4 Color = {0.7f, 0.7f, 0.7f, 1.0f};
                //PushRect(RenderGroup, Entity->P, v2{Entity->Width, Entity->Height}, Color);
                PushBitmap(RenderGroup, &HeroBitmaps->Body, Entity->P);
            } break;

            case EntityType_Wall:
            {
                v4 Color = {0.2f, 0.7f, 0.7f, 1.0f};
                PushRect(RenderGroup, Entity->P, v2{Entity->Width, Entity->Height}, Color);
            } break;

            case EntityType_Tree:
            {
                v3 Color = {0.2f, 0.7f, 0.7f};
                PushBitmap(RenderGroup, &GameState->Tree[0], Entity->P);
                //PushRect(RenderGroup, Entity->P, v2{Entity->Width, Entity->Height}, Color);
            } break;
            
            InvalidDefaultCase;
        }

        if (IsSet(Entity, EntityFlag_Moves))
        {
            MoveEntity(SimRegion, Entity, dt, &MoveSpec, ddP);
        }

    }

    local_persist r32 Angle = 0;
    Angle+= Input->dtForFrame;

#if 1 // Rotation
    v2 XAxis = 100.0f * V2(Cos(0.5f*Angle), Sin(0.5f*Angle));
#else
    v2 XAxis = 300.0f * V2(1.f, 0.f);
#endif
    
    v2 YAxis = Perp(XAxis);
    v4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
#if 1
    v2 Origin = ScreenCenter - 0.5f*XAxis - 0.5f*YAxis;
#else
    v2 Origin = ScreenCenter - Sin(Angle)*XAxis - 0.5f*YAxis;
#endif

#if 0 // Offset
    v2 ScreenPosition = V2(400.f*Cos(Angle), 0.f);
#else
    v2 ScreenPosition = V2(0.f, 0.f);
#endif

#if 0 // Angle
    MakeTestNormalMap(&GameState->TestNormal, (40.f * (Cos(Angle) + 1.0f)/2.f));
#else
    MakeTestNormalMap(&GameState->TestNormal, 0.f);
#endif
    
    normal_map *NormalMap = &GameState->SphereNormal;
#if 0
    PushTextureSlow(RenderGroup, Origin, XAxis, YAxis,
                    &GameState->Reflection, NormalMap, Color,
                    &TranState->Sky, &TranState->Ground,
                    ScreenPosition);
#endif
#if 0
    NormalMap = &GameState->PyramidNormal;
    PushTextureSlow(RenderGroup, V2(0, 0), 0.3f*XAxis, 0.3f*YAxis,
                    TranState->Sky.LOD[0], 0, Color,
                    &TranState->Sky, &TranState->Ground,
                    ScreenPosition);
    
#endif
    DrawGroundBuffers(Video, GameState, TranState, GameState->CameraPosition);
    RenderOutput(RenderGroup, Video, GameState->PixelsPerMeter);

    //DrawTilesOutline(Video, GameState, GameState->CameraPosition);


    EndSim(SimRegion, GameState);
    EndTemporaryMemory(&SimMemory);
    EndTemporaryMemory(&RenderMemory);


    TIMED_FUNCTION();
}

//debug_cycle_counter DebugCycles[__COUNTER__ + 1];



 

#include "game.h"
#include "entity.cpp"
#include "tile_map.cpp"
#include "sim_region.cpp"

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
DrawHitpoints(entity_visible_piece_group *PieceGroup, sim_entity *Entity)
{
    
}

internal void
UpdateSword(sim_region *SimRegion, sim_entity *Entity, float dt)
{
    
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

internal void
DrawRectangle(game_offscreen_bitmap *Bitmap, v2 TopLeft, v2 BottomRight,
              r32 R, r32 G, r32 B)
{
    Assert(R >= 0 && R <= 1.0f);
    Assert(G >= 0 && G <= 1.0f);
    Assert(B >= 0 && B <= 1.0f);

    TopLeft.X = (TopLeft.X < 0) ? 0 : TopLeft.X;
    TopLeft.Y = (TopLeft.Y < 0) ? 0 : TopLeft.Y;
    BottomRight.X = (BottomRight.X > Bitmap->Width) ? Bitmap->Width : BottomRight.X;
    BottomRight.Y = (BottomRight.Y > Bitmap->Height) ? Bitmap->Height : BottomRight.Y;

    s32 MinX = (s32)(TopLeft.X);
    s32 MinY = (s32)(TopLeft.Y);
    s32 MaxX = (s32)(BottomRight.X);
    s32 MaxY = (s32)(BottomRight.Y);

    u32 Color =
        RoundR32ToU32(B * 255.0f) << 0 |
        RoundR32ToU32(G * 255.0f) << 8 |
        RoundR32ToU32(R * 255.0f) << 16;
    
    
    u32 *Row = (u32 *)Bitmap->Memory + Bitmap->Width*MinY + MinX;
    for (s32 y = MinY; y < MaxY; y++)
    {
        u32 *Pixel = Row;
        for (s32 x = MinX; x < MaxX; x++)
        {
            *Pixel++ = Color;
        }
        Row += Bitmap->Width ;
    }
    
}

internal void
DrawRectangle(game_offscreen_bitmap *Bitmap,
              r32 TopLeftX, r32 TopLeftY,
              r32 BottomRightX, r32 BottomRightY,
              r32 R, r32 G, r32 B)
{
    v2 TopLeft = {TopLeftX, TopLeftY};
    v2 BottomRight = {BottomRightX, BottomRightY};
    DrawRectangle(Bitmap, TopLeft, BottomRight, R, G, B);
}

inline u32
ColorShift(u32 Mask)
{
    u32 Result =
        Mask & 0xFF000000 ? 24 :
        Mask & 0x00FF0000 ? 16 :
        Mask & 0x0000FF00 ? 8 : 0;
    return(Result);
}

internal loaded_bitmap
LoadBitmap(read_file_to_memory ReadFileToMemory, char *FileName)
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

        Result.Offset.X = 20;
        Result.Offset.Y = 33;
        
        Result.Bytes = (u8 *)LoadedFile.Bytes + BitmapFormat->BitmapOffset;

        u32 Length = Result.Width * Result.Height;
        u32 *Pixel = (u32 *)Result.Bytes;

        u32 RedShift = ColorShift(BitmapFormat->RedMask);
        u32 GreenShift = ColorShift(BitmapFormat->GreenMask);
        u32 BlueShift = ColorShift(BitmapFormat->BlueMask);
        u32 AlphaShift = ColorShift(BitmapFormat->AlphaMask);
        while(Length--)
        {
            u32 A = (*Pixel >> AlphaShift) & 0xFF;
            u32 R = (*Pixel >> RedShift) & 0xFF;
            u32 G = (*Pixel >> GreenShift) & 0xFF;
            u32 B = (*Pixel >> BlueShift) & 0xFF;

            r32 AN = A / 255.0f;
            R = (u32)((r32)R * AN);
            G = (u32)((r32)G * AN);
            B = (u32)((r32)B * AN);

            *Pixel++ =
                (A << 24) |
                (R << 16) |
                (G <<  8) |
                (B <<  0);
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

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *Arena, u32 Width, u32 Height, bool32 ClearToZero = true)
{
    loaded_bitmap Result = {};
    
    Result.Width = Width;
    Result.Height = Height;
    Result.Pitch = Width * BITMAP_BYTES_PER_PIXEL;
    u32 TotalBitmapSize = Width * Height * BITMAP_BYTES_PER_PIXEL;
    Result.Bytes = PushArray(Arena, TotalBitmapSize, u8);

    if(ClearToZero)
    {
        ClearBitmap(&Result);
    }
    
    return(Result);
}

internal void
CompositeBitmap(loaded_bitmap *Destination, loaded_bitmap *Source,
           r32 TopLeftX, r32 TopLeftY, r32 Alpha = 1.0f)
{

    s32 ClipX = (TopLeftX < 0) ? (s32)-TopLeftX : 0;
    s32 ClipY = (TopLeftY < 0) ? (s32)-TopLeftY : 0;

    s32 BottomRightX = (s32)TopLeftX + Source->Width;
    s32 BottomRightY = (s32)TopLeftY + Source->Height;
    BottomRightX = (BottomRightX > (s32)Destination->Width) ? Destination->Width : BottomRightX;
    BottomRightY = (BottomRightY > (s32)Destination->Height) ? Destination->Height : BottomRightY;

    TopLeftX = (TopLeftX < 0) ? 0 : TopLeftX;
    TopLeftY = (TopLeftY < 0) ? 0 : TopLeftY;
    
    s32 MinX = (s32)(TopLeftX);
    s32 MinY = (s32)(TopLeftY);
    s32 MaxX = (s32)(BottomRightX);
    s32 MaxY = (s32)(BottomRightY);
    
    u32 *DestinationRow = (u32 *)Destination->Bytes + Destination->Width*MinY + MinX;
    u32 *SourceRow = (u32 *)Source->Bytes;
    
    SourceRow += ClipY*Source->Width + ClipX; 
    
    for (s32 y = MinY; y < MaxY; y++)
    {
        u32 *DestinationPixel = DestinationRow;
        u32 *SourcePixel = SourceRow;
        for (s32 x = MinX; x < MaxX; x++)
        {

            u32 SA = ((*SourcePixel >> 24) & 0xFF);
            u32 SR = ((*SourcePixel >> 16) & 0xFF);
            u32 SG = ((*SourcePixel >> 8)  & 0xFF);
            u32 SB = ((*SourcePixel >> 0)  & 0xFF);

            u32 DA = ((*DestinationPixel >> 24) & 0xFF);
            u32 DR = ((*DestinationPixel >> 16) & 0xFF);
            u32 DG = ((*DestinationPixel >> 8)  & 0xFF);
            u32 DB = ((*DestinationPixel >> 0)  & 0xFF);

            r32 SAN = (r32)SA / 255.0f;
            r32 DAN = (r32)DA / 255.0f;
            r32 AN = SAN + DAN - (SAN * DAN ); 

            u32 R = DR + SR-(DR*SA)/256;
            u32 G = DG + SG-(DG*SA)/256;
            u32 B = DB + SB-(DB*SA)/256;

            u32 A = (u32)(AN * 255.0f);
            
            *DestinationPixel++ = (A << 24) | (R << 16) | (G << 8) | (B << 0);
            SourcePixel++;
        }
        DestinationRow += Destination->Width;
        SourceRow += Source->Width;
    }
    
}

internal void
DrawBitmap(game_offscreen_bitmap *Destination, loaded_bitmap *Source,
           r32 TopLeftX, r32 TopLeftY, r32 Alpha = 1.0f)
{

    s32 ClipX = (TopLeftX < 0) ? (s32)-TopLeftX : 0;
    s32 ClipY = (TopLeftY < 0) ? (s32)TopLeftY : 0;

    s32 BottomRightX = (s32)TopLeftX + Source->Width;
    s32 BottomRightY = (s32)TopLeftY + Source->Height;
    BottomRightX = (BottomRightX > Destination->Width) ? Destination->Width : BottomRightX;
    BottomRightY = (BottomRightY > Destination->Height) ? Destination->Height : BottomRightY;

    TopLeftX = (TopLeftX < 0) ? 0 : TopLeftX;
    TopLeftY = (TopLeftY < 0) ? 0 : TopLeftY;

    s32 MinX = (s32)(TopLeftX);
    s32 MinY = (s32)(TopLeftY);
    s32 MaxX = (s32)(BottomRightX);
    s32 MaxY = (s32)(BottomRightY);

    Assert(MaxX - MinX <= (s32)Source->Width);
    Assert(MaxY - MinY <= (s32)Source->Height);
    u32 *DestinationRow = (u32 *)Destination->Memory + Destination->Width*MinY + MinX;
    u32 *SourceRow = (u32 *)Source->Bytes + Source->Width * (Source->Height - 1);
    SourceRow += ClipY*Source->Width + ClipX;
    
    for (s32 y = MinY; y < MaxY; y++)
    {
        u32 *DestinationPixel = DestinationRow;
        u32 *SourcePixel = SourceRow;
        for (s32 x = MinX; x < MaxX; x++)
        {

            u32 SA = ((*SourcePixel >> 24) & 0xFF);
            u32 SR = ((*SourcePixel >> 16) & 0xFF);
            u32 SG = ((*SourcePixel >> 8)  & 0xFF);
            u32 SB = ((*SourcePixel >> 0)  & 0xFF);

            u32 DA = ((*DestinationPixel >> 24) & 0xFF);
            u32 DR = ((*DestinationPixel >> 16) & 0xFF);
            u32 DG = ((*DestinationPixel >> 8)  & 0xFF);
            u32 DB = ((*DestinationPixel >> 0)  & 0xFF);

            u32 R = DR + SR-(DR*SA)/256;
            u32 G = DG + SG-(DG*SA)/256;
            u32 B = DB + SB-(DB*SA)/256;

            *DestinationPixel++ = (R << 16) | (G << 8) | (B << 0);
            SourcePixel++;
        }
        DestinationRow += Destination->Width;
        SourceRow -= Source->Width;
    }
    
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
PushPieace(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
           v2 Offset, v2 Dim, v2 Align, v4 Color)
{
    Assert(Group->PieceCount < ArrayCount(Group->Pieces));
    entity_visible_piece *Piece = Group->Pieces + Group->PieceCount++;
    Piece->Bitmap = Bitmap;
    r32 PixelsPerMeter = Group->GameState->PixelsPerMeter;
    Piece->Offset = Offset; // TODO Is this in screen coords?
    Piece->R = Color.R;
    Piece->G = Color.G;
    Piece->B = Color.B;
    Piece->A = Color.A;
    Piece->Dim = Dim * PixelsPerMeter;
}

internal void
PushBitmap(entity_visible_piece_group *Group, loaded_bitmap *Bitmap, v2 Offset, r32 Alpha = 1.0f)
{
    Assert(Bitmap);
    v4 Color = {0.0f, 0.0f, 0.0f, Alpha};
    v2 Dim = {};
    v2 Align = {};
    PushPieace(Group, Bitmap, Offset, Dim, Align, Color);
}

internal void
PushRect(entity_visible_piece_group *Group, v2 Dim, v4 Color)
{
    v2 Align = {};
    v2 Offset = {};
    PushPieace(Group, 0, Offset, Dim, Align, Color);
}

internal void
ClearScreen(game_offscreen_bitmap *Buffer)
{
    DrawRectangle(Buffer, 0, 0, (r32)Buffer->Width, (r32)Buffer->Height,
                  1.0f, 0.0f, 1.0f);
}

internal void
DrawTestGround(game_state *GameState, loaded_bitmap *DrawBuffer)
{
    random_series Series = RandomSeed(12345);
    r32 PixelsPerMeter = GameState->PixelsPerMeter;
    loaded_bitmap *Stamp = 0;

    r32 Radius = 5.0f;
    v2 Center = 0.5 * V2(DrawBuffer->Width, DrawBuffer->Height);

    for(u32 Index = 0; Index < 15; ++Index)
    {
        Stamp = &GameState->Ground[RandomChoice(&Series, 4)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(RandomBilateral(&Series), RandomBilateral(&Series));
        v2 P = (Center - BitmapCenter) + Radius * PixelsPerMeter * Offset;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }

    for(u32 Index = 0; Index < 15; ++Index)
    {
        Stamp = &GameState->Grass[RandomChoice(&Series, 2)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(RandomBilateral(&Series), RandomBilateral(&Series));
        v2 P = (Center - BitmapCenter) + Radius * PixelsPerMeter * Offset;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }

    for(u32 Index = 0; Index < 15; ++Index)
    {
        Stamp = &GameState->Tuft[RandomChoice(&Series, 3)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(RandomBilateral(&Series), RandomBilateral(&Series));
        v2 P = (Center - BitmapCenter) + Radius * PixelsPerMeter * Offset;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }
    
    for(u32 Index = 0; Index < 5; ++Index)
    {
        Stamp = &GameState->Rock[RandomChoice(&Series, 4)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(RandomBilateral(&Series), RandomBilateral(&Series));
        v2 P = (Center - BitmapCenter) + Radius * PixelsPerMeter * Offset;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }

    
    
}

internal void
DrawTileGround(game_state *GameState, loaded_bitmap *DrawBuffer, tile_map_position Position)
{
    //TODO Seed by wong integer hash?
    random_series Series = RandomSeed(131*Position.X + 592*Position.Y);
    r32 PixelsPerMeter = GameState->PixelsPerMeter;
    u32 Width = DrawBuffer->Width;
    u32 Height = DrawBuffer->Height;
    loaded_bitmap *Stamp = 0;

    for(u32 Index = 0; Index < 15; ++Index)
    {
        Stamp = &GameState->Ground[RandomChoice(&Series, 4)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(Width * RandomUnilateral(&Series),
                       Height * RandomUnilateral(&Series));
        
        v2 P = Offset - BitmapCenter;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }

    for(u32 Index = 0; Index < 15; ++Index)
    {
        Stamp = &GameState->Grass[RandomChoice(&Series, 2)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(Width * RandomUnilateral(&Series),
                       Height * RandomUnilateral(&Series));
        
        v2 P = Offset - BitmapCenter;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }

    for(u32 Index = 0; Index < 15; ++Index)
    {
        Stamp = &GameState->Tuft[RandomChoice(&Series, 3)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(Width * RandomUnilateral(&Series),
                       Height * RandomUnilateral(&Series));
        
        v2 P = Offset - BitmapCenter;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }
    
    for(u32 Index = 0; Index < 5; ++Index)
    {
        Stamp = &GameState->Rock[RandomChoice(&Series, 4)];
        v2 BitmapCenter = 0.5f * V2(Stamp->Width, Stamp->Height);

        v2 Offset = V2(Width * RandomUnilateral(&Series),
                       Height * RandomUnilateral(&Series));
        
        v2 P = Offset - BitmapCenter;
        
        CompositeBitmap(DrawBuffer, Stamp, P.X, P.Y);    
    }

    
    
}
internal void
DrawRectangleOutline(game_offscreen_bitmap *Bitmap, v2 TopLeft, v2 BottomRight, v3 Color)
{
    r32 T = 1.0f;
    r32 Width = BottomRight.X - TopLeft.X;
    r32 Height = TopLeft.Y - BottomRight.Y;
    // Top and Bottom lines
    DrawRectangle(Bitmap, TopLeft, v2{BottomRight.X, TopLeft.Y + T}, Color.R, Color.G, Color.B);
    DrawRectangle(Bitmap, v2{TopLeft.X, BottomRight.Y - T}, BottomRight, Color.R, Color.G, Color.B);

    // Right and Left lines
    DrawRectangle(Bitmap, TopLeft, v2{TopLeft.X + T, BottomRight.Y}, Color.R, Color.G, Color.B);
    DrawRectangle(Bitmap, v2{BottomRight.X - T, TopLeft.Y}, BottomRight, Color.R, Color.G, Color.B);
    

}

internal void
DrawTilesOutline(game_offscreen_bitmap *Bitmap, game_state *GameState, tile_map_position Origin)
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
            v2 ScreenP = ScreenCenter + PixelsPerMeter * V2(RelP.X, -RelP.Y);
            v2 TopLeft = ScreenP - 0.5f*V2(TileSizeInPixels, TileSizeInPixels);
            v2 BottomRight = ScreenP + 0.5f*V2(TileSizeInPixels, TileSizeInPixels);
            v3 Color = {};
            DrawRectangleOutline(Bitmap, TopLeft, BottomRight, Color );
        }
    }
}

internal void
DrawGroundBuffers(game_offscreen_bitmap *Bitmap, game_state *GameState,
                  transient_state *TranState, tile_map_position Origin)
{
    
    tile_map *TileMap = &GameState->TileMap;
    ground_buffer *Buffers = TranState->GroundBuffers;
    loaded_bitmap *Template = &TranState->GroundBitmapTemplate;
    
    u32 TileSizeInPixels = TileMap->TileSizeInPixels;
    r32 PixelsPerMeter = GameState->PixelsPerMeter;

    v2 ScreenCenter = 0.5f * V2(Bitmap->Width, Bitmap->Height);
    
    v2 ScreenDimInMeters = GameState->MetersPerPixel * V2(Bitmap->Width, Bitmap->Height);
    rectangle2 ScreenBounds = RectCentDim(V2(0,0), ScreenDimInMeters);

    tile_map_position MinTile = MapIntoTileSpace(TileMap, Origin, ScreenBounds.Min);
    tile_map_position MaxTile = MapIntoTileSpace(TileMap, Origin, ScreenBounds.Max);

    ground_buffer *EmptyGroundBuffer = 0;
    for (s32 Y = MinTile.Y; Y <= MaxTile.Y; ++Y)
    {
        for (s32 X = MinTile.X; X <= MaxTile.X; ++X)
        {
            tile_map_position Position = {X, Y};
            bool32 NotFound = true;
            for (u32 Index = 0; Index < TranState->GroundBufferCount; ++Index)
            {
                ground_buffer *GroundBuffer = Buffers + Index;
                
                if (IsInSameTile(Position, GroundBuffer->P))
                {
                    NotFound = false;
                    break;
                }
                else if (!IsValid(GroundBuffer->P))
                {
                    EmptyGroundBuffer = GroundBuffer;
                }
            }
            if (EmptyGroundBuffer && NotFound)
            {
                EmptyGroundBuffer->P = Position;
                Template->Bytes = EmptyGroundBuffer->Bitmap;
                DrawTileGround(GameState, Template, Position);
                EmptyGroundBuffer = 0;
            }
        }
    }
}



internal void
InitializeGameState(game_state *GameState, game_offscreen_bitmap *Video, game_memory *Memory)
{
    
        loaded_bitmap HeroFacingRight = LoadBitmap(Memory->ReadFileToMemory, "car_right.bmp");
        GameState->HeroFacingRight.Body = HeroFacingRight;
        loaded_bitmap HeroFacingLeft = LoadBitmap(Memory->ReadFileToMemory, "car_left.bmp");
        GameState->HeroFacingLeft.Body = HeroFacingLeft;

        
        GameState->Grass[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/grass00.bmp");
        GameState->Grass[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/grass01.bmp");
        
        GameState->Ground[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground00.bmp");
        GameState->Ground[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground01.bmp");
        GameState->Ground[2] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground02.bmp");
        GameState->Ground[3] = LoadBitmap(Memory->ReadFileToMemory, "test2/ground03.bmp");
        
        GameState->Tree[0] = LoadBitmap(Memory->ReadFileToMemory, "test2/tree00.bmp");
        GameState->Tree[1] = LoadBitmap(Memory->ReadFileToMemory, "test2/tree01.bmp");
        GameState->Tree[2] = LoadBitmap(Memory->ReadFileToMemory, "test2/tree02.bmp");
        
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
                {0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                {1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
                {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                {1, 0, 0, 0, 1, 0, 1, 0, 0, 1},
                {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
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
                    //TODO Do we need wall size here?
                    v2 Delta = {GameState->WallSize * j, GameState->WallSize * i};
                    P = MapIntoTileSpace(TileMap, P, Delta);
                    AddWall(GameState, P);
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

        r32 ScreenOverDraw = 1.5f;
        u32 GroundWidth = RoundR32ToU32((r32)Video->Width * ScreenOverDraw);
        u32 GroundHeight = RoundR32ToU32((r32)Video->Height * ScreenOverDraw);
        GameState->GroundBitmap = MakeEmptyBitmap(&GameState->Arena, GroundWidth, GroundHeight);
        DrawTestGround(GameState, &GameState->GroundBitmap);
        GameState->GroundP = GameState->CameraPosition;

        GameState->IsInitialized = true;
}


internal void
InitializeTranState(transient_state *TranState, game_state *GameState,
                    game_offscreen_bitmap *Video, game_memory *Memory)
{
    InitializeArena(&TranState->Arena,
                    Memory->TransientStorageSize - sizeof(transient_state),
                    (u8 *)Memory->TransientStorage + sizeof(transient_state));

    TranState->GroundBufferCount = 32;
    TranState->GroundBuffers = PushArray(&TranState->Arena, TranState->GroundBufferCount, ground_buffer);

    u32 GroundWidth = GameState->TileMap.TileSizeInPixels;
    u32 GroundHeight = GroundWidth;
    loaded_bitmap *Template = &TranState->GroundBitmapTemplate;
        
    for(u32 Index = 0; Index < TranState->GroundBufferCount; ++Index)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + Index;
        *Template = MakeEmptyBitmap(&TranState->Arena, GroundWidth, GroundHeight, false);
        GroundBuffer->Bitmap = Template->Bytes;
        GroundBuffer->P = NullPosition();
    }

    TranState->IsInitialized = true;    
}

extern "C" void
GameEngine(game_memory *Memory, game_input *Input,
           game_sound_output_buffer *Sound,
           game_offscreen_bitmap *Video)
{
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
        ConHero->ddP.X += 1.0f;               
    }
    if (Keyboard->Left.IsDown)
    {
        ConHero->ddP.X += -1.0f;
    }
    if (Keyboard->Down.IsDown)
    {
        ConHero->ddP.Y += -1.0f;
    }
    if (Keyboard->Up.IsDown)
    {
        ConHero->ddP.Y += 1.0f;
    }


    v2 ScreenCenter = 0.5f * V2(Video->Width, Video->Height);

//    ClearScreen(Video);
    
    v2 GroundBitmapDim  = V2(TranState->GroundBitmapTemplate.Width,
                             TranState->GroundBitmapTemplate.Height);

    for (u32 Index = 0; Index < TranState->GroundBufferCount; ++Index)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + Index;
        
        if (IsValid(GroundBuffer->P))
        {

            v2 P = ScreenCenter - 0.5f *GroundBitmapDim;
            v2 Offset = SubtractPosition(&GameState->TileMap, GroundBuffer->P, GameState->CameraPosition);
            Offset *= GameState->PixelsPerMeter;
            Offset.Y = -Offset.Y;
            P += Offset;

            loaded_bitmap *Template = &TranState->GroundBitmapTemplate;
            Template->Bytes = GroundBuffer->Bitmap;
            
            //TODO Make CompositeBitmap 
            DrawBitmap(Video, Template, P.X, P.Y, 1.0f);
            //CompositeBitmap(Video, Template, P.X, P.Y, 1.0f);
        }
    
    }
    
    tile_map *TileMap = &GameState->TileMap;
    v2 CameraDim = TileMap->TileSizeInMeters * v2{(r32)Video->Width, (r32)Video->Height};
    rectangle2 CameraBounds = RectCentDim(v2{0,0}, CameraDim);


    memory_arena *SimArena = &TranState->Arena;
    temporary_memory TempMemory = BeginTemporaryMemory(SimArena);
    sim_region *SimRegion = BeginSim(SimArena, GameState, GameState->CameraPosition, CameraBounds);

    entity_visible_piece_group PieceGroup;
    PieceGroup.GameState = GameState;
    sim_entity *Entity = SimRegion->Entities;
    for (uint32 EntityIndex = 0;
         EntityIndex < SimRegion->EntityCount;
         ++EntityIndex, ++Entity)
    {
        if (!Entity->Updatable)
        {
            continue;
        }
        
        PieceGroup.PieceCount = 0;

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
                
                if((ConHero->dSword.X != 0.0f) || (ConHero->dSword.Y != 0.0f))
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
                PushRect(&PieceGroup, v2{Entity->Width, Entity->Height}, Color);
                PushBitmap(&PieceGroup, &HeroBitmaps->Body, -HeroBitmaps->Body.Offset);
                DrawHitpoints(&PieceGroup, Entity);

            } break;

            case EntityType_Wall:
            {
                v4 Color = {0.2f, 0.7f, 0.7f, 1.0f};
                PushRect(&PieceGroup, v2{Entity->Width, Entity->Height}, Color);
            } break;

            default:
            {
                INVALID_CODE_PATH;
            } break;

        }

        if (IsSet(Entity, EntityFlag_Moves))
        {
            MoveEntity(SimRegion, Entity, dt, &MoveSpec, ddP);
        }

        r32 PixelsPerMeter = GameState->PixelsPerMeter;

        //TODO FIX Y invert
        v2 EntityGroundPoint = ScreenCenter + PixelsPerMeter * v2{Entity->P.X, -Entity->P.Y};
        
        for (u32 PieceIndex = 0; PieceIndex < PieceGroup.PieceCount; ++PieceIndex)
        {
            entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
            v2 Center = EntityGroundPoint + Piece->Offset;
            if (Piece->Bitmap)
            {
                DrawBitmap(Video, Piece->Bitmap,
                           Center.X,
                           Center.Y,
                           Piece->A);
            }
            else
            {
                v2 HalfDim = 0.5f * Piece->Dim;
                DrawRectangle(Video, Center - HalfDim, Center + HalfDim,
                              Piece->R, Piece->G, Piece->B);
            }
        }
    }
    DrawTilesOutline(Video, GameState, GameState->CameraPosition);
    
    DrawGroundBuffers(Video, GameState, TranState, GameState->CameraPosition);


    EndSim(SimRegion, GameState);
    EndTemporaryMemory(&TempMemory);
    DrawRectangleOutline(Video, V2(50,50), V2(80,80), V3(0, 0, 0));
    DrawRectangle(Video, V2(0,0), V2(50,50), 0.0f, 0.0f, 0.0f);
}





 

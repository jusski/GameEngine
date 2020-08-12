#include "render_group.h"
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

#define PushRenderEntry(Group, type) (type *)PushRenderEntry_(Group, sizeof(type), RenderGroupEntryType_##type)

internal entry_header *
PushRenderEntry_(render_group *Group, u32 Size, render_group_entry_type Type)
{
    Assert((Group->PushBufferSize + Size) < Group->MaxPushBufferSize);

    entry_header *Entry = (entry_header *)Group->PushBufferBasis + Group->PushBufferSize;
    Entry->Type = Type;
    Group->PushBufferSize += Size; 

    return(Entry);
    
}

internal void
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v2 Offset, r32 Alpha = 1.0f)
{
    Assert(Bitmap);
    render_entry_bitmap *Entry = PushRenderEntry(Group, render_entry_bitmap);

    Entry->Bitmap = Bitmap;
    Entry->Alpha = Alpha;

    r32 PixelsPerMeter = Group->GameState->PixelsPerMeter;
    Offset.Y = -Offset.Y;
    v2 Align = Bitmap->Align;
    Entry->Offset = Offset * PixelsPerMeter - Align; 
}

internal void
PushRect(render_group *Group, v2 Offset, v2 Dim, v3 Color)
{
    render_entry_rect *Entry = PushRenderEntry(Group, render_entry_rect);

    Entry->Color = Color;
    
    r32 PixelsPerMeter = Group->GameState->PixelsPerMeter;
    Offset.Y = -Offset.Y;
    Entry->Offset = Offset * PixelsPerMeter;

    Entry->Dim = PixelsPerMeter * Dim;
}

internal void
DrawRectangle(game_offscreen_bitmap *Bitmap, v2 TopLeft, v2 BottomRight, v3 Color)
{
    Assert(Color.R >= 0 && Color.R <= 1.0f);
    Assert(Color.G >= 0 && Color.G <= 1.0f);
    Assert(Color.B >= 0 && Color.B <= 1.0f);

    TopLeft.X = (TopLeft.X < 0) ? 0 : TopLeft.X;
    TopLeft.Y = (TopLeft.Y < 0) ? 0 : TopLeft.Y;
    BottomRight.X = (BottomRight.X > Bitmap->Width) ? Bitmap->Width : BottomRight.X;
    BottomRight.Y = (BottomRight.Y > Bitmap->Height) ? Bitmap->Height : BottomRight.Y;

    s32 MinX = (s32)(TopLeft.X);
    s32 MinY = (s32)(TopLeft.Y);
    s32 MaxX = (s32)(BottomRight.X);
    s32 MaxY = (s32)(BottomRight.Y);

    u32 ColorBGR =
        RoundR32ToU32(Color.B * 255.0f) << 0 |
        RoundR32ToU32(Color.G * 255.0f) << 8 |
        RoundR32ToU32(Color.R * 255.0f) << 16;
    
    
    u32 *Row = (u32 *)Bitmap->Memory + Bitmap->Width*MinY + MinX;
    for (s32 y = MinY; y < MaxY; y++)
    {
        u32 *Pixel = Row;
        for (s32 x = MinX; x < MaxX; x++)
        {
            *Pixel++ = ColorBGR;
        }
        Row += Bitmap->Width ;
    }
    
}

internal void
DrawRectangle(game_offscreen_bitmap *Bitmap,
              r32 TopLeftX, r32 TopLeftY,
              r32 BottomRightX, r32 BottomRightY,
              v3 Color)
{
    v2 TopLeft = {TopLeftX, TopLeftY};
    v2 BottomRight = {BottomRightX, BottomRightY};
    DrawRectangle(Bitmap, TopLeft, BottomRight, Color);
}

internal render_group *
AllocateRenderGroup(memory_arena *Arena, game_state *GameState,
                    u32 MaxPushBufferSize)
{
    render_group *Result = PushStruct(Arena, render_group);
    Result->PushBufferBasis = PushSize(Arena, MaxPushBufferSize);
    Result->MaxPushBufferSize = MaxPushBufferSize;
    Result->PushBufferSize = 0;

    Result->GameState = GameState;

    return(Result);
}

internal void
RenderOutput(render_group *Group, game_offscreen_bitmap *Target,
             r32 PixelsPerMeter )
{
    v2 ScreenCenter = 0.5f * V2(Target->Width, Target->Height);

    for (u32 Address = 0; Address < Group->PushBufferSize;)
    {
       entry_header *Header = (entry_header *)Group->PushBufferBasis + Address;
        
        switch(Header->Type)
        {
            case(RenderGroupEntryType_render_entry_bitmap):
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *) Header;
                Assert(Entry->Bitmap);
                
                v2 P = ScreenCenter + Entry->Offset;
                DrawBitmap(Target, Entry->Bitmap, P.X, P.Y, Entry->Alpha);

                Address += sizeof(*Entry);
            } break;
            
            case(RenderGroupEntryType_render_entry_rect):
            {
                render_entry_rect *Entry = (render_entry_rect *) Header;
                v2 P = ScreenCenter + Entry->Offset;
                v2 HalfDim = 0.5f * Entry->Dim;
         
                DrawRectangle(Target, P - HalfDim, P + HalfDim, Entry->Color);

                Address += sizeof(*Entry);
            } break;
            
            InvalidDefaultCase;
        }
    }
}
 

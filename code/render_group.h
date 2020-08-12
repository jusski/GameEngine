#pragma once

enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_invalid,
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_rect,
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_composite,
    
};

struct entry_header
{
    render_group_entry_type Type;
};

struct render_entry_clear
{
    entry_header Header;
    v3 Color;
    
};

struct render_entry_rect
{
    entry_header Header;
    v2 Offset;
    v3 Color;
    v2 Dim;
};

struct render_entry_bitmap
{
    entry_header Header;
    loaded_bitmap *Bitmap;
    v2 Offset;
    r32 Alpha;
};

struct render_entry_composite
{
    entry_header Header;
    loaded_bitmap *Source;
    loaded_bitmap *Destination;
    v2 P;
    
    v2 Offset;
    r32 Alpha;
};

struct render_entry
{
    entry_header Header;

};

struct render_group
{
    game_state *GameState;
    
    u32 MaxPushBufferSize;
    u32 PushBufferSize;
    u8 *PushBufferBasis;
};


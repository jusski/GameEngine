#include "render_group.h"

internal bilinear_sample
BilinearSample(u32 *TexelPtr, u32 Pitch)
{
    bilinear_sample Result;
    
    u32 *TexelAPtr = TexelPtr;
    u32 *TexelBPtr = TexelPtr + 1;
    u32 *TexelCPtr = TexelPtr + Pitch;
    u32 *TexelDPtr = TexelPtr + Pitch + 1;

    Result.A = UnpackBGRA(TexelAPtr);
    Result.B = UnpackBGRA(TexelBPtr);
    Result.C = UnpackBGRA(TexelCPtr);
    Result.D = UnpackBGRA(TexelDPtr);

    return(Result);
}

internal v4
BilinearLerp(bilinear_sample &Texel, r32 s, r32 t)
{
    v4 TexelA = SRGBA255ToLinear1(Texel.A);
    v4 TexelB = SRGBA255ToLinear1(Texel.B);
    v4 TexelC = SRGBA255ToLinear1(Texel.C);
    v4 TexelD = SRGBA255ToLinear1(Texel.D);

    v4 Blend = Lerp(Lerp(TexelA, s, TexelB), t,
                    Lerp(TexelC, s, TexelD));

    return(Blend);
}

internal v4
SampleEnviromentMap(enviroment_map *Map, v4 Normal, v2 ScreenPosition,
                    v4 DefaultColor)
{
    //ScreenPosition = V2(0.5f, -0.5f);
    loaded_bitmap *Texture = Map->LOD[0];
    
    Normal.xyz = Normalize(Normal.xyz);
    // Reflection vector = -e - 2 * (e,N) * N  
    v3 Reflection = V3(0,0,-1) + 2.0f * Normal.z * Normal.xyz;
    
#if 0
    v4 DebugColor = V4(1,1,1,1);
    DebugColor.xyz = 0.5f*DebugColor.xyz + 0.5f*Reflection;
    return(DebugColor);
#else
    r32 Distance = 100.f + 1.0f - Normal.w;
    r32 Ratio = SafeRatio0(Distance, Reflection.z);
    if (Reflection.z < 0.0f)
    {
        return DefaultColor;
    }

    r32 s = Ratio * Reflection.x;
    r32 t = Ratio * Reflection.y;
    
    r32 X = ScreenPosition.x + s;
    r32 Y = ScreenPosition.y + t;
    if ((X > (r32)Texture->Width - 2.0f) ||
        (Y > (r32)Texture->Height - 2.0f) ||
        (X < 0.0f) || (Y < 0.0f))
    {
        return DefaultColor;
    }

    u32 x = (u32)X;
    u32 y = (u32)Y;
    
    r32 fX = X - (r32)x;
    r32 fY = Y - (r32)y;

    Assert(X <= (Texture->Width - 2));
    Assert(Y <= (Texture->Height - 2));
    u32 *TexelPtr = (u32 *)Texture->Bytes + y * Texture->Width + x;

    bilinear_sample Sample = BilinearSample(TexelPtr, Texture->Width);
    v4 BlendedLight = BilinearLerp(Sample, fX, fY);
    
    return(BlendedLight);
#endif
}

internal void
CompositeBitmap(loaded_bitmap *Destination, loaded_bitmap *Source,
                v2 TopLeft, r32 Alpha = 1.0f)
{

    s32 ClipX = (TopLeft.x < 0) ? (s32)-TopLeft.x : 0;
    s32 ClipY = (TopLeft.y < 0) ? (s32)-TopLeft.y : 0;

    s32 BottomRightX = (s32)TopLeft.x + Source->Width;
    s32 BottomRightY = (s32)TopLeft.y + Source->Height;
    BottomRightX = (BottomRightX > (s32)Destination->Width) ? Destination->Width : BottomRightX;
    BottomRightY = (BottomRightY > (s32)Destination->Height) ? Destination->Height : BottomRightY;

    TopLeft.x = (TopLeft.x < 0) ? 0 : TopLeft.x;
    TopLeft.y = (TopLeft.y < 0) ? 0 : TopLeft.y;
    
    s32 MinX = (s32)(TopLeft.x);
    s32 MinY = (s32)(TopLeft.y);
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

            u32 R = DR + SR-(DR*SA)/255;
            u32 G = DG + SG-(DG*SA)/255;
            u32 B = DB + SB-(DB*SA)/255;

            u32 A = (u32)(AN * 255.0f);
            
            *DestinationPixel++ = (A << 24) | (R << 16) | (G << 8) | (B << 0);
            SourcePixel++;
        }
        DestinationRow += Destination->Width;
        SourceRow += Source->Width;
    }
    
}

internal void
DrawBitmap(loaded_bitmap *Destination, loaded_bitmap *Source,
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

    Assert(MaxX - MinX <= (s32)Source->Width);
    Assert(MaxY - MinY <= (s32)Source->Height);
    u32 *DestinationRow = (u32 *)Destination->Bytes + Destination->Width*MinY + MinX;
    u32 *SourceRow = (u32 *)Source->Bytes;// + Source->Width * (Source->Height - 1);
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

            u32 R = DR + SR-(DR*SA)/255;
            u32 G = DG + SG-(DG*SA)/255;
            u32 B = DB + SB-(DB*SA)/255;

            *DestinationPixel++ = (R << 16) | (G << 8) | (B << 0);
            SourcePixel++;
        }
        DestinationRow += Destination->Width;
        SourceRow += Source->Width;
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

    if(Entry)
    {
        Entry->Bitmap = Bitmap;
        Entry->Alpha = Alpha;

        r32 PixelsPerMeter = Group->GameState->PixelsPerMeter;
        Offset.y = Offset.y;
        Entry->Offset = Offset * PixelsPerMeter - Bitmap->Align;
    }
}

internal void
PushCompositeBitmap(render_group *Group, loaded_bitmap *Destination,
                    loaded_bitmap *Source, v2 P, r32 Alpha = 1.0f)
{
    Assert(Source);
    Assert(Destination);
    render_entry_composite *Entry = PushRenderEntry(Group, render_entry_composite);

    if(Entry)
    {
        Entry->Source = Source;
        Entry->Destination = Destination;
        Entry->Alpha = Alpha;
        Entry->P = P;
    }
}

internal void
PushRect(render_group *Group, v2 Offset, v2 Dim, v4 Color)
{
    render_entry_rect *Entry = PushRenderEntry(Group, render_entry_rect);

    if(Entry)
    {
        Entry->Color = Color;
    
        r32 PixelsPerMeter = Group->GameState->PixelsPerMeter;
        Offset.y = Offset.y;
        Entry->Offset = Offset * PixelsPerMeter;

        Entry->Dim = PixelsPerMeter * Dim;
    }
}

internal void
PushRectSlow(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color)
{
    render_entry_rect_slow *Entry = PushRenderEntry(Group, render_entry_rect_slow);

    if(Entry)
    {
        Entry->Color = Color;
        Entry->Origin = Origin;
        Entry->XAxis = XAxis;
        Entry->YAxis = YAxis;
    }    
}

internal void
PushTextureSlow(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis,
                loaded_bitmap *Texture, normal_map *NormalMap,
                v4 Color, enviroment_map *Sky, enviroment_map *Ground,
                v2 ScreenPosition)
{
    render_entry_tex_slow *Entry = PushRenderEntry(Group, render_entry_tex_slow);

    if(Entry)
    {
        Entry->Origin = Origin;
        Entry->XAxis = XAxis;
        Entry->YAxis = YAxis;
        Entry->Texture = Texture;
        Entry->NormalMap = NormalMap;
        Entry->Color = Color;
        Entry->Sky = Sky;
        Entry->Ground = Ground;
        Entry->ScreenPosition = ScreenPosition;
    }    
}

internal void
PushClear(render_group *Group, v4 Color)
{
    render_entry_clear *Entry = PushRenderEntry(Group, render_entry_clear);

    if(Entry)
    {
        Entry->Color = Color;
    }
}

internal void
DrawRectangle(loaded_bitmap *Bitmap, v2 TopLeft, v2 BottomRight, v4 Color)
{
    Assert(Color.r >= 0 && Color.r <= 1.0f);
    Assert(Color.g >= 0 && Color.g <= 1.0f);
    Assert(Color.b >= 0 && Color.b <= 1.0f);
    Assert(Color.a >= 0 && Color.a <= 1.0f);

    TopLeft.x = (TopLeft.x < 0) ? 0 : TopLeft.x;
    TopLeft.y = (TopLeft.y < 0) ? 0 : TopLeft.y;
    BottomRight.x = (BottomRight.x > Bitmap->Width) ? Bitmap->Width : BottomRight.x;
    BottomRight.y = (BottomRight.y > Bitmap->Height) ? Bitmap->Height : BottomRight.y;

    s32 MinX = (s32)(TopLeft.x);
    s32 MinY = (s32)(TopLeft.y);
    s32 MaxX = (s32)(BottomRight.x);
    s32 MaxY = (s32)(BottomRight.y);

    u32 ColorBGR =
        RoundR32ToU32(Color.b * 255.0f) << 0  |
        RoundR32ToU32(Color.g * 255.0f) << 8  |
        RoundR32ToU32(Color.r * 255.0f) << 16 |
        RoundR32ToU32(Color.a * 255.0f) << 24;
    
    
    u32 *Row = (u32 *)Bitmap->Bytes + Bitmap->Width*MinY + MinX;
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
DrawRectangleSlowly(loaded_bitmap *Destination, v2 Origin,
                    v2 XAxis, v2 YAxis, v4 Color)
{
    Assert(Color.r >= 0 && Color.r <= 1.0f);
    Assert(Color.g >= 0 && Color.g <= 1.0f);
    Assert(Color.b >= 0 && Color.b <= 1.0f);

    u32 ColorBGRA =
        RoundR32ToU32(Color.b * 255.0f) << 0  |
        RoundR32ToU32(Color.g * 255.0f) << 8  |
        RoundR32ToU32(Color.r * 255.0f) << 16 |
        RoundR32ToU32(Color.a * 255.0f) << 24;

    

    v2 P[4];
    P[0] = Origin;
    P[1] = Origin + XAxis;
    P[2] = Origin + XAxis + YAxis;
    P[3] = Origin + YAxis;

    s32 MinX = Destination->Width;
    s32 MaxX = 0;
    s32 MinY = Destination->Height;
    s32 MaxY = 0;
    
    for (u32 i = 0 ; i < ArrayCount(P); ++i)
    {
        MinX = Minimum(MinX, Floor(P[i].x));
        MinY = Minimum(MinY, Floor(P[i].y));

        MaxX = Maximum(MaxX, Ceill(P[i].x));
        MaxY = Maximum(MaxY, Ceill(P[i].y));
        
    }

    MinX = Maximum(0, MinX);
    MinY = Maximum(0, MinY);
    MaxX = Minimum((s32)Destination->Width - 1, MaxX);
    MaxY = Minimum((s32)Destination->Height - 1, MaxY);
    
    for (s32 y = MinY; y <= MaxY; ++y)
    {
        for (s32 x = MinX; x <= MaxX; ++x)
        {
            u32 *Pixel = (u32 *)Destination->Bytes + y * Destination->Width + x;
            
            v2 PixelP = V2(x, y);
            v2 d = PixelP - Origin;

            r32 Edge0 = d * -XAxis;
            r32 Edge1 = d * -YAxis;
            r32 Edge2 = (d - XAxis) * XAxis;
            r32 Edge3 = (d - YAxis) * YAxis;


            if ((Edge0 < 0) &&
                (Edge1 < 0) &&
                (Edge2 < 0) &&
                (Edge3 < 0))
            {
                *Pixel = ColorBGRA;    
            }
        } 
    }
        
}

internal void
DrawTextureSlowly(loaded_bitmap *Destination, v2 Origin,
                  v2 XAxis, v2 YAxis, loaded_bitmap *Texture,
                  normal_map *NormalMap, v4 Color,
                  enviroment_map *Sky, enviroment_map *Ground,
                  v2 ScreenPosition)
{

    
    v2 P[4];
    P[0] = Origin;
    P[1] = Origin + XAxis;
    P[2] = Origin + XAxis + YAxis;
    P[3] = Origin + YAxis;

    s32 MinX = Destination->Width;
    s32 MaxX = 0;
    s32 MinY = Destination->Height;
    s32 MaxY = 0;
    
    for (u32 i = 0 ; i < ArrayCount(P); ++i)
    {
        MinX = Minimum(MinX, Floor(P[i].x));
        MinY = Minimum(MinY, Floor(P[i].y));

        MaxX = Maximum(MaxX, Ceill(P[i].x));
        MaxY = Maximum(MaxY, Ceill(P[i].y));
        
    }

    MinX = Maximum(0, MinX);
    MinY = Maximum(0, MinY);
    MaxX = Minimum((s32)Destination->Width - 1, MaxX);
    MaxY = Minimum((s32)Destination->Height - 1, MaxY);

    r32 InvXAxisLenSq = 1.0f / LengthSquared(XAxis);
    r32 InvYAxisLenSq = 1.0f / LengthSquared(YAxis);
    u32 TexWidth = Texture->Width;
    u32 TexHeight = Texture->Height;
    for (s32 y = MinY; y <= MaxY; ++y)
    {
        for (s32 x = MinX; x <= MaxX; ++x)
        {
            u32 *PixelPtr = (u32 *)Destination->Bytes + y * Destination->Width + x;
            
            v2 PixelP = V2(x, y);
            v2 d = PixelP - Origin;

            r32 Edge0 = d * -XAxis;
            r32 Edge1 = d * -YAxis;
            r32 Edge2 = (d - XAxis) * XAxis;
            r32 Edge3 = (d - YAxis) * YAxis;

            if (Edge0 <= 0 &&
                Edge1 <= 0 &&
                Edge2 <= 0 &&
                Edge3 <= 0)
            {
                BEGIN_TIMED_BLOCK(PixelHit);
                d -= YAxis;
                r32 U = (d * XAxis) * InvXAxisLenSq;
                r32 V = (d * -YAxis) * InvYAxisLenSq;
                
                //Assert((U >= 0.0f) && (U <= 1.0f));
                //Assert((V >= 0.0f) && (V <= 1.0f));
                
                r32 TexX = U * (r32)(TexWidth - 2);
                r32 TexY = V * (r32)(TexHeight - 2);

                u32 X = (u32)TexX;
                u32 Y = (u32)TexY;

                r32 fX = TexX - (r32)X;
                r32 fY = TexY - (r32)Y;
                
                u32 *TexelPtr = (u32 *)Texture->Bytes + Y * TexWidth + X;

                u32 *TexelAPtr = TexelPtr;
                u32 *TexelBPtr = TexelPtr + 1;
                u32 *TexelCPtr = TexelPtr + TexWidth;
                u32 *TexelDPtr = TexelPtr + TexWidth + 1;

                v4 TexelA = UnpackBGRA(TexelAPtr);
                v4 TexelB = UnpackBGRA(TexelBPtr);
                v4 TexelC = UnpackBGRA(TexelCPtr);
                v4 TexelD = UnpackBGRA(TexelDPtr);
                
                TexelA = SRGBA255ToLinear1(TexelA);
                TexelB = SRGBA255ToLinear1(TexelB);
                TexelC = SRGBA255ToLinear1(TexelC);
                TexelD = SRGBA255ToLinear1(TexelD);

                v4 Texel = Lerp(Lerp(TexelA, fX, TexelB), fY,
                                Lerp(TexelC, fX, TexelD));

                if(NormalMap)
                {
                    v4 *NormalPtr = (v4 *)NormalMap->Bytes + Y * NormalMap->Width + X;

                    v4 *NormalAPtr = NormalPtr;
                    v4 *NormalBPtr = NormalPtr + 1;
                    v4 *NormalCPtr = NormalPtr + NormalMap->Width;
                    v4 *NormalDPtr = NormalPtr + NormalMap->Width + 1;

                    v4 NormalA = *NormalAPtr;
                    v4 NormalB = *NormalBPtr;
                    v4 NormalC = *NormalCPtr;
                    v4 NormalD = *NormalDPtr;

                    v4 Normal = Lerp(Lerp(NormalA, fX, NormalB), fY,
                                     Lerp(NormalC, fX, NormalD));
                    
                    v2 NXAxis = 1.f/Length(XAxis) * XAxis;
                    v2 NYAxis = 1.f/Length(YAxis) * YAxis;
                
                    Normal.xy =  Normal.x*(NXAxis) - Normal.y*(NYAxis);
                
                         
                    v4 Light = SampleEnviromentMap(Sky, Normal,
                                                   ScreenPosition + 1.0f*V2(x,y),
                                                   V4(1,1,1,1));

                    Texel.rgb = Texel.rgb +
                        Texel.a* Lerp(Texel.rgb, Normal.w, Light.rgb);
                }
                
                Texel = Hadamard(Texel, Color);

                v4 Pixel = UnpackBGRA(PixelPtr);
                Pixel = SRGBA255ToLinear1(Pixel);
                
                r32 InvA = 1.0f - Texel.a;
                v4 Blended = InvA * Pixel + Texel;
                Blended = Linear1ToSRGBA255(Blended);

                *PixelPtr = PackBGRA(Blended);
                END_TIMED_BLOCK(PixelHit);
            }
        } 
    }
        
}

internal void
DrawTextureQuick(loaded_bitmap *Destination, v2 Origin,
                  v2 XAxis, v2 YAxis, loaded_bitmap *Texture,
                  v4 Color)
{

    __m128 Value = _mm_set1_ps(1.0f);
    
    v2 P[4];
    P[0] = Origin;
    P[1] = Origin + XAxis;
    P[2] = Origin + XAxis + YAxis;
    P[3] = Origin + YAxis;

    s32 MinX = Destination->Width;
    s32 MaxX = 0;
    s32 MinY = Destination->Height;
    s32 MaxY = 0;
    
    for (u32 i = 0 ; i < ArrayCount(P); ++i)
    {
        MinX = Minimum(MinX, Floor(P[i].x));
        MinY = Minimum(MinY, Floor(P[i].y));

        MaxX = Maximum(MaxX, Ceill(P[i].x));
        MaxY = Maximum(MaxY, Ceill(P[i].y));
        
    }

    MinX = Maximum(0, MinX);
    MinY = Maximum(0, MinY);
    MaxX = Minimum((s32)Destination->Width - 1, MaxX);
    MaxY = Minimum((s32)Destination->Height - 1, MaxY);

    r32 InvXAxisLenSq = 1.0f / LengthSquared(XAxis);
    r32 InvYAxisLenSq = 1.0f / LengthSquared(YAxis);
    u32 TexWidth = Texture->Width;
    u32 TexHeight = Texture->Height;
    BEGIN_TIMED_BLOCK(PixelHit);
    for (s32 y = MinY; y <= MaxY; ++y)
    {
        for (s32 x = MinX; x <= MaxX; ++x)
        {
            u32 *PixelPtr = (u32 *)Destination->Bytes + y * Destination->Width + x;
            
            //v2 PixelP = V2(x, y);
            //v2 d = PixelP - Origin;

            f32 Px = (f32)x;
            f32 Py = (f32)y;
            f32 dx = Px - Origin.x;
            f32 dy = Py - Origin.y;
           
            //r32 U = (d * XAxis) * InvXAxisLenSq;
            //r32 V = (d * YAxis) * InvYAxisLenSq;

            f32 U = (dx * XAxis.x + dy * XAxis.y) * InvXAxisLenSq;
            f32 V = (dx * YAxis.x + dy * YAxis.y) * InvYAxisLenSq;

            if (U >= 0.0f &&
                U <= 1.0f &&
                V >= 0.0f &&
                V <= 1.0f)
            {
                
                //Assert((U >= 0.0f) && (U <= 1.0f));
                //Assert((V >= 0.0f) && (V <= 1.0f));
                
                f32 TexX = U * (f32)(TexWidth - 2);
                f32 TexY = V * (f32)(TexHeight - 2);

                u32 X = (u32)TexX;
                u32 Y = (u32)TexY;

                f32 fX = TexX - (f32)X;
                f32 fY = TexY - (f32)Y;
                
                u32 *TexelPtr = (u32 *)Texture->Bytes + Y * TexWidth + X;

                u32 *TexelAPtr = TexelPtr;
                u32 *TexelBPtr = TexelPtr + 1;
                u32 *TexelCPtr = TexelPtr + TexWidth;
                u32 *TexelDPtr = TexelPtr + TexWidth + 1;

                //v4 TexelA = UnpackBGRA(TexelAPtr);
                //v4 TexelB = UnpackBGRA(TexelBPtr);
                //v4 TexelC = UnpackBGRA(TexelCPtr);
                //v4 TexelD = UnpackBGRA(TexelDPtr);
                // Unpack A B C D
                f32 TexelAr = (f32)((*TexelAPtr >> 16) & 0xFF);
                f32 TexelAg = (f32)((*TexelAPtr >> 8)  & 0xFF);
                f32 TexelAb = (f32)((*TexelAPtr >> 0)  & 0xFF);
                f32 TexelAa = (f32)((*TexelAPtr >> 24) & 0xFF);

                f32 TexelBr = (f32)((*TexelBPtr >> 16) & 0xFF);
                f32 TexelBg = (f32)((*TexelBPtr >> 8)  & 0xFF);
                f32 TexelBb = (f32)((*TexelBPtr >> 0)  & 0xFF);
                f32 TexelBa = (f32)((*TexelBPtr >> 24) & 0xFF);

                f32 TexelCr = (f32)((*TexelCPtr >> 16) & 0xFF);
                f32 TexelCg = (f32)((*TexelCPtr >> 8)  & 0xFF);
                f32 TexelCb = (f32)((*TexelCPtr >> 0)  & 0xFF);
                f32 TexelCa = (f32)((*TexelCPtr >> 24) & 0xFF);

                f32 TexelDr = (f32)((*TexelDPtr >> 16) & 0xFF);
                f32 TexelDg = (f32)((*TexelDPtr >> 8)  & 0xFF);
                f32 TexelDb = (f32)((*TexelDPtr >> 0)  & 0xFF);
                f32 TexelDa = (f32)((*TexelDPtr >> 24) & 0xFF);

                //TexelA = SRGBA255ToLinear1(TexelA);
                //TexelB = SRGBA255ToLinear1(TexelB);
                //TexelC = SRGBA255ToLinear1(TexelC);
                //TexelD = SRGBA255ToLinear1(TexelD);
                // SRGBA255 To Linear space
                f32 Inv255 = 1.0f/255.0f;

                TexelAr *= Inv255;
                TexelAg *= Inv255;
                TexelAb *= Inv255;
                TexelAa *= Inv255;

                TexelBr *= Inv255;
                TexelBg *= Inv255;
                TexelBb *= Inv255;
                TexelBa *= Inv255;

                TexelCr *= Inv255;
                TexelCg *= Inv255;
                TexelCb *= Inv255;
                TexelCa *= Inv255;

                TexelDr *= Inv255;
                TexelDg *= Inv255;
                TexelDb *= Inv255;
                TexelDa *= Inv255;

                TexelAr = Square(TexelAr);
                TexelAg = Square(TexelAg);
                TexelAb = Square(TexelAb);

                TexelBr = Square(TexelBr);
                TexelBg = Square(TexelBg);
                TexelBb = Square(TexelBb);

                TexelCr = Square(TexelCr);
                TexelCg = Square(TexelCg);
                TexelCb = Square(TexelCb);

                TexelDr = Square(TexelDr);
                TexelDg = Square(TexelDg);
                TexelDb = Square(TexelDb);
                
                

                //             v4 Texel = Lerp(Lerp(TexelA, fX, TexelB), fY,
                //              Lerp(TexelC, fX, TexelD));
                // Lerp A B C D
                f32 LerpABr = (1.0f - fX) * TexelAr + fX * TexelBr;
                f32 LerpABg = (1.0f - fX) * TexelAg + fX * TexelBg;
                f32 LerpABb = (1.0f - fX) * TexelAb + fX * TexelBb;
                f32 LerpABa = (1.0f - fX) * TexelAa + fX * TexelBa;
                
                f32 LerpCDr = (1.0f - fX) * TexelCr + fX * TexelDr;
                f32 LerpCDg = (1.0f - fX) * TexelCg + fX * TexelDg;
                f32 LerpCDb = (1.0f - fX) * TexelCb + fX * TexelDb;
                f32 LerpCDa = (1.0f - fX) * TexelCa + fX * TexelDa;

                f32 Texelr = (1.0f - fY) * LerpABr + fY * LerpCDr;
                f32 Texelg = (1.0f - fY) * LerpABg + fY * LerpCDg;
                f32 Texelb = (1.0f - fY) * LerpABb + fY * LerpCDb;
                f32 Texela = (1.0f - fY) * LerpABa + fY * LerpCDa;
                                
                //Texel = Hadamard(Texel, Color);

                //v4 Pixel = UnpackBGRA(PixelPtr);
                //Pixel = SRGBA255ToLinear1(Pixel);
                f32 Pixelr = (f32)((*PixelPtr >> 16) & 0xFF);
                f32 Pixelg = (f32)((*PixelPtr >> 8)  & 0xFF);
                f32 Pixelb = (f32)((*PixelPtr >> 0)  & 0xFF);
                f32 Pixela = (f32)((*PixelPtr >> 24) & 0xFF);

                Pixelr *= Inv255;
                Pixelg *= Inv255;
                Pixelb *= Inv255;
                Pixela *= Inv255;

                Pixelr = Square(Pixelr);
                Pixelg = Square(Pixelg);
                Pixelb = Square(Pixelb);

                
                f32 InvA = 1.0f - Texela;
                //v4 Blended = InvA * Pixel + Texel;
                f32 Blendedr = InvA * Pixelr + Texelr;
                f32 Blendedg = InvA * Pixelg + Texelg;
                f32 Blendedb = InvA * Pixelb + Texelb;
                f32 Blendeda = InvA * Pixela + Texela;
                
                //Blended = Linear1ToSRGBA255(Blended);
                f32 One255 = 255.0f;
                Blendedr = One255 * SquareRoot(Blendedr);
                Blendedg = One255 * SquareRoot(Blendedg);
                Blendedb = One255 * SquareRoot(Blendedb);
                Blendeda = One255 * Blendeda;

                //*PixelPtr = PackBGRA(Blended);
                        
                *PixelPtr = (
                    ((u32)Blendedr << 16) |
                    ((u32)Blendedg << 8)  |
                    ((u32)Blendedb << 0)  |
                    ((u32)Blendeda << 24));

            }
        } 
    }
    END_TIMED_BLOCK_COUNTED(PixelHit, ((MaxX-MinX)*(MaxY-MinY)));
        
}

internal void
MakeSphereNormalMap(normal_map *Map)
{
    u32 Width = Map->Width;
    u32 Height = Map->Height;
    
    for (u32 y = 0; y < Height; ++y)
    {
        for (u32 x = 0; x < Width; ++x)
        {
            v4 *Normal = (v4 *)Map->Bytes + y * Width + x;
            //Normalize
            r32 u = (r32)x / (Width - 1);
            r32 v = (r32)y / (Height - 1);

            //Scale to -1..1
            u = 2.0f * u - 1.0f;
            v = 2.0f * v - 1.0f;
            
            //Sphere equation is x^2 + y^2 + z^2 = 1
            r32 z = 1 - u*u - v*v;
            if (z >= 0.0f)
            {
                z = SquareRoot(z);

                *Normal = V4(u, v, z, z);

            }
            else
            {
                v4 DefaultNormal = V4(0.0f, 0.0f, 1.0f, 0.0f);
                *Normal = DefaultNormal;
            }
            
        }
    }
}

internal void
MakeTestNormalMap(normal_map *Map, r32 AngleDegrees = 90.f)
{
    u32 Width = Map->Width;
    u32 Height = Map->Height;
    
    for (u32 Y = 0; Y < Height; ++Y)
    {
        for (u32 X = 0; X < Width; ++X)
        {
            v4 *Normal = (v4 *)Map->Bytes + Y * Width + X;
            
            r32 AngleRadiance = AngleDegrees * (3.14f / 180.f);

            r32 x = (r32)X / (Width - 1);
            r32 y = 0.f;
            r32 z = x * Tan(AngleRadiance);
            r32 w = z;

            if (x*x + z*z > 1.0f)
            {
                *Normal = V4(0,0,0,0);
            }
            else
            {
                v4 n = V4(-z, 0.f, x, w);
                n.xyz = Normalize(n.xyz);
                *Normal = n;                
            }
            
            
        }
    }
}

internal void
MakePyramidNormalMap(loaded_bitmap *Bitmap)
{
    u32 Width = Bitmap->Width;
    u32 Height = Bitmap->Height;
    
    for (u32 y = 0; y < Height; ++y)
    {
        for (u32 x = 0; x < Width; ++x)
        {
            u32 *Pixel = (u32 *)Bitmap->Bytes + y * Width + x;
            //Normalize
            r32 u = (r32)x / (Width - 1);
            r32 v = (r32)y / (Height - 1);

            //Scale to -1..1
            u = 2.0f * u - 1.0f;
            v = 2.0f * v - 1.0f;

            v4 Normal = V4(0,0,0,0);
            if (u > v)
            {
                if (u > -v)
                {
                    Normal = V4(0.707106781187f, 0.0f, 0.707106781187f, 1.0f - u);
                }
                else
                {
                    Normal = V4(0.0f, -0.707106781187f, 0.707106781187f, 1.0f - v );
                }
                
            }
            else
            {
                if (-u > v)
                {
                    Normal = V4(-0.707106781187f, 0.0f, 0.707106781187f, 1.0f - u );
                }
                else
                {
                    Normal = V4(0.0f, 0.707106781187f, 0.707106781187f, 1.0f - v );
                }
            }
            //Scale to 0..1
            Normal.xyz = 0.5f * (Normal.xyz + V3(1.0f, 1.0f, 1.0f));
            *Pixel = PackBGRA(255.0f * Normal);
        }
    }
}

internal void
DrawRectangle(loaded_bitmap *Bitmap,
              r32 TopLeftX, r32 TopLeftY,
              r32 BottomRightX, r32 BottomRightY,
              v4 Color)
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
RenderOutput(render_group *Group, loaded_bitmap *Target,
             r32 PixelsPerMeter )
{
    TIMED_FUNCTION();
    
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
#if 0
                DrawBitmap(Target, Entry->Bitmap, P.x, P.y, Entry->Alpha);
#else
                v2 XAxis = (r32)Entry->Bitmap->Width * V2(1.f, 0.f);
                v2 YAxis = (r32)Entry->Bitmap->Height * V2(0.f, 1.f);//Perp(XAxis);
                DrawTextureQuick(Target, P, XAxis, YAxis,
                                  Entry->Bitmap, V4(1,1,1,1));                
#endif
                Address += sizeof(*Entry);
            } break;

            case(RenderGroupEntryType_render_entry_composite):
            {
                render_entry_composite *Entry = (render_entry_composite *) Header;
                Assert(Entry->Source);
                Assert(Entry->Destination);

                CompositeBitmap(Entry->Destination, Entry->Source, Entry->P);

                Address += sizeof(*Entry);
            } break;
            
            case(RenderGroupEntryType_render_entry_rect):
            {
                render_entry_rect *Entry = (render_entry_rect *) Header;
                v2 P = ScreenCenter + 2.f*Entry->Offset;
                v2 HalfDim = 0.5f * Entry->Dim;
         
                DrawRectangle(Target, P - HalfDim, P + HalfDim, Entry->Color);

                Address += sizeof(*Entry);
            } break;
            
            case(RenderGroupEntryType_render_entry_rect_slow):
            {
                render_entry_rect_slow *Entry = (render_entry_rect_slow *) Header;
                v2 P = Entry->Origin;
                
                DrawRectangleSlowly(Target, P, Entry->XAxis, Entry->YAxis, Entry->Color);

                Address += sizeof(*Entry);
            } break;
            
            case(RenderGroupEntryType_render_entry_tex_slow):
            {
                render_entry_tex_slow *Entry = (render_entry_tex_slow *) Header;
                v2 P = Entry->Origin;
                
                DrawTextureSlowly(Target, P, Entry->XAxis, Entry->YAxis,
                                  Entry->Texture, Entry->NormalMap,
                                  Entry->Color, Entry->Sky, Entry->Ground,
                                  Entry->ScreenPosition);

                Address += sizeof(*Entry);
            } break;

            case(RenderGroupEntryType_render_entry_clear):
            {
                render_entry_clear *Entry = (render_entry_clear *) Header;
                DrawRectangle(Target, 0, 0, (r32)Target->Width,
                              (r32)Target->Height, Entry->Color);
                Address += sizeof(*Entry);
            } break;
            
            InvalidDefaultCase;
        }
    }
}
 

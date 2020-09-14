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
internal void DrawRectangleHopefullyQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, 
                                            loaded_bitmap *Texture, v4 Color)
{
    

    //Note: Premultiply  color up front
    Color.rgb *= Color.a;

    real32 InvXAxisLengthSq = 1.0f / LengthSquared(XAxis);
    real32 InvYAxisLengthSq = 1.0f / LengthSquared(YAxis);

    real32 XAxisLength = Length(XAxis);
    real32 YAxisLength = Length(YAxis);

    // NOTE: NzScale could be a parameter if we want people to
    // have control over the amount of scaling in the Z direction
    // that the normals appear to have.
    real32 NzScale = 0.5f * (XAxisLength + YAxisLength);

    v2 NxAxis = (YAxisLength / XAxisLength) * XAxis;
    v2 NyAxis = (XAxisLength / YAxisLength) * YAxis;

    //TODO: IMPORTANT: STOP DOIND THIS
    int WidthMax = (Buffer->Width - 1) - 3;
    int HeightMax = (Buffer->Height - 1) - 3;

    real32 InvWidthMax = 1.0f / (real32)WidthMax;
    real32 InvHeightMax = 1.0f / (real32)HeightMax;

    real32 OriginZ = 0.0f;
    real32 OriginY = (Origin + 0.5f * XAxis + 0.5f * YAxis).y;
    real32 FixedCastY = InvHeightMax * OriginY;

    int XMin = WidthMax;
    int XMax = 0;
    int YMin = HeightMax;
    int YMax = 0;

    v2 P[4] = {Origin, Origin + XAxis, Origin + XAxis + YAxis, Origin + YAxis};
    for (int PIndex = 0;
         PIndex < ArrayCount(P);
         ++PIndex)
    {
        v2 TestP = P[PIndex];
        int FloorX = Floor(TestP.x);
        int CeilX = Ceill(TestP.x);
        int FloorY = Floor(TestP.y);
        int CeilY = Ceill(TestP.y);

        if (XMin > FloorX)
        {
            XMin = FloorX;
        }
        if (YMin > FloorY)
        {
            YMin = FloorY;
        }
        if (XMax < CeilX)
        {
            XMax = CeilX;
        }
        if (YMax < CeilY)
        {
            YMax = CeilY;
        }
    }

    if (XMin < 0)
    {
        XMin = 0;
    }
    if (YMin < 0)
    {
        YMin = 0;
    }
    if (XMax > WidthMax)
    {
        XMax = WidthMax;
    }
    if (YMax > HeightMax)
    {
        YMax = HeightMax;
    }

    v2 nXAxis = InvXAxisLengthSq * XAxis;
    v2 nYAxis = InvYAxisLengthSq * YAxis;
    real32 Inv255 = 1.0f / 255.0f;
    __m128 Inv255_4x = _mm_set1_ps(Inv255);
    real32 One255 = 255.0f;
    __m128 One255_4x = _mm_set1_ps(One255);

    __m128 One = _mm_set1_ps(1.0f);
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 Colorr_4x = _mm_set1_ps(Color.r);
    __m128 Colorg_4x = _mm_set1_ps(Color.g);
    __m128 Colorb_4x = _mm_set1_ps(Color.b);
    __m128 Colora_4x = _mm_set1_ps(Color.a);

    __m128 Originx_4x = _mm_set1_ps(Origin.x);
    __m128 Originy_4x = _mm_set1_ps(Origin.y);

    __m128 nXAxisx_4x = _mm_set1_ps(nXAxis.x);
    __m128 nXAxisy_4x = _mm_set1_ps(nXAxis.y);
    __m128 nYAxisx_4x = _mm_set1_ps(nYAxis.x);
    __m128 nYAxisy_4x = _mm_set1_ps(nYAxis.y);

    __m128 WidthM2 = _mm_set1_ps((real32)Texture->Width - 2);
    __m128 HeightM2 = _mm_set1_ps((real32)Texture->Height - 2);

    __m128i MaskFF = _mm_set1_epi32(0xff);

    uint8 *Row = (((uint8 *)Buffer->Bytes) +
                  XMin * BITMAP_BYTES_PER_PIXEL +
                  YMin * Buffer->Pitch);
    BEGIN_TIMED_BLOCK(PixelHit);
    for (int Y = YMin;
         Y <= YMax;
         ++Y)
    {
#define mmSquare(a) _mm_mul_ps(a, a)
#define M(a, i) ((float *)&(a))[i]
#define Mi(a, i) ((int32 *)&(a))[i]

        uint32 *Pixel = (uint32 *)Row;
        for (int XI = XMin;
             XI <= XMax;
             XI += 4)
        {
            __m128 PixelPx = _mm_set_ps((real32)(XI + 3), (real32)(XI + 2), (real32)(XI + 1), (real32)(XI + 0));
            __m128 PixelPy = _mm_set1_ps((real32)Y);

            __m128 dx = _mm_sub_ps(PixelPx, Originx_4x);
            __m128 dy = _mm_sub_ps(PixelPy, Originy_4x);
            __m128 U = _mm_add_ps(_mm_mul_ps(dx, nXAxisx_4x), _mm_mul_ps(dy, nXAxisy_4x));
            __m128 V = _mm_add_ps(_mm_mul_ps(dx, nYAxisx_4x), _mm_mul_ps(dy, nYAxisy_4x));

            __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero),
                                                                       _mm_cmple_ps(U, One)),
                                                            _mm_and_ps(_mm_cmpge_ps(V, Zero),
                                                                       _mm_cmple_ps(V, One))));

            __m128i OriginalDest = _mm_loadu_si128((__m128i *)Pixel);

            //TODO: Later, recheck if this helps
            if (_mm_movemask_epi8(WriteMask))
            {
                U = _mm_min_ps(_mm_max_ps(U, Zero), One);
                V = _mm_min_ps(_mm_max_ps(V, Zero), One);

                __m128 tX = _mm_mul_ps(U, WidthM2);
                __m128 tY = _mm_mul_ps(V, HeightM2);

                __m128i FetchX_4x = _mm_cvttps_epi32(tX);
                __m128i FetchY_4x = _mm_cvttps_epi32(tY);

                __m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(FetchX_4x));
                __m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(FetchY_4x));

                __m128i SampleA;
                __m128i SampleB;
                __m128i SampleC;
                __m128i SampleD;

                for (int I = 0;
                     I < 4;
                     ++I)
                {
                    int32 FetchX = Mi(FetchX_4x, I);
                    int32 FetchY = Mi(FetchY_4x, I);

                    //Assert((FetchX >= 0.0f) && (FetchX < Texture->Width));
                    //Assert((FetchY >= 0.0f) && (FetchY < Texture->Height));

                    //bilinear sample
                    uint8 *TexelPtr = (((uint8 *)Texture->Bytes) + FetchX * BITMAP_BYTES_PER_PIXEL + FetchY * Texture->Pitch);
                    Mi(SampleA, I) = *(uint32 *)(TexelPtr);
                    Mi(SampleB, I) = *(uint32 *)(TexelPtr + BITMAP_BYTES_PER_PIXEL);
                    Mi(SampleC, I) = *(uint32 *)(TexelPtr + Texture->Pitch);
                    Mi(SampleD, I) = *(uint32 *)(TexelPtr + Texture->Pitch + BITMAP_BYTES_PER_PIXEL);
                }

                __m128 TexelAa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 24), MaskFF));
                __m128 TexelAr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 16), MaskFF));
                __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF));
                __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(SampleA, MaskFF));

                __m128 TexelBa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 24), MaskFF));
                __m128 TexelBr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 16), MaskFF));
                __m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF));
                __m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(SampleB, MaskFF));

                __m128 TexelCa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 24), MaskFF));
                __m128 TexelCr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 16), MaskFF));
                __m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF));
                __m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(SampleC, MaskFF));

                __m128 TexelDa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 24), MaskFF));
                __m128 TexelDr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 16), MaskFF));
                __m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF));
                __m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(SampleD, MaskFF));

                //NOTE: Convert texture from sRGB to "linear" brightness space
                TexelAr = mmSquare(_mm_mul_ps(Inv255_4x, TexelAr));
                TexelAg = mmSquare(_mm_mul_ps(Inv255_4x, TexelAg));
                TexelAb = mmSquare(_mm_mul_ps(Inv255_4x, TexelAb));
                TexelAa = _mm_mul_ps(Inv255_4x, TexelAa);

                TexelBr = mmSquare(_mm_mul_ps(Inv255_4x, TexelBr));
                TexelBg = mmSquare(_mm_mul_ps(Inv255_4x, TexelBg));
                TexelBb = mmSquare(_mm_mul_ps(Inv255_4x, TexelBb));
                TexelBa = _mm_mul_ps(Inv255_4x, TexelBa);

                TexelCr = mmSquare(_mm_mul_ps(Inv255_4x, TexelCr));
                TexelCg = mmSquare(_mm_mul_ps(Inv255_4x, TexelCg));
                TexelCb = mmSquare(_mm_mul_ps(Inv255_4x, TexelCb));
                TexelCa = _mm_mul_ps(Inv255_4x, TexelCa);

                TexelDr = mmSquare(_mm_mul_ps(Inv255_4x, TexelDr));
                TexelDg = mmSquare(_mm_mul_ps(Inv255_4x, TexelDg));
                TexelDb = mmSquare(_mm_mul_ps(Inv255_4x, TexelDb));
                TexelDa = _mm_mul_ps(Inv255_4x, TexelDa);

                //NOTE: Bilinear texture Blend
                __m128 ifX = _mm_sub_ps(One, fX);
                __m128 ifY = _mm_sub_ps(One, fY);

                __m128 l0 = _mm_mul_ps(ifY, ifX);
                __m128 l1 = _mm_mul_ps(ifY, fX);
                __m128 l2 = _mm_mul_ps(fY, ifX);
                __m128 l3 = _mm_mul_ps(fY, fX);

                __m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAr), _mm_mul_ps(l1, TexelBr)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCr), _mm_mul_ps(l3, TexelDr)));
                __m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAg), _mm_mul_ps(l1, TexelBg)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCg), _mm_mul_ps(l3, TexelDg)));
                __m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAb), _mm_mul_ps(l1, TexelBb)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCb), _mm_mul_ps(l3, TexelDb)));
                __m128 Texela = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAa), _mm_mul_ps(l1, TexelBa)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCa), _mm_mul_ps(l3, TexelDa)));

                Texelr = _mm_mul_ps(Texelr, Colorr_4x);
                Texelg = _mm_mul_ps(Texelg, Colorg_4x);
                Texelb = _mm_mul_ps(Texelb, Colorb_4x);
                Texela = _mm_mul_ps(Texela, Colora_4x);

                Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), One);
                Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), One);
                Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), One);

                //NOTE: Load destination
                __m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));
                __m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
                __m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF));
                __m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));

                //NOTE: Go from sRGB to "linear" brightness space
                Destr = mmSquare(_mm_mul_ps(Inv255_4x, Destr));
                Destg = mmSquare(_mm_mul_ps(Inv255_4x, Destg));
                Destb = mmSquare(_mm_mul_ps(Inv255_4x, Destb));
                Desta = _mm_mul_ps(Inv255_4x, Desta);

                //NOTE: Destination blend
                __m128 InvTexelA = _mm_sub_ps(One, Texela);
                __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
                __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
                __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
                __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);

                //NOTE: Go from "linear" brightness space to sRGB
                Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
                Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
                Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
                Blendeda = _mm_mul_ps(One255_4x, Blendeda);

                __m128i Intr = _mm_cvtps_epi32(Blendedr);
                __m128i Intg = _mm_cvtps_epi32(Blendedg);
                __m128i Intb = _mm_cvtps_epi32(Blendedb);
                __m128i Inta = _mm_cvtps_epi32(Blendeda);

                Inta = _mm_slli_epi32(Inta, 24);
                Intr = _mm_slli_epi32(Intr, 16);
                Intg = _mm_slli_epi32(Intg, 8);
                //Intb = _mm_slli_epi32(Inta, 0);

                __m128i Out = _mm_or_si128(_mm_or_si128(Intr, Intg), _mm_or_si128(Intb, Inta));

                __m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                                                 _mm_andnot_si128(WriteMask, OriginalDest));

                _mm_storeu_si128((__m128i *)Pixel, MaskedOut);
            }
            Pixel += 4;
        }
        Row += Buffer->Pitch;
    }
    END_TIMED_BLOCK_COUNTED(PixelHit, (XMax - XMin + 1) * (YMax - YMin + 1));
    
}

internal void
DrawTextureQuick(loaded_bitmap *Destination, v2 Origin,
                 v2 XAxis, v2 YAxis, loaded_bitmap *Texture,
                 v4 Color)
{

    v2 P[4];
    P[0] = Origin;
    P[1] = Origin + XAxis;
    P[2] = Origin + XAxis + YAxis;
    P[3] = Origin + YAxis;

    s32 MinX = Destination->Width;
    s32 MaxX = -1;
    s32 MinY = Destination->Height;
    s32 MaxY = -1;
    
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
   
    v2 nXAxis = InvXAxisLenSq * XAxis;
    v2 nYAxis = InvYAxisLenSq * YAxis;
   
    
    u32 TexWidth = Texture->Width;
    u32 TexHeight = Texture->Height;

    __m128 nXAxisX = _mm_set1_ps(nXAxis.x);
    __m128 nXAxisY = _mm_set1_ps(nXAxis.y);
    __m128 nYAxisX = _mm_set1_ps(nYAxis.x);
    __m128 nYAxisY = _mm_set1_ps(nYAxis.y);
    
    __m128 OriginX = _mm_set1_ps(Origin.x);
    __m128 OriginY = _mm_set1_ps(Origin.y);

    __m128 TexWidth_4x = _mm_set1_ps((f32)(TexWidth - 2));
    __m128 TexHeight_4x = _mm_set1_ps((f32)(TexHeight - 2));
    
    __m128 TexelAr = _mm_setzero_ps();
    __m128 TexelAg = _mm_setzero_ps();
    __m128 TexelAb = _mm_setzero_ps();
    __m128 TexelAa = _mm_setzero_ps();

    __m128 TexelBr = _mm_setzero_ps();
    __m128 TexelBg = _mm_setzero_ps();
    __m128 TexelBb = _mm_setzero_ps();
    __m128 TexelBa = _mm_setzero_ps();

    __m128 TexelCr = _mm_setzero_ps();
    __m128 TexelCg = _mm_setzero_ps();
    __m128 TexelCb = _mm_setzero_ps();
    __m128 TexelCa = _mm_setzero_ps();

    __m128 TexelDr = _mm_setzero_ps();
    __m128 TexelDg = _mm_setzero_ps();
    __m128 TexelDb = _mm_setzero_ps();
    __m128 TexelDa = _mm_setzero_ps();

    __m128 Pixelr = _mm_setzero_ps();
    __m128 Pixelg = _mm_setzero_ps();
    __m128 Pixelb = _mm_setzero_ps();
    __m128 Pixela = _mm_setzero_ps();

    __m128i TexelAi = _mm_setzero_si128();
    __m128i TexelBi = _mm_setzero_si128();
    __m128i TexelCi = _mm_setzero_si128();
    __m128i TexelDi = _mm_setzero_si128();

    __m128i Mask_0xFF = _mm_set1_epi32(0xFF);
                    
    __m128i RedShift = _mm_set_epi32(0,0,0,16);
    __m128i GreenShift = _mm_set_epi32(0,0,0,8);
    __m128i BlueShift = _mm_set_epi32(0,0,0,0);
    __m128i AlphaShift = _mm_set_epi32(0,0,0,24);
                        
    __m128 Inv255 = _mm_set1_ps(1.0f/255.0f);
    __m128 One255 = _mm_set1_ps(255.0f);
    __m128 One = _mm_set1_ps(1.0f);
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 Four = _mm_set1_ps(4.0f);
    

    u32 *PixelPtr = 0;
    
    BEGIN_TIMED_BLOCK(PixelHit);
    for (s32 y = MinY; y <= MaxY; ++y)
    {
        
        __m128 y_4x = _mm_set1_ps((f32) y);
        __m128 dy = _mm_sub_ps(y_4x, OriginY);
        __m128 dx = _mm_set_ps((f32)(MinX + 3 - Origin.x),
                                 (f32)(MinX + 2 - Origin.x),
                                 (f32)(MinX + 1 - Origin.x),
                                 (f32)(MinX + 0 - Origin.x));
        
        for (s32 x = MinX; x <= MaxX; x+=4)
        {
            
            //__m128 dx = _mm_sub_ps(x_4x, OriginX);
            
            __m128 U = _mm_add_ps(_mm_mul_ps(dx, nXAxisX),
                                  _mm_mul_ps(dy, nXAxisY));
            __m128 V = _mm_add_ps(_mm_mul_ps(dx, nYAxisX),
                                  _mm_mul_ps(dy, nYAxisY));

#define MI(A, I) ((s32 *)&(A))[I]
//#define M(A, I) ((f32 *)&(A))[I]

            __m128i ShouldFill;
            
            __m128 TexX = _mm_mul_ps(U, TexWidth_4x);
            __m128 TexY = _mm_mul_ps(V, TexHeight_4x);
            
            __m128i Xi = _mm_cvttps_epi32(TexX);
            __m128i Yi = _mm_cvttps_epi32(TexY);
                
            
            __m128 fX = _mm_sub_ps(TexX, _mm_cvtepi32_ps(Xi));
            __m128 fY = _mm_sub_ps(TexY, _mm_cvtepi32_ps(Yi));

            ShouldFill = _mm_castps_si128(_mm_and_ps(
                (_mm_and_ps(_mm_cmpge_ps(U, Zero), _mm_cmple_ps(U, One))),
                (_mm_and_ps(_mm_cmpge_ps(V, Zero), _mm_cmple_ps(V, One)))));

            //Assert((U >= 0.0f) && (U <= 1.0f));
            //Assert((V >= 0.0f) && (V <= 1.0f));
            
            // Unpack A B C D Texel

            // Clamp Xi, Yi 0..1
            Yi = _mm_and_si128(Yi, ShouldFill);
            Xi = _mm_and_si128(Xi, ShouldFill);
            for (u32 I = 0; I < 4; ++I)
            {
                u32 *TexelPtr = (u32 *)Texture->Bytes + MI(Yi,I) * TexWidth + MI(Xi,I);
                
                u32 *TexelAPtr = TexelPtr;
                u32 *TexelBPtr = TexelPtr + 1;
                u32 *TexelCPtr = TexelPtr + TexWidth;
                u32 *TexelDPtr = TexelPtr + TexWidth + 1;

                MI(TexelAi,I) = *TexelAPtr;
                MI(TexelBi,I) = *TexelBPtr;
                MI(TexelCi,I) = *TexelCPtr;
                MI(TexelDi,I) = *TexelDPtr;
                
            }

            
            // Unapck TexelA

            TexelAr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelAi, RedShift), Mask_0xFF));
            TexelAg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelAi, GreenShift), Mask_0xFF));
            TexelAb = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelAi, BlueShift), Mask_0xFF));
            TexelAa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelAi, AlphaShift), Mask_0xFF));

            // Unapck TexelB
            
            TexelBr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelBi, RedShift), Mask_0xFF));
            TexelBg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelBi, GreenShift), Mask_0xFF));
            TexelBb = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelBi, BlueShift), Mask_0xFF));
            TexelBa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelBi, AlphaShift), Mask_0xFF));

            // Unapck TexelC

            TexelCr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelCi, RedShift), Mask_0xFF));
            TexelCg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelCi, GreenShift), Mask_0xFF));
            TexelCb = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelCi, BlueShift), Mask_0xFF));
            TexelCa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelCi, AlphaShift), Mask_0xFF));

            // Unapck TexelD

            TexelDr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelDi, RedShift), Mask_0xFF));
            TexelDg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelDi, GreenShift), Mask_0xFF));
            TexelDb = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelDi, BlueShift), Mask_0xFF));
            TexelDa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(TexelDi, AlphaShift), Mask_0xFF));

            // Unpack Destination Pixel
            PixelPtr = (u32 *)Destination->Bytes + y * Destination->Width + x;
            __m128i Pixeli = _mm_loadu_si128((__m128i *)PixelPtr);

            Pixelr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(Pixeli, RedShift), Mask_0xFF));
            Pixelg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(Pixeli, GreenShift), Mask_0xFF));
            Pixelb = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(Pixeli, BlueShift), Mask_0xFF));
            Pixela = _mm_cvtepi32_ps(_mm_and_si128(_mm_srl_epi32(Pixeli, AlphaShift), Mask_0xFF));
                
            // SRGBA255 To Linear space
            TexelAr = _mm_mul_ps(TexelAr, Inv255);
            TexelAg = _mm_mul_ps(TexelAg, Inv255);
            TexelAb = _mm_mul_ps(TexelAb, Inv255);
            TexelAa = _mm_mul_ps(TexelAa, Inv255);

            TexelBr = _mm_mul_ps(TexelBr, Inv255);
            TexelBg = _mm_mul_ps(TexelBg, Inv255);
            TexelBb = _mm_mul_ps(TexelBb, Inv255);
            TexelBa = _mm_mul_ps(TexelBa, Inv255);

            TexelCr = _mm_mul_ps(TexelCr, Inv255);
            TexelCg = _mm_mul_ps(TexelCg, Inv255);
            TexelCb = _mm_mul_ps(TexelCb, Inv255);
            TexelCa = _mm_mul_ps(TexelCa, Inv255);

            TexelDr = _mm_mul_ps(TexelDr, Inv255);
            TexelDg = _mm_mul_ps(TexelDg, Inv255);
            TexelDb = _mm_mul_ps(TexelDb, Inv255);
            TexelDa = _mm_mul_ps(TexelDa, Inv255);

            TexelAr = _mm_mul_ps(TexelAr, TexelAr);
            TexelAg = _mm_mul_ps(TexelAg, TexelAg);
            TexelAb = _mm_mul_ps(TexelAb, TexelAb);

            TexelBr = _mm_mul_ps(TexelBr, TexelBr);
            TexelBg = _mm_mul_ps(TexelBg, TexelBg);
            TexelBb = _mm_mul_ps(TexelBb, TexelBb);

            TexelCr = _mm_mul_ps(TexelCr, TexelCr);
            TexelCg = _mm_mul_ps(TexelCg, TexelCg);
            TexelCb = _mm_mul_ps(TexelCb, TexelCb);

            TexelDr = _mm_mul_ps(TexelDr, TexelDr);
            TexelDg = _mm_mul_ps(TexelDg, TexelDg);
            TexelDb = _mm_mul_ps(TexelDb, TexelDb);
                
            // Lerp A B C D
            __m128 ifX = _mm_sub_ps(One, fX);
            __m128 ifY = _mm_sub_ps(One, fY);
                
            __m128 LerpABr = _mm_add_ps(_mm_mul_ps(ifX, TexelAr), _mm_mul_ps(fX, TexelBr));
            __m128 LerpABg = _mm_add_ps(_mm_mul_ps(ifX, TexelAg), _mm_mul_ps(fX, TexelBg));
            __m128 LerpABb = _mm_add_ps(_mm_mul_ps(ifX, TexelAb), _mm_mul_ps(fX, TexelBb));
            __m128 LerpABa = _mm_add_ps(_mm_mul_ps(ifX, TexelAa), _mm_mul_ps(fX, TexelBa));
                
            __m128 LerpCDr = _mm_add_ps(_mm_mul_ps(ifX, TexelCr), _mm_mul_ps(fX, TexelDr));
            __m128 LerpCDg = _mm_add_ps(_mm_mul_ps(ifX, TexelCg), _mm_mul_ps(fX, TexelDg));
            __m128 LerpCDb = _mm_add_ps(_mm_mul_ps(ifX, TexelCb), _mm_mul_ps(fX, TexelDb));
            __m128 LerpCDa = _mm_add_ps(_mm_mul_ps(ifX, TexelCa), _mm_mul_ps(fX, TexelDa));

            __m128 Texelr = _mm_add_ps(_mm_mul_ps(ifY, LerpABr), _mm_mul_ps(fY, LerpCDr));
            __m128 Texelg = _mm_add_ps(_mm_mul_ps(ifY, LerpABg), _mm_mul_ps(fY, LerpCDg));
            __m128 Texelb = _mm_add_ps(_mm_mul_ps(ifY, LerpABb), _mm_mul_ps(fY, LerpCDb));
            __m128 Texela = _mm_add_ps(_mm_mul_ps(ifY, LerpABa), _mm_mul_ps(fY, LerpCDa));
                                
            //Texel = Hadamard(Texel, Color);
            
            Pixelr = _mm_mul_ps(Pixelr, Inv255);
            Pixelg = _mm_mul_ps(Pixelg, Inv255);
            Pixelb = _mm_mul_ps(Pixelb, Inv255);
            Pixela = _mm_mul_ps(Pixela, Inv255);

            Pixelr =  _mm_mul_ps(Pixelr, Pixelr);
            Pixelg =  _mm_mul_ps(Pixelg, Pixelg);
            Pixelb =  _mm_mul_ps(Pixelb, Pixelb);

            //v4 Blended = InvA * Pixel + Texel;
            __m128 InvA = _mm_sub_ps(One, Texela);
            __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvA, Pixelr), Texelr);
            __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvA, Pixelg), Texelg);
            __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvA, Pixelb), Texelb);
            __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvA, Pixela), Texela);
                
            //Blended = Linear1ToSRGBA255(Blended);
                
            Blendedr = _mm_mul_ps(One255, _mm_sqrt_ps(Blendedr));
            Blendedg = _mm_mul_ps(One255, _mm_sqrt_ps(Blendedg));
            Blendedb = _mm_mul_ps(One255, _mm_sqrt_ps(Blendedb));
            Blendeda = _mm_mul_ps(One255, Blendeda);

            //*PixelPtr = PackBGRA(Blended);

            __m128i Blendedri = _mm_cvttps_epi32(Blendedr);
            __m128i Blendedgi = _mm_cvttps_epi32(Blendedg);
            __m128i Blendedbi = _mm_cvttps_epi32(Blendedb);
            __m128i Blendedai = _mm_cvttps_epi32(Blendeda);

            Blendedri = _mm_sll_epi32(Blendedri, RedShift);
            Blendedgi = _mm_sll_epi32(Blendedgi, GreenShift);
            Blendedbi = _mm_sll_epi32(Blendedbi, BlueShift);
            Blendedai = _mm_sll_epi32(Blendedai, AlphaShift);

            __m128i Blended = _mm_or_si128(
                _mm_or_si128(Blendedri, Blendedgi),
                _mm_or_si128(Blendedbi, Blendedai));
            __m128i Mask1 = _mm_and_si128(ShouldFill, Blended);
            __m128i Mask2 = _mm_andnot_si128(ShouldFill, Pixeli);

            _mm_storeu_si128((__m128i *)PixelPtr, _mm_or_si128(Mask1, Mask2));
            
                
            
            dx = _mm_add_ps(dx, Four);
        }
    }
    END_TIMED_BLOCK_COUNTED(PixelHit, ((MaxX-MinX+1)*(MaxY-MinY+1)));
        
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
                //DrawBitmap(Target, Entry->Bitmap, P.x, P.y, Entry->Alpha);
                v2 XAxis = (r32)Entry->Bitmap->Width * V2(1.f, 0.f);
                v2 YAxis = (r32)Entry->Bitmap->Height * V2(0.f, 1.f);//Perp(XAxis);
                DrawRectangleHopefullyQuickly(Target, P, XAxis, YAxis,
                                 Entry->Bitmap, V4(1,1,1,1));                
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
 

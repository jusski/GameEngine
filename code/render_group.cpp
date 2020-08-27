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

//    Normal.xyz = Normalize(Normal.xyz);
    // Reflection vector = -e - 2 * (e,N) * N  
    v3 Reflection = V3(0,0,-1) + 2.0f * Normal.z * Normal.xyz;
    

    
    r32 Distance = 0.f + 1.0f - Normal.w;
    r32 Ratio = SafeRatio0(Distance, Reflection.z);
    if (Reflection.z < 0.0f)
    {
        return DefaultColor;
    }

    r32 s = Ratio * Reflection.x;
    r32 t = Ratio * Reflection.y;
    
    r32 X = (ScreenPosition.x + s) * (Texture->Width);
    r32 Y = (ScreenPosition.y + t) * (Texture->Height);
    if ((X > (r32)Texture->Width - 2.0f) ||
        (Y > (r32)Texture->Height - 2.0f) ||
        (X < 0.0f) || (Y < 0.0f))
    {
        return DefaultColor;
    }
        //X = Clamp(X, 0.0f, 98.0f);
        //Y = Clamp(Y, 0.0f, 98.0f);
    u32 x = (u32)X;
    u32 y = (u32)Y;
    
    r32 fX = X - (r32)x;
    r32 fY = Y - (r32)y;

    Assert(X <= (Texture->Width - 2));
    Assert(Y <= (Texture->Height - 2));
    u32 *TexelPtr = (u32 *)Texture->Bytes + y * Texture->Width + x;
#if 1
    bilinear_sample Sample = BilinearSample(TexelPtr, Texture->Width);
    v4 BlendedLight = BilinearLerp(Sample, fX, fY);
    //BlendedLight = Normalize(BlendedLight);
#else
    v4 BlendedLight = UnpackBGRA(TexelPtr);
    BlendedLight = Normalize(BlendedLight);
#endif
    return(BlendedLight);
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
    s32 ClipY = (TopLeftY < 0) ? (s32)TopLeftY : 0;

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

            u32 R = DR + SR-(DR*SA)/255;
            u32 G = DG + SG-(DG*SA)/255;
            u32 B = DB + SB-(DB*SA)/255;

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
    Offset.y = -Offset.y;
    Entry->Offset = Offset * PixelsPerMeter - Bitmap->Align; 
}

internal void
PushCompositeBitmap(render_group *Group, loaded_bitmap *Destination,
                    loaded_bitmap *Source, v2 P, r32 Alpha = 1.0f)
{
    Assert(Source);
    Assert(Destination);
    render_entry_composite *Entry = PushRenderEntry(Group, render_entry_composite);

    Entry->Source = Source;
    Entry->Destination = Destination;
    Entry->Alpha = Alpha;
    Entry->P = P;
}

internal void
PushRect(render_group *Group, v2 Offset, v2 Dim, v4 Color)
{
    render_entry_rect *Entry = PushRenderEntry(Group, render_entry_rect);

    Entry->Color = Color;
    
    r32 PixelsPerMeter = Group->GameState->PixelsPerMeter;
    Offset.y = -Offset.y;
    Entry->Offset = Offset * PixelsPerMeter;

    Entry->Dim = PixelsPerMeter * Dim;
}

internal void
PushRectSlow(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color)
{
    render_entry_rect_slow *Entry = PushRenderEntry(Group, render_entry_rect_slow);

    Entry->Color = Color;
    Entry->Origin = Origin;
    Entry->XAxis = XAxis;
    Entry->YAxis = YAxis;
    
}

internal void
PushTextureSlow(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis,
                loaded_bitmap *Texture, normal_map *NormalMap,
                v4 Color, enviroment_map *Sky, enviroment_map *Ground,
                v2 ScreenPosition)
{
    render_entry_tex_slow *Entry = PushRenderEntry(Group, render_entry_tex_slow);

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

internal void
PushClear(render_group *Group, v4 Color)
{
    render_entry_clear *Entry = PushRenderEntry(Group, render_entry_clear);

    Entry->Color = Color;
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
                    

                v4 Light = SampleEnviromentMap(Sky, Normal,
                                               ScreenPosition + 1.f*V2(U,V),
                                                   V4(1,1,1,1));
#if 1                
                    Texel.rgb = Texel.rgb +
                      Texel.a* Lerp(Texel.rgb, Normal.z , Light.rgb);
#else
                    Texel.rgb = Light.rgb;
#endif
                Texel = Hadamard(Texel, Color);

                v4 Pixel = UnpackBGRA(PixelPtr);
                Pixel = SRGBA255ToLinear1(Pixel);
                
                r32 InvA = 1.0f - Texel.a;
                v4 Blended = InvA * Pixel + Texel;
                Blended = Linear1ToSRGBA255(Blended);

                *PixelPtr = PackBGRA(Blended);
            }
        } 
    }
        
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
MakeSphereNormalMap(loaded_bitmap *Bitmap)
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
            
            //Sphere equation is x^2 + y^2 + z^2 = 1
            r32 z = 1 - u*u - v*v;
            if (z >= 0.0f)
            {
                r32 w = SquareRoot(z);

                //Scale to 0..1
                u = (u + 1.0f) / 2.0f;
                v = (v + 1.0f) / 2.0f;
                w = (w + 1.0f) / 2.0f;

                v4 Normal = {u, v, w, w};

                //Convert to RGB space
                Normal *= 255.0f;
                
                *Pixel = PackBGRA(Normal);
            }
            else
            {
                v4 DefaultNormal = 255.0f *V4(0.0f, 0.0f, 1.0f, 0.0f);
                *Pixel = PackBGRA(DefaultNormal);
            }
            //v4 V = 255.0f *V4(0.0f, 0.0f, 1.0f, 0.0f);
            //*Pixel = PackBGRA(V);
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

            r32 x = Cos(AngleRadiance);
            r32 y = 0.f;
            r32 z = Sin(AngleRadiance);
            r32 w = -(r32)X / (Width - 1);
            
            
            *Normal = V4(x, y, z, w);            
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
#endif
                Address += sizeof(*Entry);
            } break;

            case(RenderGroupEntryType_render_entry_composite):
            {
                render_entry_composite *Entry = (render_entry_composite *) Header;
                Assert(Entry->Source);
                Assert(Entry->Destination);
                
#if 0 
                CompositeBitmap(Entry->Destination, Entry->Source, Entry->P);
#endif
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
 

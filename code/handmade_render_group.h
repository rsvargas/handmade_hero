#pragma once

/*
    1) Everywhere outside the renderer, Y _always_ goes upward, X to the right.

    2) All bitmaps including the render targer, are assumed to be bottom-up
       (meaning that the first row pointer points to the bottom-most row
        when viewer on screen).

    3) It is mandatory that all inputs to the renderer are in world 
       coordinate ("meters"), NOT pixels. It for some reason something 
       absolutelu has to be specified in pixels, that will be explicitly
       marked in the API, but this sould occour exceedengly sparingly.

    4) Z is a special coordinate because it is broken up into discrete slices,
       and the renderer actually understands these slices. Z slices are what 
       control the _scaling_ of thinfs, whereas Z offsets inside a slice are
       what controle Y offsetting.

       //TODO: ZHANDLING

    5) All color values secified to the rendere as V4's are in 
       NON-premiltiplied alpha.

*/

struct loaded_bitmap
{
    v2 AlignPercentage;
    real32 WidthOverHeight;

    int32 Width;
    int32 Height;
    int32 Pitch;
    uint32 *Memory;
};

struct environment_map
{
    loaded_bitmap LOD[4];
    real32 Pz;
};

enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_rectangle,
    RenderGroupEntryType_render_entry_coordinate_system,
};

struct render_group_entry_header
{
    render_group_entry_type Type;
};

struct render_entry_clear
{
    v4 Color;
};

struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;

    v4 Color;
    v2 P;
    v2 Size;
};

struct render_entry_rectangle
{
    v4 Color;
    v2 P;
    v2 Dim;
};

// NOTE: This is only for test
struct render_entry_coordinate_system
{
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
    loaded_bitmap *Texture;
    loaded_bitmap *NormalMap;

    //real32 PixelsToMeters; //Need to store this for lighting

    environment_map *Top;
    environment_map *Middle;
    environment_map *Bottom;
};

struct render_transform
{
    bool32 Orthographic;
    //NOTE: Camera parameters
    real32 MetersToPixels; // NOTE: This translates meters _on the monitor_  into pixels _on the monitor_
    v2 ScreenCenter;

    real32 FocalLength;
    real32 DistanceAboveTarget;

    v3 OffsetP;
    real32 Scale;
};

struct render_group
{
    struct game_assets *Assets;
    real32 GlobalAlpha;

    v2 MonitorHalfDimInMeters;
    render_transform Transform;

    uint32 MaxPushBufferSize;
    uint32 PushBufferSize;
    uint8 *PushBufferBase;
};

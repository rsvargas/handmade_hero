#include "handmade.h"


internal void GameOutputSound(game_state& GameState, game_sound_output_buffer& SoundBuffer, real32 ToneHz)
{
    
    int16 ToneVolume = 3000;
    real32 WavePeriod = (real32)SoundBuffer.SamplesPerSecond / ToneHz;

    int16* SampleOut = SoundBuffer.Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer.SampleCount; ++SampleIndex)
    {
#if 0
        real32 SineValue = sinf(GameState.tSine);
        int16 SampleValue = (int16)(SineValue*ToneVolume);
#else
        int16 SampleValue = 0;
#endif

        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

#if 0
        GameState.tSine += 2.0f * Pi32 * (1.0f / (real32)WavePeriod);
        if (GameState.tSine >(2.0f * Pi32))
        {
            GameState.tSine -= (2.0f * Pi32);
        }
#endif
    }
}

internal int32 RoundReal32ToInt32(real32 val)
{
    return (val > 0.0f) ? (int)(val + 0.5f) : (int)(val - 0.5f);
}


internal void DrawRectangle(game_offscreen_buffer &Buffer, 
    real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
    uint32 Color
    )
{
    int MinX = RoundReal32ToInt32(RealMinX);
    int MinY = RoundReal32ToInt32(RealMinY);
    int MaxX = RoundReal32ToInt32(RealMaxX);
    int MaxY = RoundReal32ToInt32(RealMaxY);

    if (MinX < 0)
    {
        MinX = 0;
    }

    if (MinY < 0)
    {
        MinY = 0;
    }

    if (MaxX > Buffer.Width)
    {
        MaxX = Buffer.Width;
    }

    if (MaxY > Buffer.Height)
    {
        MaxY = Buffer.Height;
    }

    uint8 *EndOfBuffer = ((uint8*)Buffer.Memory) + (Buffer.Pitch * Buffer.Height);

    uint8* Row= (((uint8*)Buffer.Memory) +
        MinX * Buffer.BytesPerPixel +
        MinY * Buffer.Pitch);

    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Pixel = (uint32*)Row;
        for (int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }
        Row += Buffer.Pitch;
    }

}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    ASSERT((&Input.Controllers[0].Terminator - &Input.Controllers[0].Buttons[0]) == ARRAY_COUNT(Input.Controllers->Buttons) )
    ASSERT(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        Memory->IsInitialized = true;
    }

    for (int ControllerIndex = 0;
        ControllerIndex < ARRAY_COUNT(Input.Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(&Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            //NOTE: Use analog movement tuning
        }
        else
        {
            //NOTE: Use digital movement tuning
        }

    }

    DrawRectangle(Buffer, 0, 0, (real32)Buffer.Width, (real32)Buffer.Height, 0x00FF00FF);
    DrawRectangle(Buffer, 100, 100, 110, 110, 0x0000FFFF);
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state*)Memory->PermanentStorage;
    GameOutputSound(*GameState, SoundOutput, 400);
}





/*
internal void RenderWeirdGradiend(const game_offscreen_buffer& Buffer, int XOffset, int YOffset)
{
    //TODO: Lets see what the optimizer does

    uint8* Row = (uint8*)Buffer.Memory;
    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0; X < Buffer.Width; ++X)
        {
            uint8 Green = (uint8)(Y + YOffset);
            uint8 Blue = (uint8)(X + XOffset);
            //Memory:   BB GG RR xx
            //Register: xx RR GG BB
            //Pixel (32-bits)
            *Pixel++ = (Green << 8) | Blue;
        }
        Row += Buffer.Pitch;
    }
}
*/

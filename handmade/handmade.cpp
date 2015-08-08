#include "handmade.h"


internal void GameOutputSound(const game_sound_output_buffer& SoundBuffer, int ToneHz)
{
    local_persist real32 tSine = 0.0f;
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer.SamplesPerSecond / ToneHz;

    int16* SampleOut = SoundBuffer.Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer.SampleCount; ++SampleIndex)
    {
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue*ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * (1.0f / (real32)WavePeriod);
    }
}

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
            /*
            Memory:   BB GG RR xx
            Register: xx RR GG BB
            Pixel (32-bits)
            */
            *Pixel++ = (Green << 8) | Blue;
        }
        Row += Buffer.Pitch;
    }
}


internal void GameUpdateAndRender(game_memory* Memory,
    const game_offscreen_buffer& Buffer,
    const game_input& Input,
    const game_sound_output_buffer& SoundOutput)
{
    ASSERT(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        char* Filename = __FILE__;
        debug_read_file_result Bitmap = DEBUGPlatformReadEntireFile(Filename);
        if (Bitmap.Contents)
        {
            DEBUGPlatformWriteEntireFile("test.txt", Bitmap.ContentsSize, Bitmap.Contents);
            DEBUGPlatformFreeFileMemory(Bitmap.Contents);
        }

        GameState->ToneHz = 256;
        Memory->IsInitialized = true;
    }

    game_controller_input Input0 = Input.Controllers[0];

    if (Input0.IsAnalog)
    {
        //NOTE: Use analog movement tuning
        GameState->ToneHz = 256 + (int)(128.0f*Input0.EndY);
        GameState->BlueOffset += (int)(4.0f*Input0.EndX);
    }
    else
    {
        //NOTe: Use digital movement tuning
    }

    if (Input0.Down.EndedDown)
    {
        GameState->GreenOffset += 1;
    }




    //TODO: Allow sample offsets here for more robust platform options
    GameOutputSound(SoundOutput, GameState->ToneHz);
    RenderWeirdGradiend(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}
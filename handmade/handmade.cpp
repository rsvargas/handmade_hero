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

        GameState.tSine += 2.0f * Pi32 * (1.0f / (real32)WavePeriod);
        if (GameState.tSine >(2.0f * Pi32))
        {
            GameState.tSine -= (2.0f * Pi32);
        }
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

internal void RenderPlayer(game_offscreen_buffer &Buffer, int PlayerX, int PlayerY)
{
    uint8 *EndOfbuffer = (uint8*)Buffer.Memory + Buffer.BytesPerPixel* Buffer.Pitch * Buffer.Height;
    uint32 Color = 0xFFFFFFFF;
    int Top = PlayerY;
    int Bottom = PlayerY + 10;
    for (int X = PlayerX; X < PlayerX + 10; ++X)
    {
        uint8* Pixel = ((uint8*)Buffer.Memory +
            X* Buffer.BytesPerPixel +
            Top * Buffer.Pitch);

        for (int Y = Top; Y < Bottom; ++Y)
        {
            if (Pixel >= Buffer.Memory && ((Pixel + Buffer.BytesPerPixel) < EndOfbuffer))
            {
                *(uint32*)Pixel = Color;
                Pixel += Buffer.Pitch;
            }
        }
    }

}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    ASSERT((&Input.Controllers[0].Terminator - &Input.Controllers[0].Buttons[0]) == ARRAY_COUNT(Input.Controllers->Buttons) )
    ASSERT(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        char* Filename = __FILE__;
        debug_read_file_result Bitmap = Memory->DEBUGPlatformReadEntireFile(Filename);
        if (Bitmap.Contents)
        {
            Memory->DEBUGPlatformWriteEntireFile("test.txt", Bitmap.ContentsSize, Bitmap.Contents);
            Memory->DEBUGPlatformFreeFileMemory(Bitmap.Contents);
        }

        GameState->ToneHz = 512.0f;
        GameState->tSine = 0.0f;

        GameState->PlayerX = 100;
        GameState->PlayerY = 100;

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
            GameState->ToneHz = 512.0f + (128.0f*Controller->StickAverageY);
            GameState->BlueOffset += (int)(4.0f*Controller->StickAverageX);
        }
        else
        {
            if (Controller->MoveLeft.EndedDown)
            {
                GameState->BlueOffset -= 1;
            }
            if (Controller->MoveRight.EndedDown)
            {
                GameState->BlueOffset += 1;
            }

            //NOTE: Use digital movement tuning
        }

        GameState->PlayerX += (int)(7.0f*Controller->StickAverageX);
        GameState->PlayerY -= (int)(7.0f*Controller->StickAverageY);

        if (GameState->tJump > 0)
        {
            GameState->PlayerY += (int)(10.0f*sinf(0.5f*Pi32*GameState->tJump));
        }

        if (Controller->ActionDown.EndedDown)
        {
            GameState->tJump = 4.0f;
        }
        if (GameState->tJump> 0.0f)
        {
            GameState->tJump -= 0.033f;
        }
    }

    RenderWeirdGradiend(Buffer, GameState->BlueOffset, GameState->GreenOffset);
    RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY); 
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state*)Memory->PermanentStorage;
    GameOutputSound(*GameState, SoundOutput, GameState->ToneHz);
}

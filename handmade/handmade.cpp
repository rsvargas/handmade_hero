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


internal void GameUpdateAndRender(const game_offscreen_buffer& Buffer, int BlueOffset, int GreenOffset,
								  const game_sound_output_buffer& SoundOutput, int ToneHz)
{
	//TODO: Allow sample offsets here for more robust platform options
	GameOutputSound(SoundOutput, ToneHz);
	RenderWeirdGradiend(Buffer, BlueOffset, GreenOffset);
}
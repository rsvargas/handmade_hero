#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

#define ARRAY_COUNT(A) (sizeof(A)/sizeof((A)[0]))



//TODO: Services that the platform layer provides to the game

/*
//NOTE: Services that the game provides to the platform layer
	(this may extando in the future - sound on separare thread, etc. )
*/
// Four Things - timing, controller/keyboar input, bitmap buffer to use, sound buffer to use

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

// TODO: I the future, rendering _specifically_ will become a three-tiered abstaction!!
struct game_offscreen_buffer
{
	//NOTE: Pixels are always 32bit wide
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsAnalog;

	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;
	union
	{
		game_button_state buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};


struct game_input
{
	game_controller_input Controllers[4];
};

void GameUpdateAndRender(const game_offscreen_buffer& Buffer,
						 const game_input& Input,
						 const game_sound_output_buffer& SoundOutput);


#endif// HANDMADE_H
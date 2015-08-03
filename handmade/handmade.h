#pragma once

//TODO: Services that the platform layer provides to the game

/*
//NOTE: Services that the game provides to the platform layer
	(this may extando in the future - sound on separare thread, etc. )
*/
// Four Things - timing, controller/keyboar input, bitmap buffer to use, sound buffer to use

// TODO: I the future, rendering _specifically_ will become a three-tiered abstaction!!
struct game_offscreen_buffer
{
	//NOTE: Pixels are always 32bit wide
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

void GameUpdateAndRender(const game_offscreen_buffer& Buffer, int BlueOffset, int GreenOffset);




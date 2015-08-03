#include "handmade.h"


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


internal void GameUpdateAndRender(const game_offscreen_buffer& Buffer, int BlueOffset, int GreenOffset)
{
	RenderWeirdGradiend(Buffer, BlueOffset, GreenOffset);
}
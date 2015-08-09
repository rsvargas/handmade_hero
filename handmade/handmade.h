#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

/*
    NOTE:

    HANDMADE_INTERNAL
        0 = Build for public release
        1 = Build for developer only

    HANDMADE_SLOW
        0 = No slow code allowed
        1 = Slow code allowed!
*/


#if HANDMADE_SLOW == 1
#define ASSERT(X) if(!(X)) { *(int*)0 = 0;}
#else
#define ASSERT(X)
#endif

#define KILOBYTES(V) ((V)*1024LL)
#define MEGABYTES(V) (KILOBYTES(V)*1024LL)
#define GIGABYTES(V) (MEGABYTES(V)*1024LL)
#define TERABYTES(V) (GIGABYTES(V)*1024LL)

#define ARRAY_COUNT(A) (sizeof(A)/sizeof((A)[0]))

inline uint32 SafeTruncateUInt64(uint64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    uint32 Result = (uint32)value;
    return Result;
}



//TODO: Services that the platform layer provides to the game

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    uint32 ContentsSize;
    void* Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char * Filename);
internal void DEBUGPlatformFreeFileMemory(void* Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void* Memory);   
#endif

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
    bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

            game_button_state Start;
            game_button_state Back;

            //NOTE: All buttons shuold be added bellow this line
            game_button_state Terminator;
        };
	};
};


struct game_input
{
    //keyboard is zero
	game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
    ASSERT(ControllerIndex < ARRAY_COUNT(Input->Controllers));
    return &Input->Controllers[ControllerIndex];
}

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void* PermanentStorage; //NOTE: REQUIRED to be cleared to zero at startup

    uint64 TransientStorageSize;
    void* TransientStorage; //NOTE: REQUIRED to be cleared to zero at startup

};

internal void GameUpdateAndRender(game_memory* Memory,
    const game_offscreen_buffer& Buffer,
    const game_input& Input,
    const game_sound_output_buffer& SoundOutput);


struct game_state
{
    float ToneHz;
    int GreenOffset;
    int BlueOffset;
};





#endif// HANDMADE_H
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
#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;


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

struct thread_context
{
    int Placeholder;
};



#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    uint32 ContentsSize;
    void* Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* Thread,char * Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* Thread, char *Filename, uint32 MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

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
    int BytesPerPixel;
    int MemorySize;
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
    game_button_state MouseButtons[5];
    int32 MouseX;
    int32 MouseY;
    int32 MouseZ;

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

#if HANDMADE_INTERNAL
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
#endif

};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory* Memory, game_offscreen_buffer& Buffer, game_input& Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

//NOTE: At the moment, this has to be a very fast function it cannot be more than a millisecond or so.
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory* Memory, game_sound_output_buffer& SoundOutput)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

struct game_state
{
    float ToneHz;
    int GreenOffset;
    int BlueOffset;

    real32 tSine;

    int PlayerX;
    int PlayerY;
    real32 tJump;
};





#endif// HANDMADE_H
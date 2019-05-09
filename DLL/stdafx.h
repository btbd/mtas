#define _CRT_SECURE_NO_WARNINGS
#define DIRECTINPUT_VERSION 0x0800

#include <stdio.h>
#include <vector>
#include <map>
#include <stdlib.h>
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <winternl.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "synchronization.lib")

#include "memory.h"
#define EXPORT __declspec(dllexport)

extern "C" {
	EXPORT void AddControl(DWORD);
	EXPORT void RemoveControl(DWORD);
	EXPORT void GetControl(DWORD *);
	EXPORT void Wait();
	EXPORT void NewDemo();
	EXPORT void LoadDemo(char *path);
	EXPORT void SaveDemo(char *path);
	EXPORT void StartDemo();
	EXPORT void GotoFrame(DWORD);
	EXPORT void GetDemoCommand(DWORD *);
	EXPORT void GetDemoFrame(DWORD *);
	EXPORT void GetDemoFrameCount(DWORD *);
	EXPORT void GetDemoFrames(DWORD *);
	EXPORT void SetTimescale(float);
	EXPORT void GetTimescale(float *);
	EXPORT void AddGotoFlag(DWORD);
	EXPORT void RemoveGotoFlag(DWORD);
	EXPORT void GetGotoFlags(DWORD *);
}

#define CONTROL_PLAY (0)
#define CONTROL_PAUSE (1 << 0)
#define CONTROL_ADVANCE (1 << 1)
#define CONTROL_RECORD (1 << 2)
#define CONTROL_PAUSE_CHANGE (1 << 3)
#define CONTROL_PAUSE_GROUND (1 << 4)
#define CONTROL_PAUSE_AIR (1 << 5)
#define CONTROL_PAUSE_WALLRUN (1 << 6)
#define CONTROL_PAUSE_WALLCLIMB (1 << 7)
#define GOTO_FULL (0)
#define GOTO_FAST (1 << 0)
#define GOTO_NO_STREAM (1 << 1)
#define UpdateRenderFunc() rendering.Disable = (goto_flags & GOTO_FAST) ? FastDisableRendering : FullDisableRendering;
#define Lock() { EnterCriticalSection(&mutex); }
#define Unlock() { LeaveCriticalSection(&mutex); }
#define TryLock(body) { bool __unlock = TryEnterCriticalSection(&mutex); { body }; if (__unlock) { Unlock(); }}

static wchar_t *KEYS[] = {
	L"", // 0
	L"LeftMouseButton", // 1
	L"RightMouseButton", // 2
	L"Cancel", // 3
	L"MiddleMouseButton", // 4
	L"ThumbMouseButton", // 5
	L"ThumbMouseButton2", // 6
	L"", // 7
	L"BackSpace", // 8
	L"TAB", // 9
	L"", // a
	L"", // b
	L"Clear", // c
	L"Enter", // d
	L"", // e
	L"", // f
	L"", // 10
	L"", // 11
	L"", // 12
	L"Pause", // 13
	L"CAPSLOCK", // 14
	L"", // 15
	L"", // 16
	L"", // 17
	L"", // 18
	L"", // 19
	L"", // 1a
	L"Escape", // 1b
	L"", // 1c
	L"", // 1d
	L"", // 1e
	L"", // 1f
	L"SpaceBar", // 20
	L"PageUp", // 21
	L"PageDown", // 22
	L"End", // 23
	L"Home", // 24
	L"Left", // 25
	L"Up", // 26
	L"Right", // 27
	L"Down", // 28
	L"Select", // 29
	L"Print", // 2a
	L"Execute", // 2b
	L"", // 2c
	L"Insert", // 2d
	L"Delete", // 2e
	L"Help", // 2f
	L"ZERO", // 30
	L"ONE", // 31
	L"TWO", // 32
	L"THREE", // 33
	L"FOUR", // 34
	L"FIVE", // 35
	L"SIX", // 36
	L"SEVEN", // 37
	L"EIGHT", // 38
	L"NINE", // 39
	L"", // 3a
	L"", // 3b
	L"", // 3c
	L"", // 3d
	L"", // 3e
	L"", // 3f
	L"", // 40
	L"A", // 41
	L"B", // 42
	L"C", // 43
	L"D", // 44
	L"E", // 45
	L"F", // 46
	L"G", // 47
	L"H", // 48
	L"I", // 49
	L"J", // 4a
	L"K", // 4b
	L"L", // 4c
	L"M", // 4d
	L"N", // 4e
	L"O", // 4f
	L"P", // 50
	L"Q", // 51
	L"R", // 52
	L"S", // 53
	L"T", // 54
	L"U", // 55
	L"V", // 56
	L"W", // 57
	L"X", // 58
	L"Y", // 59
	L"Z", // 5a
	L"", // 5b
	L"", // 5c
	L"", // 5d
	L"", // 5e
	L"", // 5f
	L"NumPadZero", // 60
	L"NumPadOne", // 61
	L"NumPadTwo", // 62
	L"NumPadThree", // 63
	L"NumPadFour", // 64
	L"NumPadFive", // 65
	L"NumPadSix", // 66
	L"NumPadSeven", // 67
	L"NumPadEight", // 68
	L"NumPadNine", // 69
	L"Multiply", // 6a
	L"Add", // 6b
	L"", // 6c
	L"Subtract", // 6d
	L"Decimal", // 6e
	L"Divide", // 6f
	L"F1", // 70
	L"F2", // 71
	L"F3", // 72
	L"F4", // 73
	L"F5", // 74
	L"F6", // 75
	L"F7", // 76
	L"F8", // 77
	L"F9", // 78
	L"F10", // 79
	L"F11", // 7a
	L"F12", // 7b
	L"F13", // 7c
	L"F14", // 7d
	L"F15", // 7e
	L"F16", // 7f
	L"F17", // 80
	L"F18", // 81
	L"F19", // 82
	L"F20", // 83
	L"F21", // 84
	L"F22", // 85
	L"F23", // 86
	L"F24", // 87
	L"", // 88
	L"", // 89
	L"", // 8a
	L"", // 8b
	L"", // 8c
	L"", // 8d
	L"", // 8e
	L"", // 8f
	L"NumLock", // 90
	L"ScrollLock", // 91
	L"", // 92
	L"", // 93
	L"", // 94
	L"", // 95
	L"", // 96
	L"", // 97
	L"", // 98
	L"", // 99
	L"", // 9a
	L"", // 9b
	L"", // 9c
	L"", // 9d
	L"", // 9e
	L"", // 9f
	L"LeftShift", // a0
	L"RightShift", // a1
	L"LeftControl", // a2
	L"RightControl", // a3
	L"LeftAlt", // a4
	L"RightAlt", // a5
	L"", // a6
	L"", // a7
	L"", // a8
	L"", // a9
	L"", // aa
	L"", // ab
	L"", // ac
	L"", // ad
	L"", // ae
	L"", // af
	L"", // b0
	L"", // b1
	L"", // b2
	L"", // b3
	L"", // b4
	L"", // b5
	L"", // b6
	L"", // b7
	L"", // b8
	L"", // b9
	L"", // ba
	L"", // bb
	L"", // bc
	L"", // bd
	L"", // be
	L"", // bf
	L"", // c0
	L"", // c1
	L"", // c2
	L"", // c3
	L"", // c4
	L"", // c5
	L"", // c6
	L"", // c7
	L"", // c8
	L"", // c9
	L"", // ca
	L"", // cb
	L"", // cc
	L"", // cd
	L"", // ce
	L"", // cf
	L"", // d0
	L"", // d1
	L"", // d2
	L"", // d3
	L"", // d4
	L"", // d5
	L"", // d6
	L"", // d7
	L"", // d8
	L"", // d9
	L"", // da
	L"LeftBracket", // db
	L"Backslash", // dc
	L"RightBracket", // dd
	L"Quote", // de
	L"", // df
	L"", // e0
	L"", // e1
	L"", // e2
	L"", // e3
	L"", // e4
	L"", // e5
	L"", // e6
	L"", // e7
	L"", // e8
	L"", // e9
	L"", // ea
	L"", // eb
	L"", // ec
	L"", // ed
	L"", // ee
	L"", // ef
	L"", // f0
	L"", // f1
	L"", // f2
	L"", // f3
	L"", // f4
	L"", // f5
	L"", // f6
	L"", // f7
	L"", // f8
	L"", // f9
	L"", // fa
	L"", // fb
	L"", // fc
	L"", // fd
	L"", // fe
};

#define DELAY (0.02f)

struct FRAME {
	struct {
		float change, delta; // negative delta if MouseY
	} mouse_moves[0x10];
	struct {
		short keycode; // negative if keyup
		wchar_t alias[0x20];
	} keys[100];
};

typedef struct {
	wchar_t command[0xFF];
	DWORD frame_count;
	FRAME frames[1];
} DEMO;

static struct {
	double *delta, *last, *current, *frequency;
} time = { 0 };

static struct {
	DWORD ticks;
	LARGE_INTEGER frequency;
} qpc = { 0 };

static struct {
	DWORD faith, engine, input, strings, check, uworld, r0, r1, r2;
} base = { 0 };

DWORD GetStringId(wchar_t *str);
wchar_t *GetStringById(DWORD id);
void ExecuteCommand(wchar_t *);
void FullDisableRendering(bool);
void FastDisableRendering(bool);
void MainHooks();

static NTSTATUS(WINAPI *QueryInformationThread)(HANDLE, DWORD, PVOID, ULONG, PULONG);
static void(__thiscall *PlayerHandlerOriginal)(void *, float, int);
static int(__thiscall *InputHandlerOriginal)(void *, int, int, int, int, int, float, int);
static int(__thiscall *MouseHandlerOriginal)(int, int, int, int, int, float, float, int);
static void **(__thiscall *ExecuteCommandOriginal)(int, void **, int, int);
static void(*GetEngineBaseOriginal)();
static short GetKeyStateKeyboard[0xFF] = { 0 };
static short(WINAPI *GetKeyStateOriginal)(int);
static void(*CreateDeviceOriginal)();
static int(__thiscall *LevelLoadOriginal)(void *, int, __int64);
static DWORD(WINAPI *WaitForSingleObjectOriginal)(HANDLE a, DWORD b);
static BOOL(WINAPI *QueryPerformanceFrequencyOriginal)(LARGE_INTEGER *);
static BOOL(WINAPI *QueryPerformanceCounterOriginal)(LARGE_INTEGER *);
static BOOL(WINAPI *PeekMessageWOriginal)(LPMSG, HWND, UINT, UINT, UINT);
static void(WINAPI *SleepOriginal)(DWORD);
static HRESULT(WINAPI *PresentOriginal)(IDirect3DDevice9 *, RECT *, RECT *, HWND, RGNDATA *);
static int *(__thiscall *LevelStreamOriginal)(DWORD);
static void *(__thiscall *CopyActorInfoOriginal)(DWORD *, float *, void *, int, int, int);
static int(__thiscall *UpdateObjectOriginal)(void *, int);
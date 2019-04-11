#include "stdafx.h"

bool disable_rendering = false;
DWORD control = 0, processed_frames = 0;
double delay = DELAY;

struct {
	wchar_t command[0xFF] = { 0 };
	DWORD frame = 0, jump = 0;
	std::vector<FRAME> *frames = new std::vector<FRAME>;
} demo;

void DoFrameInput(DWORD index) {
	DWORD b = (DWORD)GetPointer(GetCurrentProcess(), 3, base.input, 0, 0);
	if (b && index < demo.frames->size()) {
		DWORD a0 = *(DWORD *)(b + 0x20);
		DWORD a1 = b + 4;

		FRAME *frame = (FRAME *)&(*demo.frames)[index];
		for (DWORD i = 0; i < sizeof(frame->keys) / sizeof(frame->keys[0]); ++i) {
			auto k = frame->keys[i];
			if (k.keycode == 0) break;

			if (k.keycode > 0) {
				if (k.keycode < 0xFF) GetKeyStateKeyboard[k.keycode] = 1;
				InputHandlerOriginal((void *)a0, a1, 0, GetStringId(k.alias), 0, 0, 1, 0);
			} else {
				byte keycode = -k.keycode;
				if (keycode < 0xFF) GetKeyStateKeyboard[keycode] = 2;
				InputHandlerOriginal((void *)a0, a1, 0, GetStringId(k.alias), 0, 1, 1, 0);
			}
		}

		for (DWORD i = 0; i < sizeof(frame->mouse_moves) / sizeof(frame->mouse_moves[0]); ++i) {
			auto mm = frame->mouse_moves[i];
			if (mm.delta == 0) break;

			MouseHandlerOriginal(a0, a1, 0, GetStringId(mm.delta > 0 ? L"MouseX" : L"MouseY"), 0, mm.change, mm.delta, 0);
		}
	}
}

void UpdateEngineHook() {
	static struct {
		byte state;
		LARGE_INTEGER time;
	} last = { 0 };

	++processed_frames;

	*time.delta = DELAY;
	*time.last = *time.current = 0;

	if (*demo.command && demo.jump) {
		SetThreadPriority(GetCurrentThread(), 2);

		DoFrameInput(demo.frame);
		if (demo.frame++ >= demo.jump - 1) {
			control |= CONTROL_PAUSE;
			demo.jump = 0;
		}

		if (demo.frame >= demo.jump - 1) {
			DisableRendering(false);
		}
	} else {
		SetThreadPriority(GetCurrentThread(), 0);

		if (*demo.command) {
			if (demo.frame == demo.frames->size()) {
				demo.frames->push_back({ 0 });
			}

			byte state = ReadChar(GetCurrentProcess(), (void *)(base.faith + 0x68));
			if ((last.state != state && (control & CONTROL_PAUSE_CHANGE)) ||
				(state == 1 && (control & CONTROL_PAUSE_GROUND)) ||
				(state == 2 && (control & CONTROL_PAUSE_AIR)) ||
				(state == 12 && (control & CONTROL_PAUSE_WALLRUN)) ||
				(state == 13 && (control & CONTROL_PAUSE_WALLCLIMB)))
			{
				control |= CONTROL_PAUSE;
			}

			last.state = state;

			while ((control & CONTROL_PAUSE) && !(control & CONTROL_ADVANCE)) {
				Sleep(1);
			}

			if (control & CONTROL_ADVANCE) {
				control = (control & ~CONTROL_ADVANCE) | CONTROL_PAUSE;
			}

			DoFrameInput(demo.frame++);
		}

		for (;;) {
			LARGE_INTEGER t = { 0 };
			QueryPerformanceCounterOriginal(&t);
			if ((double)(t.QuadPart - last.time.QuadPart) / (double)qpc.frequency.QuadPart >= delay) {
				break;
			}

			Sleep(0);
		}

		QueryPerformanceCounterOriginal(&last.time);
	}
}

int __fastcall InputHandlerHook(void *this_, void *idle_, int a2, int a3, int id, int a5, int event, float a7, int a8) {
	if (event == 2) return 0;

	if (*demo.command && (control & CONTROL_RECORD) && demo.frame) {
		DWORD index = demo.frame - 1;
		if (index < demo.frames->size()) {
			FRAME *f = &(*demo.frames)[index];
			DWORD i = 0;
			for (; i < sizeof(f->keys) / sizeof(f->keys[0]); ++i) {
				if (f->keys[i].keycode == 0) break;
			}

			auto k = &f->keys[i];
			wcscpy(k->alias, GetStringById(id));

			short keycode = 0xFF;
			for (short i = 0; i < sizeof(KEYS) / sizeof(KEYS[0]); ++i) {
				if (wcscmp(k->alias, KEYS[i]) == 0) {
					keycode = i;
					break;
				}
			}

			k->keycode = (event == 0 ? keycode : -keycode);
		}
	}

	return InputHandlerOriginal(this_, a2, a3, id, a5, event, a7, a8);
}

int __fastcall MouseHandlerHook(int this_, void *idle_, int a2, int a3, int id, int a5, float change, float delta, int a8) {
	if (*demo.command && (control & CONTROL_RECORD) && demo.frame) {
		DWORD index = demo.frame - 1;
		if (index < demo.frames->size()) {
			FRAME *f = &(*demo.frames)[index];
			DWORD i = 0;
			for (; i < sizeof(f->mouse_moves) / sizeof(f->mouse_moves[0]); ++i) {
				if (f->mouse_moves[i].delta == 0) break;
			}

			auto mm = &f->mouse_moves[i];
			mm->change = change;
			mm->delta = (wcscmp(GetStringById(id), L"MouseX") == 0 ? delta : -delta);
		}
	}

	return MouseHandlerOriginal(this_, a2, a3, id, a5, change, delta, a8);
}

void __fastcall PlayerHandlerHook(void *this_, void *idle_, float a2, int a3) {
	base.faith = (DWORD)this_;
	PlayerHandlerOriginal(this_, DELAY, a3);
}

__declspec(naked) void CreateDeviceHook() {
	static D3DPRESENT_PARAMETERS *params = 0;
	__asm {
		mov params, ecx
		push eax
		push ebx
		push edx
		push esi
		push edi
		push ebp
		push esp
	}

	params->Windowed = TRUE;

	__asm {
		pop esp
		pop ebp
		pop edi
		pop esi
		pop edx
		pop ebx
		pop eax
		jmp CreateDeviceOriginal
	}
}

void ExecuteCommand(wchar_t *command) {
	if (base.engine) {
		wchar_t c[0xFF] = { 0 };
		wsprintf(c, L"%s\r\n", command);

		char *this_ = (char *)calloc(0xFFF, 1);
		*(DWORD *)(this_ + 716) = base.engine;

		char *a2 = (char *)calloc(0xFFF, 1);
		*(DWORD *)a2 = (DWORD)a2;

		char *a3 = (char *)calloc(0xFFF, 1);
		*(DWORD *)(a3 + 4) = wcslen(c);
		*(DWORD *)a3 = (DWORD)c;

		ExecuteCommandOriginal((int)this_, (void **)a2, (int)a3, 1);

		free(this_);
		free(a2);
		free(a3);
	}
}

DWORD GetStringId(wchar_t *str) {
	struct wstr_cmp {
		bool operator()(wchar_t *a, wchar_t *b) const {
			return wcscmp(a, b) < 0;
		}
	};
	static std::map<wchar_t *, DWORD, wstr_cmp> *cache = new std::map<wchar_t *, DWORD, wstr_cmp>;

	auto c = cache->find(str);
	if (c != cache->end()) {
		return c->second;
	}

	DWORD s = ReadInt(GetCurrentProcess(), (void *)base.strings);
	if (s) {
		for (DWORD i = 0; i < 0xFFFF; ++i) {
			DWORD b = *(DWORD *)(s + (4 * i));
			if (b) {
				wchar_t *s = (wchar_t *)(b + 0x10);
				if (s && wcscmp(s, str) == 0) {
					cache->insert(std::pair<wchar_t *, DWORD>(_wcsdup(str), i));
					return i;
				}
			}
		}
	}

	return 0;
}

wchar_t *GetStringById(DWORD id) {
	return (wchar_t *)GetPointer(GetCurrentProcess(), 3, base.strings, 4 * id, 0x10);
}

__declspec(naked) void GetEngineBaseHook() {
	__asm {
		mov eax, [ecx]
		mov base.engine, eax
		jmp GetEngineBaseOriginal
	}
}

DWORD WINAPI GetTickCountHook() {
	return (DWORD)(qpc.ticks * 0.01);
}

DWORD WINAPI GetTickCount64Hook() {
	return (DWORD)(qpc.ticks * 0.01);
}

BOOL WINAPI QueryPerformanceFrequencyHook(LARGE_INTEGER *f) {
	static bool hooked = false;
	if (!hooked) {
		InterlockedExchange8((char *)&hooked, 1);
		hooked = MainHooks();
	}
	f->QuadPart = (ULONG64)1e7;
	return TRUE;
}

BOOL WINAPI QueryPerformanceCounterHook(LARGE_INTEGER *f) {
	f->QuadPart = ++qpc.ticks;
	return TRUE;
}

int randHook() {
	return 16383;
}

SHORT WINAPI GetKeyStateHook(int key) {
	if (key < 0xFF && GetKeyStateKeyboard[key]) {
		switch (GetKeyStateKeyboard[key]) {
			case 1:
				GetKeyStateKeyboard[key] = 0;
				return (short)0x8000;
			case 2:
				GetKeyStateKeyboard[key] = 0;
				return 0;
		}
	}

	return GetKeyStateOriginal(key);
}

BOOL WINAPI PeekMessageWHook(LPMSG msg, HWND window, UINT min, UINT max, UINT remove) {
	BOOL ret = PeekMessageWOriginal(msg, window, min, max, remove);
	if (!ret) return ret;

	if (msg && msg->hwnd && (remove & PM_REMOVE) != 0 && msg->message != WM_PAINT && *demo.command && !(control & CONTROL_RECORD)) {
		msg->message = WM_NULL;
	}

	return TRUE;
}

void WINAPI SleepHook(DWORD t) {
	if (disable_rendering) return;
	SleepOriginal(t);
}

HRESULT WINAPI PresentHook(IDirect3DDevice9 *device, RECT *src, RECT *dest, HWND window, RGNDATA *dirty) {
	if (disable_rendering) {
		device->Clear(1, (D3DRECT *)dest, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 0, 0);

		LPD3DXFONT font;
		D3DXCreateFontA(device, 20, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &font);
		char text[0xFF] = { 0 };
		sprintf(text, "Reverting to frame %d", demo.jump);
		RECT r = { 10, 10 };
		font->DrawTextA(0, text, strlen(text), &r, DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));
		font->Release();
	}

	return PresentOriginal(device, src, dest, window, dirty);
}

void DisableRendering(bool disable) {
	static byte bytes[22] = { 0 };
	static byte nops[22] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	disable_rendering = disable;
	if (disable) {
		if (!*bytes) {
			VirtualProtect((void *)base.rendering, sizeof(bytes), PAGE_EXECUTE_READWRITE, (DWORD *)bytes);
			memcpy(bytes, (void *)base.rendering, sizeof(bytes));
		}

		memcpy((void *)base.rendering, nops, sizeof(bytes));
		ExecuteCommand(L"set GameViewportClient bDisableWorldRendering 1");
		ExecuteCommand(L"set UIScene bDisableWorldRendering 1");
	} else {
		memcpy((void *)base.rendering, bytes, sizeof(bytes));
		ExecuteCommand(L"set GameViewportClient bDisableWorldRendering 0");
		ExecuteCommand(L"set UIScene bDisableWorldRendering 0");
	}
}

void MainThread() {
	if (!GetModuleHandleA("MirrorsEdge.exe")) {
		return;
	}

	while (!GetModuleHandleA("d3d9.dll")) {
		Sleep(1);
	}

	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	TrampolineHook(QueryPerformanceFrequencyHook, QueryPerformanceFrequency, (void **)&QueryPerformanceFrequencyOriginal);
}

bool MainHooks() {
	MODULEENTRY32 mod = GetModuleInfoByName(GetCurrentProcessId(), L"mirrorsedge.exe");
	DWORD addr = 0;

	// 0x40418E
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\xDD\x05\x00\x00\x00\x00\x5E\xDD\x15", "xx????xxx");
	if (!addr) return false;

	addr += 7;
	printf("Times: %x\n", addr);
	// 0x1F723E0
	time.delta = (double *)(*(DWORD *)(addr + 2));
	printf("- Delta: %x\n", (DWORD)time.delta);
	// 0x2027FA8
	time.current = (double *)(*(DWORD *)(addr + 8));
	printf("- Current: %x\n", (DWORD)time.current);
	// 0x1F98618
	time.last = (double *)(*(DWORD *)(addr + 14));
	printf("- Last %x\n", (DWORD)time.last);
	// 0x2020738
	time.frequency = (double *)(*(DWORD *)(addr + 49));
	printf("- Frequency: %x\n", (DWORD)time.frequency);

	// 0x404140
	addr -= 78;
	printf("UpdateEngine: %x\n", addr);
	SetJMP(UpdateEngineHook, (void *)addr, 0);

	// 0x417A4F
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\xA3\x00\x00\x00\x00\x57\x53\xFF", "x????xxx");
	printf("Input: %x\n", addr);
	// 0x1F993BC
	base.input = *(DWORD *)(addr + 1);
	printf("- Base: %x\n", base.input);

	// 0xB725FF
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\x6A\x01\xC1\xE8\x02", "xxxxx");
	printf("Rendering: %x\n", addr);
	base.rendering = addr;

	// 0x11132C9
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\xA3\x00\x00\x00\x00\x8D\x14\x98", "x????xxx");
	printf("Strings: %x\n", addr);
	// 0x204E7D8
	base.strings = *(DWORD *)(addr + 1);
	printf("- Table: %x\n", base.strings);

	SetJMP(GetTickCountHook, GetTickCount, 0);
	SetJMP(GetTickCount64Hook, GetTickCount64, 0);
	TrampolineHook(QueryPerformanceCounterHook, QueryPerformanceCounter, (void **)&QueryPerformanceCounterOriginal);
	SetJMP(randHook, GetProcAddress(GetModuleHandleA("msvcr80.dll"), "rand"), 0);

	// 0x18AF75F
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\x51\x8B\x4C\x24\x5C\x52\x8B\x54\x24\x4C\x51", "xxxxxxxxxxx");
	printf("CreateDevice: %x\n", addr);
	TrampolineHook(CreateDeviceHook, (void *)addr, (void **)&CreateDeviceOriginal);

	// 0x12B0960
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\x0F\xB6\x51\x68\x81", "xxxxx") - 36;
	printf("PlayerHandler: %x\n", addr);
	TrampolineHook(PlayerHandlerHook, (void *)addr, (void **)&PlayerHandlerOriginal);

	// 0xB38A30
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\x51\x53\x8B\x5C\x24\x24\x55\x8B\x6C\x24\x20", "xxxxxxxxxxx");
	printf("InputHandler: %x\n", addr);
	TrampolineHook(InputHandlerHook, (void *)addr, (void **)&InputHandlerOriginal);

	// 0xB38BF0
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\x53\x55\x56\x57\x8B\xD9\x33\xFF\x39\xBB\xB0", "xxxxxxxxxxx");
	printf("MouseHandler: %x\n", addr);
	TrampolineHook(MouseHandlerHook, (void *)addr, (void **)&MouseHandlerOriginal);

	TrampolineHook(GetKeyStateHook, (void *)GetKeyState, (void **)&GetKeyStateOriginal);
	TrampolineHook(PeekMessageWHook, (void *)PeekMessageW, (void **)&PeekMessageWOriginal);
	TrampolineHook(SleepHook, (void *)Sleep, (void **)&SleepOriginal);
	TrampolineHook(PresentHook, (void *)GetD3D9Exports()[D3D9_EXPORT_PRESENT], (void **)&PresentOriginal);

	// 0xFF5D7D
	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\x8B\x01\xF3\x0F\x10\x98\x98\x00\x00\x00", "xxxxxxxxxx");
	printf("GetEngineBase: %x\n", addr);
	TrampolineHook(GetEngineBaseHook, (void *)addr, (void **)&GetEngineBaseOriginal);

	addr = (DWORD)FindPattern(mod.modBaseAddr, mod.modBaseSize, "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x83\xEC\x28\x53\x55\x56\x57\xA1\x00\x00\x00\x00\x33\xC4\x50\x8D\x44\x24\x3C\x64\xA3\x00\x00\x00\x00\x89\x4C\x24\x18\x33\xDB\x89\x5C\x24\x14\x39\x99\xCC\x02\x00\x00", "xxx????xxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	printf("ExecuteCommand: %x\n", addr);
	*(DWORD *)&ExecuteCommandOriginal = addr;

	QueryPerformanceFrequencyHook(&qpc.frequency);
	*time.frequency = 1.0 / (double)qpc.frequency.QuadPart;
	QueryPerformanceFrequencyOriginal(&qpc.frequency);

	return true;
}

EXPORT void AddControl(DWORD c) {
	control |= c;
}

EXPORT void RemoveControl(DWORD c) {
	control &= ~c;
}

EXPORT void GetControl(DWORD *c) {
	if (c) *c = control;
}

EXPORT void Wait() {
	if (!(control & CONTROL_PAUSE)) {
		DWORD f = processed_frames;
		while (processed_frames == f) Sleep(1);
	}
}

EXPORT void NewDemo() {
	RemoveControl(CONTROL_PAUSE);
	Wait();
	*demo.command = 0;
	demo.frame = demo.jump = 0;
	demo.frames->clear();
	demo.frames->shrink_to_fit();
}

EXPORT void LoadDemo(char *path) {
	NewDemo();

	FILE *file = fopen(path, "rb");
	if (file) {
		DEMO *fdemo = (DEMO *)calloc(sizeof(DEMO), 1);
		fread(fdemo, sizeof(DEMO), 1, file);
		fseek(file, 0, SEEK_SET);
		DWORD size = sizeof(DEMO) + (fdemo->frame_count * sizeof(FRAME));
		fdemo = (DEMO *)realloc(fdemo, size);
		fread(fdemo, size, 1, file);

		wcscpy(demo.command, fdemo->command);
		for (DWORD i = 0; i < fdemo->frame_count; ++i) {
			demo.frames->push_back(fdemo->frames[i]);
		}

		free(fdemo);
		fclose(file);

		StartDemo();
	}
}

EXPORT void SaveDemo(char *path) {
	if (demo.command) {
		DWORD frames_size = demo.frames->size() * sizeof(FRAME);
		DWORD size = sizeof(DEMO) + frames_size;
		DEMO *fdemo = (DEMO *)calloc(size, 1);
		wcscpy(fdemo->command, demo.command);
		fdemo->frame_count = demo.frames->size();
		memcpy(&fdemo->frames[0], &(*demo.frames)[0], frames_size);

		FILE *file = fopen(path, "wb");
		if (file) {
			fwrite(fdemo, size, 1, file);
			fclose(file);
		}

		free(fdemo);
	}
}

EXPORT void StartDemo() {
	RemoveControl(CONTROL_PAUSE);
	Wait();
	demo.frame = demo.jump = 0;
	ExecuteCommand(demo.command);
}

EXPORT void GotoFrame(DWORD frame) {
	if (*demo.command && frame < demo.frames->size() && frame != demo.frame) {
		DWORD c = control;
		control = 0;

		Wait();
		if (frame == 0) {
			ExecuteCommand(demo.command);
			demo.frame = demo.jump = 0;
		} else {
			DisableRendering(true);
			if (frame < demo.frame) {
				ExecuteCommand(demo.command);
				demo.frame = 0;
			}
			demo.jump = frame;
			while (demo.jump) Sleep(1);
		}

		control = (c | CONTROL_PAUSE);
	}
}

EXPORT void GetDemoCommand(DWORD *out) {
	if (out) *out = (DWORD)&demo.command[0];
}

EXPORT void GetDemoFrame(DWORD *out) {
	if (out) *out = demo.frame;
}

EXPORT void GetDemoFrameCount(DWORD *out) {
	if (out && demo.frames) *out = demo.frames->size();
}

EXPORT void GetDemoFrames(DWORD *out) {
	if (out && demo.frames) *out = (DWORD)&(*demo.frames)[0];
}

EXPORT void SetTimescale(float scale) {
	delay = DELAY / scale;
}

EXPORT void GetTimescale(float *scale) {
	if (scale) *scale = DELAY / (float)delay;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, 0, 0, 0);
	}

	return TRUE;
}
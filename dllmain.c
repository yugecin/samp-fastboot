#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#ifdef _DEBUG
#define _dbg(X) OutputDebugStringA(X);
#define _dbgf(...) do{sprintf(szDebug,__VA_ARGS__);OutputDebugStringA(szDebug);}while(0);
char szDebug[200];
#else
#define _dbg(X)
#define _dbgf(X,Y,...)
#endif

HMODULE hSamp;

/**
 * This removes the 3s delay between the "SA-MP 0.3.7-R4 Started" message
 * and "Connecting to xxx...".
 */
static
void
remove_delay_between_start_and_first_connect()
{
	DWORD oldvp;
	unsigned short *addr;

	addr = (unsigned short*) ((int) hSamp + 0x8942 + 1); // cmp eax, 0BB8h (0.3.7)
	if (*addr != 0xBB8) {
		addr = (unsigned short*) ((int) hSamp + 0x8642 + 1); // cmp eax, 0BB8h (0.3.DL)
		if (*addr != 0xBB8) {
			return;
		}
	}
	VirtualProtect(addr, 2, PAGE_EXECUTE_READWRITE, &oldvp);
	*addr = 0;
	VirtualProtect(addr, 2, oldvp, &oldvp);
}

static
void
thread_runner_remove_wait_fade_in_mission_script()
{
	short *waitTime;
	short *fadeTime;

	// 0001: wait 500 ms
	waitTime = (short*) (0xA49960 + 0xC685);
	// 016A: fade 1 time 1000
	fadeTime = (short*) (0xA49960 + 0xC68A);
	do {
		Sleep(100);
	} while (*waitTime != 500 || *fadeTime != 1000);
	_dbg("script loaded, removing wait/fade");
	*waitTime = 0;
	*fadeTime = 0;
	ExitThread(0);
}
static
void
remove_wait_fade_in_mission_script()
{
	LPTHREAD_START_ROUTINE routine;
	HANDLE hThread;

	routine = (LPTHREAD_START_ROUTINE) thread_runner_remove_wait_fade_in_mission_script;
	hThread = CreateThread(NULL, 0, routine, NULL, 0, NULL);
	if (hThread) {
		CloseHandle(hThread);
	} else {
		_dbg("failed to start thread to remove wait/fade in mission script");
	}
}

static
void
experimental_remove_loading_bar()
{
	DWORD oldvp;

	VirtualProtect((void*) 0x590D00, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*(unsigned char*) 0x590D00 = 0xC3; // ret
}

static
void
__cdecl
logb(void *ret, char *a, char *b)
{
	_dbg("log");
	if (a[0]) {
		_dbg("a");
		_dbg(a);
	}
	if (b[0]) {
		_dbg("b");
		_dbg(b);
	}
}

__declspec(naked)
static
void loga(char *a, char *b)
{
	_asm {
		call logb
		//mov eax, 0x590D00
		//jmp eax
		ret
	}
}

static
void
experimental_hook_log()
{
	DWORD oldvp;
	int addr;

	addr = 0x53DED0;
	addr = 0x590240; // causes flashing 2nd loadscreen while game shows
	VirtualProtect((void*) addr, 5, PAGE_EXECUTE_READWRITE, &oldvp);
	*(unsigned char*) addr = 0xE9;
	*(unsigned int*) (addr+1) = ((int) loga - (addr + 5));
}

static
void
dont_draw_loading_bar()
{
	DWORD oldvp;

	VirtualProtect((void*) 0x590370, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*(unsigned char*) 0x590370 = 0xC3;
}

_declspec(naked)
static
void
set_gamestate_to_wait_connect_continue_f3_handler()
{
	_asm {
		mov eax, [esp+4]
		cmp eax, 0x72 // F3
		jnz nope
		mov eax, hSamp
		add eax, 0x26EA0C
		mov eax, [eax]
		mov dword ptr [eax+0x3CD], 1
nope:
		xor eax, eax
		ret
	}
}

static
void
rebind_f3_as_reconnect()
{
	int *addr;
	DWORD oldvp;

	addr = (int*) ((int) hSamp + 0x614E0);
	if (*addr != (int) hSamp + 0x614B0) {
		addr = (int*) ((int) hSamp + 0x60FA0);
		if (*addr != (int) hSamp + 0x60F70) {
			return;
		}
	}
	VirtualProtect(addr, 4, PAGE_EXECUTE_READWRITE, &oldvp);
	*addr = (int) set_gamestate_to_wait_connect_continue_f3_handler;
	VirtualProtect(addr, 4, oldvp, &oldvp);
}

BOOL
APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	DWORD oldvp;

	if (ul_reason_for_call != DLL_PROCESS_ATTACH) {
		return TRUE;
	}

	hSamp = GetModuleHandleA("samp.dll");
	if (hSamp) {
		remove_delay_between_start_and_first_connect();
		remove_wait_fade_in_mission_script();
		experimental_remove_loading_bar();
		dont_draw_loading_bar();
		//experimental_hook_log();
		//strcpy((void*) 0x866CD8, "nvidia"); // the first loading screen
		//strcpy((void*) 0x866CCC, "nvidia"); // the second loading screen
		//*(int*) 0xC8D4C0 = 5; // _gameState, 0=start with movies 2=crash 5=ok

		// remove a sleep
		//VirtualProtect((void*) 0x748E11, 8, PAGE_EXECUTE_READWRITE, &oldvp);
		//memset((void*) 0x748E11, 0x90, 8);

		// something with setting up camera? less iterations, 0=inf loop
		VirtualProtect((void*) 0x5909AB, 1, PAGE_EXECUTE_READWRITE, &oldvp);
		*(unsigned char*) 0x5909AB = 2;
		VirtualProtect((void*) 0x590A1E, 1, PAGE_EXECUTE_READWRITE, &oldvp);
		*(unsigned char*) 0x590A1E = 2;

		rebind_f3_as_reconnect();
	}
	return TRUE;
}

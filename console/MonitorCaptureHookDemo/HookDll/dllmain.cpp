// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <Windows.h>
#include <detours/detours.h>

static BOOL(WINAPI* TrueBitBlt)(HDC, int, int, int, int, HDC, int, int, DWORD) = BitBlt;

BOOL WINAPI MyBitBlt(
    HDC     hdcDest,
    int     nXDest,
    int     nYDest,
    int     nWidth,
    int     nHeight,
    HDC     hdcSrc,
    int     nXSrc,
    int     nYSrc,
    DWORD   dwRop)
{
    MessageBox(NULL, L"BitBlt Hooked...", L"BitBlt", MB_OK);
    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueBitBlt, MyBitBlt);
        DetourTransactionCommit();
        break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        /*DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueBitBlt, MyBitBlt);
        DetourTransactionCommit();*/
        break;
    }
    return TRUE;
}


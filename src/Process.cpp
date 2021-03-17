/*
 * PROJECT:     MemView
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     The process-selection menu
 * COPYRIGHT:   Copyright 2021 Mark Jansen (mark.jansen@reactos.org)
 */

#include "MemView.h"
#include "mfl/win32/tlhelp32.h"
#include <Psapi.h>
#include <map>

static BOOL g_IsRunningOnWow = -1;

static DWORD g_ProcessId;
HANDLE g_ProcessHandle;
std::wstring g_ProcessName;
static bool g_ProcessIsx86;

std::map<DWORD, HWND> g_Windows;

void UpdateStatic(HWND Static)
{
    if (Static)
    {
        WCHAR buf[MAX_PATH + 20];
        StringCchPrintfW(buf, _countof(buf), L"%s (%u%s)", g_ProcessName.c_str(), g_ProcessId, g_ProcessIsx86 ? L", x86" : L"");
        Static_SetText(Static, buf);
    }
}

static HANDLE InternalOpen(DWORD pid)
{
    return OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
}

bool OpenProcess(DWORD pid)
{
    if (pid == g_ProcessId) return true;
    if (g_ProcessHandle) CloseHandle(g_ProcessHandle);
    g_ProcessId = pid;
    g_ProcessHandle = InternalOpen(pid);
    if (g_ProcessHandle)
    {
        WCHAR buf[512];
        GetProcessImageFileNameW(g_ProcessHandle, buf, _countof(buf));
        g_ProcessName = buf;

        // When we are running as x86 on an x86 system, we do not care :)
        g_ProcessIsx86 = false;
        BOOL fIsWowProcess;
        if (IsWow64Process(g_ProcessHandle, &fIsWowProcess))
        {
            g_ProcessIsx86 = !!fIsWowProcess;
        }
    }
    return g_ProcessHandle != NULL;
}

static bool CanOpen(DWORD pid, bool& x86)
{
    HANDLE proc = InternalOpen(pid);
    if (proc)
    {
        bool canOpen = true;
        if (g_IsRunningOnWow == -1)
        {
            IsWow64Process(GetCurrentProcess(), &g_IsRunningOnWow);
        }
        BOOL fIsWowProcess;
        if (IsWow64Process(proc, &fIsWowProcess))
        {
            x86 = !!fIsWowProcess;
            if (g_IsRunningOnWow)
            {
                canOpen = fIsWowProcess == g_IsRunningOnWow;
            }
        }

        CloseHandle(proc);
        return canOpen;
    }
    return false;
}

static BOOL __stdcall enumProc2(HWND hwnd, LPARAM lParam)
{
    if (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == NULL)
    {
        DWORD pID = 0;
        GetWindowThreadProcessId(hwnd, &pID);
        g_Windows[pID] = hwnd;
    }

    return TRUE;
}

static void AddMenu(HMENU Menu, int Index, UINT AddType, DWORD pid, WCHAR* buffer)
{
    MENUITEMINFOW mii = { sizeof(mii) };
    mii.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA | MIIM_BITMAP | MIIM_FTYPE;
    mii.fType = MF_STRING | AddType;
    mii.wID = pid;
    mii.dwTypeData = buffer;
    mii.hbmpItem = HBMMENU_SYSTEM;  /* Use the icon from the window specified in dwItemData */
    ULONG_PTR wnd = (ULONG_PTR)g_Windows[pid];
    mii.dwItemData = wnd;
    InsertMenuItemW(Menu, Index, TRUE, &mii);
}

static HMENU CreateProcessMenu(UINT Height)
{
    HMENU Menu = NULL;
    g_Windows.clear();
    EnumWindows(enumProc2, NULL);

    Menu = CreatePopupMenu();
    WCHAR buffer[MAX_PATH + 20];
    UINT ItemHeight, Current = 0;
    ItemHeight = GetSystemMetrics(SM_CYMENUSIZE);
    int Num = 0;
    mfl::win32::ProcessIterator pi;
    while(pi.next())
    {
        bool x86 = false;
        if (CanOpen(pi->th32ProcessID, x86))
        {
            UINT flags = MF_STRING | MF_ENABLED;
            StringCchPrintfW(buffer, _countof(buffer), L"%s (%u%s)", pi->szExeFile, pi->th32ProcessID, x86 ? L", x86" : L"");
            Current += ItemHeight;
            AddMenu(Menu, Num++, (Current > Height) ? MF_MENUBARBREAK : 0, pi->th32ProcessID, buffer);
            if (Current > Height)
                Current = ItemHeight;
        }
    }
    return Menu;
}

bool UpdateProcessList(HWND Parent, UINT Height, int x, int y)
{
    HMENU menu = CreateProcessMenu(Height);
    if (menu)
    {
        INT n = TrackPopupMenuEx(menu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, x, y, Parent, NULL);
        PostMessageW(Parent, WM_NULL, 0, 0);
        DestroyMenu(menu);
        if (n)
        {
            return OpenProcess(n);
        }
    }
    return false;
}


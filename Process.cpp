#include "MemView.h"
#include <mfl/win32/tlhelp32.h>
#include <Psapi.h>
#include <map>


static DWORD g_ProcessId;
HANDLE g_ProcessHandle;
std::wstring g_ProcessName;
std::map<DWORD, HWND> g_Windows;

void UpdateStatic(HWND Static)
{
    if (Static)
    {
        TCHAR buf[MAX_PATH + 20];
        StringCchPrintf(buf, _countof(buf), TEXT("%s (%u)"), g_ProcessName.c_str(), g_ProcessId);
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
        TCHAR buf[512];
        GetProcessImageFileName(g_ProcessHandle, buf, _countof(buf));
        g_ProcessName = buf;
    }
    return g_ProcessHandle != NULL;
}

bool CanOpen(DWORD pid)
{
    HANDLE proc = InternalOpen(pid);
    if (proc)
    {
        CloseHandle(proc);
        return true;
    }
    return false;
}

BOOL __stdcall enumProc2(HWND hwnd, LPARAM lParam)
{
    if (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == NULL)
    {
        DWORD pID = 0;
        GetWindowThreadProcessId(hwnd, &pID);
        g_Windows[pID] = hwnd;
    }

    return TRUE;
}

void AddMenu(HMENU Menu, int Index, UINT AddType, DWORD pid, TCHAR* buffer)
{
    MENUITEMINFO mii = { sizeof(mii) };
    mii.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA | MIIM_BITMAP | MIIM_FTYPE;
    mii.fType = MF_STRING | AddType;
    mii.wID = pid;
    mii.dwTypeData = buffer;
    mii.hbmpItem = HBMMENU_SYSTEM;
    mii.dwItemData = (ULONG_PTR)g_Windows[pid];
    InsertMenuItem(Menu, Index, TRUE, &mii);
}

HMENU CreateProcessMenu(UINT Height)
{
    HMENU Menu = NULL;
    g_Windows.clear();
    EnumWindows(enumProc2, NULL);

    Menu = CreatePopupMenu();
    TCHAR buffer[MAX_PATH + 20];
    UINT ItemHeight, Current = 0;
    ItemHeight = GetSystemMetrics(SM_CYMENUSIZE);
    int Num = 0;
    mfl::win32::ProcessIterator pi;
    while(pi.next())
    {
        if (CanOpen(pi->th32ProcessID))
        {
            UINT flags = MF_STRING | MF_ENABLED;
            StringCchPrintf(buffer, _countof(buffer), TEXT("%s (%u)"), pi->szExeFile, pi->th32ProcessID);
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
        PostMessage(Parent, WM_NULL, 0, 0);
        DestroyMenu(menu);
        if (n)
            return OpenProcess(n);
    }
    return false;
}


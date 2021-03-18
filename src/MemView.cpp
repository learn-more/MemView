/*
 * PROJECT:     MemView
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     The hex-dump window
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "MemView.h"
#include "MemInfo.h"
#include <algorithm>

extern HINSTANCE g_hInst;

// http://www.catch22.net/tuts/scrollbars-scrolling

struct MemView
{
    MemView(const std::wstring& Name, DWORD pid, const MemInfo& info)
        :ProcessName(Name), ProcessPid(pid), Info(info)
        , ProcessHandle(NULL), DisplayLines(0), PerLine(16)
        , ScrollMax(0), ScrollPos(0)
        , vMax(0), vPos(0), vInc(0)
        , Dirty(true)
        , FontX(0), FontY(0), FontNL(0)
    {
        updateTotal();
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &WheelLines, 0);
        if (WheelLines <= 1)
            WheelLines = 3;
    }

    void updateTotal()
    {
        TotalLines = (Info.size()+(PerLine-1)) / PerLine;
    }

    std::wstring ProcessName;
    DWORD ProcessPid;
    HANDLE ProcessHandle;
    MemInfo Info;

    size_t TotalLines;
    size_t DisplayLines;
    size_t PerLine;
    int ScrollMax;
    int ScrollPos;
    long vMax;
    long vPos;
    int vInc;
    int WheelLines;

    bool Dirty;
    std::vector<unsigned char> Buffer;

    int FontX;
    int FontY;
    int FontNL;
};

static WCHAR Hex2Str[] = L"0123456789abcdef";

void ToBuffer(WCHAR* Buffer, size_t Cch, unsigned char* Data, SIZE_T DataLen, SIZE_T PerLine, PBYTE Address)
{
    StringCchPrintfW(Buffer, Cch, L"%p:  ", Address);
    WCHAR* p = Buffer + wcslen(Buffer);
    for(size_t n = 0; n < PerLine; ++n)
    {
        if (n < DataLen)
        {
            *(p++) = Hex2Str[Data[n] >> 4];
            *(p++) = Hex2Str[Data[n] & 0xf];
            *(p++) = ' ';
        }
        else
        {
            *(p++) = ' ';
            *(p++) = ' ';
            *(p++) = ' ';
        }
    }
    *(p++) = ' ';
    *(p++) = ' ';
    for(size_t n = 0; n < PerLine; ++n)
    {
        if (n < DataLen)
        {
            if (isprint(Data[n]))
                *(p++) = (char)Data[n];
            else
                *(p++) = '.';
        }
    }
    *p = 0;
}

static void ReadMemory(HWND hwnd, MemView* mv)
{
    SIZE_T Requested = mv->Buffer.size();
    SIZE_T Read = 0;

    MemInfo& info = mv->Info;

    PBYTE start = info.start() + (mv->vPos*mv->PerLine);
    PBYTE end = info.start() + info.size();
    if ((Requested + start) > end)
        Requested -= ((Requested + start)-end);

    std::vector<unsigned char> buf = mv->Buffer;
    ReadProcessMemory(mv->ProcessHandle, start, mv->Buffer.data(), Requested, &Read);
    mv->Dirty = false;
    if (buf != mv->Buffer)
    {
        InvalidateRect(hwnd, NULL, FALSE);
    }
    if (Requested != Read)
    {
        if (Read != 0)
            __debugbreak();
        OutputDebugString(TEXT("FAIL\n"));
    }
}

LRESULT HandleWM_PAINT(HWND hwnd, MemView* mv)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    if (mv->Dirty)
        ReadMemory(hwnd, mv);

    WCHAR Buffer[512];
    SelectObject(hdc, getFont());
    size_t PerLine = mv->PerLine;
    for(size_t n = 0; n < mv->DisplayLines; ++n)
    {
        size_t offset = (mv->vPos*PerLine) + (n*PerLine);
        size_t Left = std::min<size_t>(PerLine, mv->Info.size() - offset);
        ToBuffer(Buffer, _countof(Buffer), mv->Buffer.data() + (n*PerLine), Left, PerLine, mv->Info.start() + offset);
        TextOutW(hdc, 2, mv->FontY * (int)n, Buffer, (int)wcslen(Buffer));
    }

    EndPaint(hwnd, &ps);
    return 0l;
}

void HandleWM_SIZE(HWND hwnd, MemView* mv, LPARAM lParam)
{
    WORD ClientHeight = HIWORD(lParam);
    WORD ClientWidth = LOWORD(lParam);
    ClientWidth -= GetSystemMetrics(SM_CXVSCROLL);

    size_t DefaultOverhead = sizeof(void*) + 5 + 2;
    size_t NumBytes = 8;
    while ((((NumBytes<<1)*4+DefaultOverhead)* mv->FontX) < ClientWidth)
        NumBytes <<= 1;
    mv->PerLine = NumBytes;

    mv->TotalLines = (mv->Info.size()+(mv->PerLine-1)) / mv->PerLine + 1;

    mv->DisplayLines = ClientHeight / mv->FontY + 1;
    mv->Buffer.resize(mv->DisplayLines * mv->PerLine);

    mv->vMax = mv->TotalLines - mv->DisplayLines;
    mv->vInc = 1;
    mv->ScrollMax = std::max(0, (int)mv->vMax);
    if (mv->ScrollMax != mv->vMax)
    {
        mv->vInc = (mv->vMax / INT_MAX) + 1;
        mv->ScrollMax = INT_MAX;
    }
    mv->ScrollPos = std::min(mv->ScrollPos, mv->ScrollMax);
    mv->vPos = std::min((long)mv->ScrollPos * mv->vInc, mv->vMax);
    mv->Dirty = true;
    SetScrollRange(hwnd, SB_VERT, 0, mv->ScrollMax, FALSE);
    SetScrollPos(hwnd, SB_VERT, mv->ScrollPos, TRUE);
}

static void HandleWM_VSCROLL(HWND hwnd, MemView* mv, WPARAM wParam, LPARAM lParam)
{
    SCROLLINFO si = { sizeof(si), 0 };
    si.fMask = SIF_TRACKPOS;
    GetScrollInfo(hwnd, SB_VERT, &si);

    int nVscrollInc;
    switch (GET_WM_VSCROLL_CODE(wParam, lParam))
    {
    case SB_TOP:
        nVscrollInc = -mv->ScrollPos;
        break;
    case SB_BOTTOM:
        nVscrollInc = mv->ScrollMax  - mv->ScrollPos;
        break;
    case SB_LINEUP:
        nVscrollInc = -1;
        break;
    case SB_LINEDOWN:
        nVscrollInc = 1;
        break;
    case SB_PAGEUP:
        nVscrollInc = -std::max<int>(1, (int)mv->DisplayLines);
        break;
    case SB_PAGEDOWN:
        nVscrollInc = std::max<int>(1, (int)mv->DisplayLines);
        break;
    case SB_THUMBPOSITION:
        nVscrollInc = si.nTrackPos - mv->ScrollPos;
        break;
    case SB_THUMBTRACK:
        nVscrollInc = si.nTrackPos - mv->ScrollPos;
        break;
    case 123:
        nVscrollInc = (-(int)lParam/120) * mv->WheelLines;
        break;
    default:
        nVscrollInc = 0;
        break;
    }

    nVscrollInc = std::max(-mv->ScrollPos, std::min(nVscrollInc, mv->ScrollMax - mv->ScrollPos));
    if (nVscrollInc)
    {
        mv->ScrollPos += nVscrollInc;
        mv->vPos = std::min((long)mv->ScrollPos * mv->vInc, mv->vMax);
        mv->Dirty = true;
        SetScrollPos(hwnd, SB_VERT, mv->ScrollPos, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
}

static void CreateFont(HWND hwnd, MemView* mv)
{
    HDC hdc = GetDC(hwnd);
    SelectObject(hdc, getFont());
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    mv->FontX = tm.tmAveCharWidth;
    mv->FontY = tm.tmHeight + tm.tmExternalLeading;
    mv->FontNL = tm.tmHeight;
    ReleaseDC(hwnd, hdc);
}

const int kClassIndex = 0;

static MemView* GetPtr(HWND hwnd)
{
    return reinterpret_cast<MemView*>(GetWindowLongPtr(hwnd, kClassIndex));
}

static void SetPtr(HWND hwnd, MemView* mv)
{
    SetWindowLongPtr(hwnd, kClassIndex, reinterpret_cast<LONG_PTR>(mv));
}

LRESULT CALLBACK MemWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        WCHAR Buffer[512];
        MemView* mv = static_cast<MemView*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetPtr(hwnd, mv);
        StringCchPrintfW(Buffer, _countof(Buffer), L"%s (%u), %p - %p",
            mv->ProcessName.c_str(), mv->ProcessPid, mv->Info.start(), mv->Info.start() + mv->Info.size());
        SetWindowTextW(hwnd, Buffer);
        CreateFont(hwnd, mv);
        //ReadMemory(hwnd, mv);
        SetTimer(hwnd, 0x1ea4, 1000, NULL);
    }
        break;
    case WM_DESTROY:
    {
        KillTimer(hwnd, 0x1ea4);
        MemView* mv = GetPtr(hwnd);
        SetPtr(hwnd, NULL);
        CloseHandle(mv->ProcessHandle);
        delete mv;
        SetFocus(GetParent(hwnd));
    }
        break;

    // fix vertical stuff, then this works
    //case WM_ERASEBKGND:
    //    return 0;

    case WM_PAINT:
        HandleWM_PAINT(hwnd, GetPtr(hwnd));
        break;

    case WM_MOUSEWHEEL:
        HandleWM_VSCROLL(hwnd, GetPtr(hwnd), 123, (SHORT)HIWORD(wParam));
        break;

    case WM_VSCROLL:
        HandleWM_VSCROLL(hwnd, GetPtr(hwnd), wParam, lParam);
        break;

    case WM_SIZE:
        HandleWM_SIZE(hwnd, GetPtr(hwnd), lParam);
        break;

    case WM_TIMER:
        if (wParam == 0x1ea4)
        {
            ReadMemory(hwnd, GetPtr(hwnd));
        }
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#define MEMVIEW_CLASS TEXT("MemViewClass")

void ShowMemory(HWND Parent, const MemInfo& info, HANDLE Handle, const std::wstring& Title)
{
    WNDCLASSEX wc = { sizeof(wc), 0 };
    if (!GetClassInfoEx(g_hInst, MEMVIEW_CLASS, &wc))
    {
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MemWndProc;
        wc.hInstance = g_hInst;
        wc.hIcon = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszClassName = MEMVIEW_CLASS;
        wc.cbWndExtra = sizeof(MemView*);
        setIcons(wc);

        if (!RegisterClassEx(&wc))
            return;
    }

    std::wstring::size_type off = Title.find_last_of(L"\\/");
    off = (off == std::wstring::npos) ? 0 : (off+1);

    MemView* mi = new MemView(Title.substr(off), GetProcessId(Handle), info);
    DuplicateHandle(GetCurrentProcess(), Handle, GetCurrentProcess(), &mi->ProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

    HWND Window = CreateWindow(TEXT("MemViewClass"), TEXT("Mem"), WS_OVERLAPPEDWINDOW | WS_VSCROLL,
            CW_USEDEFAULT, CW_USEDEFAULT, 580, 400, Parent, NULL, g_hInst, mi);
    ShowWindow(Window, SW_SHOW);
    UpdateWindow(Window);
}

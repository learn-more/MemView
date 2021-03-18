/*
 * PROJECT:     MemView
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common project header
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#pragma once

#define NOMINMAX
#define _USING_V110_SDK71_ 1
#define PSAPI_VERSION 1
#pragma warning(disable : 4995)

#include <sdkddkver.h>
#include <Windows.h>
#include <windowsx.h>
#include <strsafe.h>
#include <string>


extern HINSTANCE g_hInst;
extern HANDLE g_ProcessHandle;
extern std::wstring g_ProcessName;

// Common resources
HFONT getFont();
HICON getExpandIcon();
HICON getCollapseIcon();

// Set the window icon
void setIcons(WNDCLASSEX& wc);

void MemInfo_InitProcess(HANDLE hProcess);

void UpdateStatic(HWND Static);
bool UpdateProcessList(HWND Parent, UINT Height, int x, int y);
void ShowMemory(HWND Parent, const class MemInfo& info, HANDLE Handle, const std::wstring& Title);

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool OpenProcess(DWORD pid);

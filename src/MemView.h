#pragma once

#define NOMINMAX
#define _USING_V110_SDK71_ 1
#define PSAPI_VERSION 1
#pragma warning(disable : 4995)

#include <sdkddkver.h>
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <strsafe.h>

HFONT getFont();

HICON getExpandIcon();
HICON getCollapseIcon();

void setWindowIcons(HWND hWnd);

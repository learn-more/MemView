/*
 * PROJECT:     mfl
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Toolhelp32 wrapper, allowing it to be used as pseudo-iterator
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#pragma once

#if !defined(TH32CS_SNAPPROCESS)
#include <TlHelp32.h>
#endif

namespace mfl
{

namespace win32
{

template<typename DataType, BOOL (__stdcall*First)(HANDLE, DataType*), BOOL(__stdcall*Next)(HANDLE, DataType*), int SnapType>
struct ToolhelpIterator
{
    ToolhelpIterator(DWORD pid = 0)
        :mInitial(true)
    {
        Data.dwSize = sizeof(Data);
        mSnapshot = CreateToolhelp32Snapshot(SnapType, pid);
    }
    ~ToolhelpIterator()
    {
        if (mSnapshot != INVALID_HANDLE_VALUE)
            CloseHandle(mSnapshot);
    }

    bool next()
    {
        if (mSnapshot == INVALID_HANDLE_VALUE) return false;
        if (mInitial)
        {
            mInitial = false;
            return !!First(mSnapshot, &Data);
        }
        return !!Next(mSnapshot, &Data);
    }

    DataType* operator->()
    {
        return &Data;
    }

    DataType Data;
private:
    HANDLE mSnapshot;
    bool mInitial;
};

typedef ToolhelpIterator<PROCESSENTRY32, Process32First, Process32Next, TH32CS_SNAPPROCESS> ProcessIterator;
typedef ToolhelpIterator<MODULEENTRY32, Module32First, Module32Next, TH32CS_SNAPMODULE> ModuleIterator;
typedef ToolhelpIterator<MODULEENTRY32, Module32First, Module32Next, TH32CS_SNAPMODULE32> ModuleIterator32;
typedef ToolhelpIterator<MODULEENTRY32, Module32First, Module32Next, TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32> ModuleIterator3264;
typedef ToolhelpIterator<THREADENTRY32, Thread32First, Thread32Next, TH32CS_SNAPTHREAD> ThreadIterator;

} // namespace win32

} // namespace mfl
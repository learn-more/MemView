/*
 * PROJECT:     MemView
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Memory information helper function
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "MemView.h"
#include <Psapi.h>
#include "MemInfo.h"


static LPSYSTEM_INFO g_Info = nullptr;


const wchar_t* Prot2Str(DWORD prot)
{
    switch (prot & 0x1ff)
    {
    case 0:                                     return L"   ";
    case PAGE_EXECUTE:                          return L"  E";
    case PAGE_EXECUTE_READ:                     return L"R E";
    case PAGE_EXECUTE_READWRITE:                return L"RWE";
    case PAGE_EXECUTE_WRITECOPY:                return L"RWE COW";
    case PAGE_NOACCESS:                         return L"NoAccess";
    case PAGE_READONLY:                         return L"R  ";
    case PAGE_READWRITE:                        return L"RW ";
    case PAGE_WRITECOPY:                        return L"RW  COW";
    case 0 | PAGE_GUARD:                        return L"    Guard";
    case PAGE_EXECUTE | PAGE_GUARD:             return L"  E Guard";
    case PAGE_EXECUTE_READ | PAGE_GUARD:        return L"R E Guard";
    case PAGE_EXECUTE_READWRITE | PAGE_GUARD:   return L"RWE Guard";
    case PAGE_EXECUTE_WRITECOPY | PAGE_GUARD:   return L"RWE COW Guard";
    case PAGE_NOACCESS | PAGE_GUARD:            return L"NoAccess Guard";
    case PAGE_READONLY | PAGE_GUARD:            return L"R   Guard";
    case PAGE_READWRITE | PAGE_GUARD:           return L"RW  Guard";
    case PAGE_WRITECOPY | PAGE_GUARD:           return L"RW  COW Guard";
    default: __debugbreak();                    return L"";
    }
}

Info& operator|= (Info& left, const Info& right)
{
    left = left | right;
    return left;
}

Info operator| (const Info& left, const Info& right)
{
    return static_cast<Info>(static_cast<int>(left) | static_cast<int>(right));
}

Info operator& (const Info& left, const Info& right)
{
    return static_cast<Info>(static_cast<int>(left) & static_cast<int>(right));
}

MemInfo::MemInfo()
{
    memset(&mInfo, 0, sizeof(mInfo));
    mChanged = Info::None;
}

MemInfo::MemInfo(const MEMORY_BASIC_INFORMATION& info)
    :mInfo(info), mChanged(Info::Address | Info::Size | Info::Type | Info::Protection | Info::AllocationProtection | Info::Mapped)
{
}

MemInfo::~MemInfo()
{
}

Info MemInfo::Index2Info(int index)
{
    return static_cast<Info>(1 << index);
}

void MemInfo::columnText(__out_ecount(cchDest) STRSAFE_LPWSTR pszDest, __in size_t cchDest, int Index) const
{
    Info type = Index2Info(Index);
    LPCWSTR TypeName;
    switch (type)
    {
    case Info::Address:
        // Indent sections
        StringCchPrintf(pszDest, cchDest, TEXT("%s%p"), start() == allocationStart() ? TEXT("") : TEXT(" "), mInfo.BaseAddress);
        break;
    case Info::Size:
        StringCchPrintf(pszDest, cchDest, TEXT("%08x"), mInfo.RegionSize);
        break;
    case Info::Type:
        if (isImage())
            TypeName = L"Imag";
        else if (isMapped())
            TypeName = L"Map";
        else if (isPrivate())
            TypeName = L"Priv";
        else
            TypeName = L"Other";
        StringCchCopy(pszDest, cchDest, TypeName);
        break;
    case Info::Protection:
        StringCchCopy(pszDest, cchDest, mInfo.State != MEM_RESERVE ? Prot2Str(mInfo.Protect) : L"");
        break;
    case Info::AllocationProtection:
        StringCchCopy(pszDest, cchDest, Prot2Str(mInfo.AllocationProtect));
        break;
    case Info::Mapped:
        StringCchCopy(pszDest, cchDest, mMapped.c_str());
        break;
    }
}

int MemInfo::cmp(const MemInfo& info) const
{
    return info.mInfo.BaseAddress == mInfo.BaseAddress ? 0 : ((mInfo.BaseAddress < info.mInfo.BaseAddress) ? -1 : 1);
}

void MemInfo::update(const MemInfo& info)
{
    mChanged = (mChanged != Info::None && mChanged != Info::Color ) ?  Info::Color : Info::None;
    if (info.mInfo.BaseAddress != mInfo.BaseAddress) mChanged |= Info::Address;
    if (info.mInfo.RegionSize != mInfo.RegionSize) mChanged |= Info::Size;
    if (info.mInfo.Type != mInfo.Type) mChanged |= Info::Type;
    if (info.mInfo.Protect != mInfo.Protect) mChanged |= Info::Protection;
    if (info.mInfo.AllocationProtect != mInfo.AllocationProtect) mChanged |= Info::AllocationProtection;
    if (info.mMapped != mMapped) mChanged |= Info::Mapped;
    mInfo = info.mInfo;
    mMapped = info.mMapped;
}


void MemInfo::read(HANDLE hProcess, std::vector<MemInfo>& items)
{
    items.clear();

    if (!g_Info)
    {
        LPSYSTEM_INFO info = new SYSTEM_INFO;
        //SYSTEM_INFO si; GetNativeSystemInfo(&si);
        GetNativeSystemInfo(info);
        if (InterlockedExchangePointer((void**)&g_Info, info) != nullptr)
            delete info;
    }

    for (PBYTE addr = (PBYTE)g_Info->lpMinimumApplicationAddress; addr < g_Info->lpMaximumApplicationAddress;)
    {
        MEMORY_BASIC_INFORMATION mbi = { 0 };
        wchar_t buf[MAX_PATH];
        if (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) == sizeof(mbi))
        {
            if (mbi.State != MEM_FREE)
            {
                items.push_back(MemInfo(mbi));
                DWORD num = GetMappedFileName(hProcess, mbi.BaseAddress, buf, _countof(buf));
                if (num != 0)
                {
                    items.back().mMapped = buf;
                }
            }
            addr = (PBYTE)mbi.BaseAddress + mbi.RegionSize;
        }
        else
        {
            addr += g_Info->dwPageSize;
        }
    }
}


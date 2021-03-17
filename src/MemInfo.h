/*
 * PROJECT:     MemView
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Memory information container
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#pragma once

#include <vector>
#include <memory>

enum class Info
{
    None = 0,
    Address = (1<<0),
    Size = (1 << 1),
    Type = (1 << 2),
    Protection = (1 << 3),
    AllocationProtection = (1 << 4),
    Mapped = (1 << 5),

    Color = (1<<31),
};

Info& operator|= (Info& left, const Info& right);
Info operator| (const Info& left, const Info& right);
Info operator& (const Info& left, const Info& right);

class MemInfo
{
public:
    MemInfo();
    ~MemInfo();

    void columnText(__out_ecount(cchDest) STRSAFE_LPWSTR pszDest, __in size_t cchDest, int Index) const;
    Info changed() const { return mChanged; }

    int cmp(const MemInfo& info) const;
    void update(const MemInfo& info);

    static Info Index2Info(int index);
    static void read(HANDLE hProcess, std::vector<std::unique_ptr<MemInfo>>& items);

    PBYTE start() const { return static_cast<PBYTE>(mInfo.BaseAddress); }
    SIZE_T size() const { return mInfo.RegionSize; }
    PBYTE allocationStart() const { return static_cast<PBYTE>(mInfo.AllocationBase); }

    bool isImage() const { return mInfo.Type == MEM_IMAGE; }
    bool isMapped() const { return mInfo.Type == MEM_MAPPED; }
    bool isPrivate() const { return mInfo.Type == MEM_PRIVATE; }

    bool CanExpand = false;
    bool IsExpanded = false;

protected:
    MemInfo(const MEMORY_BASIC_INFORMATION& info);

    MEMORY_BASIC_INFORMATION mInfo;
    std::wstring mMapped;
    Info mChanged;
};


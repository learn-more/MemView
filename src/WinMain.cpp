/*
 * PROJECT:     MemView
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Entry point / common resources
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "MemView.h"
#include <Commctrl.h>
#include "../res/resource.h"
#include "version.h"

// Common controls 6.0 are required for the SysLink control
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


#pragma comment (lib, "Comctl32.lib")
#pragma comment(lib, "Psapi.lib")

HINSTANCE g_hInst;


static HFONT g_Font = NULL;
HFONT getFont()
{
    if (!g_Font)
    {
        HRSRC fntResource = FindResourceW(g_hInst, MAKEINTRESOURCEW(IDR_HACK), L"TTF");
        if (fntResource)
        {
            HGLOBAL fntHandle = LoadResource(g_hInst, fntResource);
            LPVOID fntData = LockResource(fntHandle);
            DWORD fntSize = SizeofResource(g_hInst, fntResource);
            DWORD numInstalled = 0;
            if (AddFontMemResourceEx(fntData, fntSize, NULL, &numInstalled) && numInstalled)
            {
                LOGFONT lf = { 0 };
                lf.lfHeight = -11;
                lf.lfWeight = FW_NORMAL;
                StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), TEXT("HACK"));
                g_Font = CreateFontIndirect(&lf);
            }
        }

        if (!g_Font)
        {
            LOGFONT lf = { 0 };
            lf.lfHeight = -11;
            lf.lfWeight = FW_NORMAL;
            StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), TEXT("Consolas"));
            g_Font = CreateFontIndirect(&lf);
        }
    }

    return g_Font;
}

static HICON g_ExpandIcon = NULL;
HICON getExpandIcon()
{
    if (g_ExpandIcon == NULL)
    {
        g_ExpandIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_EXPAND), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    }

    return g_ExpandIcon;
}

static HICON g_CollapseIcon = NULL;
HICON getCollapseIcon()
{
    if (g_CollapseIcon == NULL)
    {
        g_CollapseIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_COLLAPSE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    }

    return g_CollapseIcon;
}

static HICON g_MainIcon = NULL;
static HICON g_MainIconSm = NULL;

void setIcons(WNDCLASSEX& wc)
{
    if (!g_MainIcon)
    {
        g_MainIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON,
                                      GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        g_MainIconSm = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }

    wc.hIcon = g_MainIcon;
    wc.hIconSm = g_MainIconSm;
}

void OnInitAboutDlg(HWND hDlg)
{
    HWND hWndParent;
    RECT rc;
    int w, h;

    hWndParent = GetParent(hDlg);
    GetWindowRect(hDlg, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    GetWindowRect(hWndParent, &rc);
    SetWindowPos(hDlg, HWND_TOP,
                 rc.left + ((rc.right - rc.left) - w) / 2,
                 rc.top + ((rc.bottom - rc.top) - h) / 2,
                 0, 0, SWP_NOSIZE);

    SetDlgItemTextA(hDlg, IDC_VERSION, "<a href=\"https://learn-more.github.io/MemView/\">MemView</a> " GIT_VERSION_STR);
}

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {
    case WM_INITDIALOG:
        OnInitAboutDlg(hDlg);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
        case IDOK:
            EndDialog(hDlg, 0);
            break;
        }
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case NM_CLICK:
        case NM_RETURN:
        {
            PNMLINK pNMLink = (PNMLINK)lParam;
            if (pNMLink->hdr.idFrom == IDC_VERSION || pNMLink->hdr.idFrom == IDC_CREDITS)
            {
                SHELLEXECUTEINFOW shExInfo = { sizeof(shExInfo) };
                shExInfo.lpVerb = L"open";
                shExInfo.hwnd = hDlg;
                shExInfo.fMask = SEE_MASK_UNICODE | SEE_MASK_NOZONECHECKS | SEE_MASK_NOASYNC;
                shExInfo.lpFile = pNMLink->item.szUrl;
                shExInfo.nShow = SW_SHOW;

                CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
                ShellExecuteExW(&shExInfo);
                CoUninitialize();
            }
        }
        break;
        }
    default:
        break;
    }
    return FALSE;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX icex;
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS;
    InitCommonControlsEx(&icex);
    g_hInst = hInstance;


    WNDCLASSEX wc = { sizeof(wc), 0 };
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wc.lpszClassName = L"MemListClass";
    setIcons(wc);

    if (!RegisterClassEx(&wc))
        return FALSE;

    OpenProcess(GetCurrentProcessId());

    HWND hwndMain = CreateWindowEx(WS_EX_CONTROLPARENT, L"MemListClass", L"MemView", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    BOOL bRet;
    MSG Msg;
    while((bRet = GetMessageW(&Msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            DestroyWindow(hwndMain);
            ExitProcess(GetLastError());
        }
        if (!IsDialogMessageW(hwndMain, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }

        /* Just spy for messages here. This way we also see it when a child control has focus */
        if (Msg.message == WM_KEYUP && Msg.wParam == VK_F1)
        {
            DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_ABOUTBOX), hwndMain, AboutProc, 0L);
        }
    }
    return (int)Msg.wParam;
}

#include "MemView.h"
#include <Commctrl.h>
#include <mfl/win32/msg2text/wm.hpp>
#include "resource.h"

#pragma comment (lib, "Comctl32.lib")
#pragma comment(lib, "Psapi.lib")

HINSTANCE g_hInst;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool OpenProcess(DWORD);


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
void setWindowIcons(HWND hWnd)
{
    if (g_MainIcon == NULL)
    {
        g_MainIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON,
                                      GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        g_MainIconSm = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }

    SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM)g_MainIcon);
    SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_MainIconSm);
}


int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX icex;
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    g_hInst = hInstance;

    WNDCLASSEX wc = { sizeof(wc), 0 };
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wc.lpszClassName = L"MemListClass";

    if (!RegisterClassEx(&wc))
        return FALSE;

    OpenProcess(GetCurrentProcessId());

    HWND hwndMain = CreateWindowEx(WS_EX_CONTROLPARENT, L"MemListClass", L"MemView", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessage(msg.hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}

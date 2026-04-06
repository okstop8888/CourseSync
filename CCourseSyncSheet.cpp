#include "stdafx.h"
#include "CCourseSyncSheet.h"
#include "CourseSync.h"
#include <shellapi.h>

#define WM_TRAY_MSG (WM_USER + 100)

IMPLEMENT_DYNAMIC(CCourseSyncSheet, CPropertySheet)

BEGIN_MESSAGE_MAP(CCourseSyncSheet, CPropertySheet)
    ON_WM_CLOSE()
    ON_WM_SYSCOMMAND()
    ON_MESSAGE(WM_TRAY_MSG, OnTrayMsg)
END_MESSAGE_MAP()

CCourseSyncSheet::CCourseSyncSheet(LPCTSTR pszCaption, CWnd *pParentWnd)
    : CPropertySheet(pszCaption, pParentWnd)
{}

BOOL CCourseSyncSheet::OnInitDialog()
{
    BOOL ret = CPropertySheet::OnInitDialog();

    NOTIFYICONDATA nid = {};
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = m_hWnd;
    nid.uID              = 1;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY_MSG;
    nid.hIcon            = AfxGetApp()->LoadIcon(IDI_APP);
    if (!nid.hIcon) nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    _tcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), _T("CourseSync"));
    Shell_NotifyIcon(NIM_ADD, &nid);

    ModifyStyleEx(0, WS_EX_TOOLWINDOW, SWP_FRAMECHANGED);
    SetWindowText(_T("CourseSync v2.0"));

    return ret;
}

void CCourseSyncSheet::OnClose()
{
    ShowWindow(SW_HIDE);
}

void CCourseSyncSheet::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == SC_MINIMIZE) {
        ShowWindow(SW_HIDE);
        return;
    }
    CPropertySheet::OnSysCommand(nID, lParam);
}

LRESULT CCourseSyncSheet::OnTrayMsg(WPARAM, LPARAM lParam)
{
    if (lParam == WM_LBUTTONDBLCLK) {
        ShowWindow(SW_SHOW);
        SetForegroundWindow();
    } else if (lParam == WM_RBUTTONUP) {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, 1, _T("Show CourseSync"));
        menu.AppendMenu(MF_STRING, 2, _T("Sync Now"));
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, 3, _T("Exit"));

        SetForegroundWindow();
        POINT pt;
        GetCursorPos(&pt);
        int cmd = (int)menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, this);

        if (cmd == 1) { ShowWindow(SW_SHOW); SetForegroundWindow(); }
        else if (cmd == 2) { theApp.GetSyncPage().PostMessage(WM_COMMAND, IDC_BTN_SYNC_NOW); }
        else if (cmd == 3) {
            theApp.SaveSettings();
            NOTIFYICONDATA nid2 = {};
            nid2.cbSize = sizeof(nid2);
            nid2.hWnd   = m_hWnd;
            nid2.uID    = 1;
            Shell_NotifyIcon(NIM_DELETE, &nid2);
            DestroyWindow();
            PostQuitMessage(0);
        }
    }
    return 0;
}

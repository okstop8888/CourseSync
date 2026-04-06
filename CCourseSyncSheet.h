#pragma once

// 主 PropertySheet，覆盖关闭行为以支持托盘隐藏
class CCourseSyncSheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CCourseSyncSheet)
public:
    CCourseSyncSheet(LPCTSTR pszCaption, CWnd *pParentWnd = nullptr);

protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg LRESULT OnTrayMsg(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};

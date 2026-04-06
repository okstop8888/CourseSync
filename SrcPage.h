#pragma once
#include "resource.h"
#include "CourseData.h"
#include "DbManager.h"

class SrcPage : public CPropertyPage
{
    DECLARE_DYNAMIC(SrcPage)
public:
    SrcPage();

    DbConfig getConfig() const;
    void     applyConfig(const DbConfig &cfg);

    CString  getSelectedTable() const;
    void     setSelectedTable(const CString &table);

    enum { IDD = IDD_PAGE_SRC };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange *pDX);

    afx_msg void OnBtnTestSrc();
    afx_msg void OnBtnRefreshTable();
    afx_msg void OnCbnSelchangeDb();
    afx_msg void OnCbnSelchangeTable();
    DECLARE_MESSAGE_MAP()

private:
    void refreshColumnPreview(const CString &table);
    void setStatus(const CString &msg, COLORREF color = RGB(0,128,0));

    CEdit      m_editHost;
    CEdit      m_editPort;
    CEdit      m_editUser;
    CEdit      m_editPass;
    CComboBox  m_cmbDb;       // populated after Test Connection
    CComboBox  m_cmbTable;
    CListCtrl  m_listCols;
    CStatic    m_lblStatus;

    DbManager *m_db;
};

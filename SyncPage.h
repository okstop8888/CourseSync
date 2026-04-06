#pragma once
#include "resource.h"
#include "CourseData.h"
#include "ApiManager.h"
#include "CourseConverter.h"

class SyncPage : public CPropertyPage
{
    DECLARE_DYNAMIC(SyncPage)
public:
    SyncPage();

    SyncOptions    getSyncOptions()    const;
    ScheduleConfig getScheduleConfig() const;
    void           applyScheduleConfig(const ScheduleConfig &cfg);
    void           applySyncOptions(const SyncOptions &opts);

    void AppendLog(const CString &msg);
    void SetProgress(int val, int max = 100);

    enum { IDD = IDD_PAGE_SYNC };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange *pDX);

    afx_msg void OnBtnSyncNow();
    afx_msg void OnBtnClearLog();
    afx_msg void OnChkAutoSync();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

private:
    void StartAutoTimer();
    void StopAutoTimer();
    void UpdateNextSyncLabel();
    void DoFullSync();

    CButton       m_chkPushPlus;
    CButton       m_chkSkipConflict;
    CButton       m_chkMatchById;
    CEdit         m_editSchedValue;
    CComboBox     m_cmbSchedUnit;
    CButton       m_chkAutoSync;
    CProgressCtrl m_progress;
    CEdit         m_editLog;
    CStatic       m_lblNextSync;
    CStatic       m_lblStats;

    UINT_PTR      m_timerSyncId;
    UINT_PTR      m_timerDisplayId;
    DWORD         m_nextSyncTick;

    ApiManager      *m_api;
    CourseConverter *m_conv;
};

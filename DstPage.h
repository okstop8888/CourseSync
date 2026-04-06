#pragma once
#include "resource.h"
#include "CourseData.h"
#include "ApiManager.h"

class DstPage : public CPropertyPage
{
    DECLARE_DYNAMIC(DstPage)
public:
    DstPage();

    PlusApiConfig getPlusApiConfig() const;
    void          applyPlusApiConfig(const PlusApiConfig &cfg);

    enum { IDD = IDD_PAGE_DST };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange *pDX);

    afx_msg void OnBtnTestPlus();
    DECLARE_MESSAGE_MAP()

private:
    // Only IP is entered by the user; URL/endpoint are assembled internally:
    // http://<ip>:7080/plusapi  +  /api/external/courseSyncPush
    CEdit           m_editPlusIp;
    CEdit           m_editPlusToken;
    CEdit           m_editPlusSemester;
    CDateTimeCtrl   m_dtPlusSemStart;   // date picker for semester start
    CButton         m_chkPlusClear, m_chkPlusEnabled;
    CStatic         m_lblPlusStatus;

    ApiManager *m_api;
};

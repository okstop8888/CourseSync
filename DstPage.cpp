#include "stdafx.h"
#include "DstPage.h"
#include "CourseSync.h"

IMPLEMENT_DYNAMIC(DstPage, CPropertyPage)

BEGIN_MESSAGE_MAP(DstPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BTN_TEST_PLUS, OnBtnTestPlus)
END_MESSAGE_MAP()

DstPage::DstPage() : CPropertyPage(IDD_PAGE_DST), m_api(nullptr) {}

void DstPage::DoDataExchange(CDataExchange *pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_PLUS_IP,       m_editPlusIp);
    DDX_Control(pDX, IDC_EDIT_PLUS_TOKEN,    m_editPlusToken);
    DDX_Control(pDX, IDC_EDIT_PLUS_SEMESTER, m_editPlusSemester);
    DDX_Control(pDX, IDC_EDIT_PLUS_SEMSTART, m_dtPlusSemStart);
    DDX_Control(pDX, IDC_CHK_PLUS_CLEAR,     m_chkPlusClear);
    DDX_Control(pDX, IDC_CHK_PLUS_ENABLED,   m_chkPlusEnabled);
    DDX_Control(pDX, IDC_STATIC_PLUS_STATUS, m_lblPlusStatus);
}

BOOL DstPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    m_api = &theApp.GetApiManager();

    // Display as "yyyy-M-d" to match the stored format
    m_dtPlusSemStart.SetFormat(_T("yyyy-M-d"));

    applyPlusApiConfig(theApp.m_cfgApi);
    return TRUE;
}

// Extract IP from a stored baseUrl like "http://192.168.90.8:7080/plusapi"
static string extractIpFromUrl(const string &baseUrl)
{
    string url = baseUrl;
    if (url.size() > 7 && url.substr(0, 7) == "http://")
        url = url.substr(7);
    else if (url.size() > 8 && url.substr(0, 8) == "https://")
        url = url.substr(8);

    size_t sep = url.find_first_of(":/");
    return (sep != string::npos) ? url.substr(0, sep) : url;
}

// Parse "YYYY-M-D" or "YYYY-MM-DD" into a SYSTEMTIME
static bool parseDateString(const string &s, SYSTEMTIME &st)
{
    memset(&st, 0, sizeof(st));
    int y = 0, m = 0, d = 0;
    if (sscanf_s(s.c_str(), "%d-%d-%d", &y, &m, &d) == 3 && y > 1900 && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
        st.wYear  = (WORD)y;
        st.wMonth = (WORD)m;
        st.wDay   = (WORD)d;
        return true;
    }
    return false;
}

// Format SYSTEMTIME as "YYYY-MM-DD" (zero-padded, required by backend SimpleDateFormat)
static string formatDateString(const SYSTEMTIME &st)
{
    char buf[32];
    sprintf_s(buf, "%04d-%02d-%02d", (int)st.wYear, (int)st.wMonth, (int)st.wDay);
    return buf;
}

PlusApiConfig DstPage::getPlusApiConfig() const
{
    PlusApiConfig cfg;
    CString s;
    m_editPlusIp.GetWindowText(s);
    string ip = CT2A(s);

    cfg.baseUrl  = ip.empty() ? "" : ("http://" + ip + ":7080/plusapi");
    cfg.endpoint = "/api/external/courseSyncPush";

    m_editPlusToken.GetWindowText(s);    cfg.token    = CT2A(s);
    m_editPlusSemester.GetWindowText(s); cfg.semester = CT2A(s);

    // Read date from DateTimePicker
    SYSTEMTIME st;
    memset(&st, 0, sizeof(st));
    if (m_dtPlusSemStart.GetTime(&st) == GDT_VALID)
        cfg.semesterStartDate = formatDateString(st);

    cfg.clearFirst = (m_chkPlusClear.GetCheck()   == BST_CHECKED);
    cfg.enabled    = (m_chkPlusEnabled.GetCheck() == BST_CHECKED);
    return cfg;
}

void DstPage::applyPlusApiConfig(const PlusApiConfig &cfg)
{
    string ip = extractIpFromUrl(cfg.baseUrl);
    m_editPlusIp.SetWindowText(CA2T(ip.c_str()));

    m_editPlusToken.SetWindowText(CA2T(cfg.token.c_str()));
    m_editPlusSemester.SetWindowText(CA2T(cfg.semester.c_str()));

    // Set date picker from stored "YYYY-M-D" string
    SYSTEMTIME st;
    if (!cfg.semesterStartDate.empty() && parseDateString(cfg.semesterStartDate, st)) {
        m_dtPlusSemStart.SetTime(&st);
    } else {
        // Default to today if nothing saved
        GetLocalTime(&st);
        m_dtPlusSemStart.SetTime(&st);
    }

    m_chkPlusClear.SetCheck(cfg.clearFirst ? BST_CHECKED : BST_UNCHECKED);
    m_chkPlusEnabled.SetCheck(cfg.enabled  ? BST_CHECKED : BST_UNCHECKED);
}

void DstPage::OnBtnTestPlus()
{
    PlusApiConfig cfg = getPlusApiConfig();
    if (cfg.baseUrl.empty()) {
        AfxMessageBox(_T("Server IP cannot be empty."));
        return;
    }
    if (cfg.token.empty()) {
        AfxMessageBox(_T("API Token cannot be empty."));
        return;
    }
    m_lblPlusStatus.SetWindowText(_T("Testing..."));
    string resp = m_api->testPlusApi(cfg);
    m_lblPlusStatus.SetWindowText(CA2T(resp.c_str()));
}

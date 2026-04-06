#include "stdafx.h"
#include "SyncPage.h"
#include "CourseSync.h"
#include <sstream>
#include <ctime>

IMPLEMENT_DYNAMIC(SyncPage, CPropertyPage)

BEGIN_MESSAGE_MAP(SyncPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BTN_SYNC_NOW,   OnBtnSyncNow)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG,  OnBtnClearLog)
    ON_BN_CLICKED(IDC_CHK_AUTO_SYNC,  OnChkAutoSync)
    ON_WM_TIMER()
END_MESSAGE_MAP()

SyncPage::SyncPage()
    : CPropertyPage(IDD_PAGE_SYNC)
    , m_timerSyncId(0), m_timerDisplayId(0)
    , m_nextSyncTick(0)
    , m_api(nullptr), m_conv(nullptr)
{}

void SyncPage::DoDataExchange(CDataExchange *pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHK_PUSH_PLUS,     m_chkPushPlus);
    DDX_Control(pDX, IDC_CHK_SKIP_CONFLICT, m_chkSkipConflict);
    DDX_Control(pDX, IDC_CHK_MATCH_BY_ID,   m_chkMatchById);
    DDX_Control(pDX, IDC_EDIT_SCHED_VALUE,  m_editSchedValue);
    DDX_Control(pDX, IDC_COMBO_SCHED_UNIT,  m_cmbSchedUnit);
    DDX_Control(pDX, IDC_CHK_AUTO_SYNC,     m_chkAutoSync);
    DDX_Control(pDX, IDC_PROGRESS,          m_progress);
    DDX_Control(pDX, IDC_EDIT_LOG,          m_editLog);
    DDX_Control(pDX, IDC_STATIC_NEXT_SYNC,  m_lblNextSync);
    DDX_Control(pDX, IDC_STATIC_SYNC_STATS, m_lblStats);
}

BOOL SyncPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    m_api  = &theApp.GetApiManager();
    m_conv = &theApp.GetConverter();

    m_cmbSchedUnit.AddString(_T("Minutes"));
    m_cmbSchedUnit.AddString(_T("Hours"));
    m_cmbSchedUnit.AddString(_T("Days"));
    m_cmbSchedUnit.SetCurSel(1);

    applySyncOptions(theApp.m_cfgSyncOpts);
    applyScheduleConfig(theApp.m_cfgSched);
    m_chkPushPlus.SetCheck(BST_CHECKED);

    m_progress.SetRange(0, 100);
    m_progress.SetPos(0);

    m_timerDisplayId = SetTimer(IDT_NEXT_SYNC_DISPLAY, 1000, nullptr);
    AppendLog(_T("CourseSync v2.0 ready."));

    return TRUE;
}

// ============================================================
// Log
// ============================================================
void SyncPage::AppendLog(const CString &msg)
{
    if (!IsWindow(m_hWnd)) return;
    CString cur;
    m_editLog.GetWindowText(cur);
    SYSTEMTIME st;
    GetLocalTime(&st);
    CString ts;
    ts.Format(_T("[%02d:%02d:%02d] "), st.wHour, st.wMinute, st.wSecond);
    if (!cur.IsEmpty()) cur += _T("\r\n");
    cur += ts + msg;
    if (cur.GetLength() > 8000) cur = cur.Right(8000);
    m_editLog.SetWindowText(cur);
    m_editLog.SetSel(cur.GetLength(), cur.GetLength());
    m_editLog.LineScroll(1);
}

void SyncPage::SetProgress(int val, int max)
{
    if (!IsWindow(m_hWnd)) return;
    m_progress.SetRange(0, max);
    m_progress.SetPos(val);
}

// ============================================================
// Sync options
// ============================================================
SyncOptions SyncPage::getSyncOptions() const
{
    SyncOptions opts;
    opts.skipOnConflict  = (m_chkSkipConflict.GetCheck() == BST_CHECKED);
    opts.matchByCourseId = (m_chkMatchById.GetCheck()    == BST_CHECKED);
    return opts;
}

ScheduleConfig SyncPage::getScheduleConfig() const
{
    ScheduleConfig cfg;
    cfg.enabled = (m_chkAutoSync.GetCheck() == BST_CHECKED);
    CString sv;
    m_editSchedValue.GetWindowText(sv);
    cfg.value = _ttoi(sv);
    if (cfg.value <= 0) cfg.value = 1;
    cfg.unitIdx = m_cmbSchedUnit.GetCurSel();
    if (cfg.unitIdx < 0) cfg.unitIdx = 1;
    return cfg;
}

void SyncPage::applySyncOptions(const SyncOptions &opts)
{
    m_chkSkipConflict.SetCheck(opts.skipOnConflict  ? BST_CHECKED : BST_UNCHECKED);
    m_chkMatchById.SetCheck(opts.matchByCourseId    ? BST_CHECKED : BST_UNCHECKED);
}

void SyncPage::applyScheduleConfig(const ScheduleConfig &cfg)
{
    m_chkAutoSync.SetCheck(cfg.enabled ? BST_CHECKED : BST_UNCHECKED);
    char buf[16]; sprintf_s(buf, "%d", cfg.value);
    m_editSchedValue.SetWindowText(CA2T(buf));
    int idx = (cfg.unitIdx >= 0 && cfg.unitIdx <= 2) ? cfg.unitIdx : 1;
    m_cmbSchedUnit.SetCurSel(idx);
    if (cfg.enabled) StartAutoTimer();
}

// ============================================================
// Timer
// ============================================================
void SyncPage::StartAutoTimer()
{
    if (m_timerSyncId) { KillTimer(m_timerSyncId); m_timerSyncId = 0; }

    ScheduleConfig cfg = getScheduleConfig();
    DWORD ms;
    if      (cfg.unitIdx == 0) ms = cfg.value * 60 * 1000;
    else if (cfg.unitIdx == 2) ms = cfg.value * 24 * 3600 * 1000;
    else                       ms = cfg.value * 3600 * 1000;
    if (ms < 60000) ms = 60000;

    m_timerSyncId  = SetTimer(IDT_SYNC_TIMER, ms, nullptr);
    m_nextSyncTick = (DWORD)(GetTickCount64() + ms);

    CString unitStr;
    m_cmbSchedUnit.GetWindowText(unitStr);
    CString logMsg;
    logMsg.Format(_T("Auto-sync scheduled: every %d %s."), cfg.value, (LPCTSTR)unitStr);
    AppendLog(logMsg);
    UpdateNextSyncLabel();
}

void SyncPage::StopAutoTimer()
{
    if (m_timerSyncId) { KillTimer(m_timerSyncId); m_timerSyncId = 0; }
    m_nextSyncTick = 0;
    m_lblNextSync.SetWindowText(_T("--"));
}

void SyncPage::UpdateNextSyncLabel()
{
    if (!m_nextSyncTick || !m_timerSyncId) { m_lblNextSync.SetWindowText(_T("--")); return; }
    ULONGLONG now  = GetTickCount64();
    ULONGLONG next = m_nextSyncTick;
    if (next <= now) { m_lblNextSync.SetWindowText(_T("soon")); return; }
    DWORD secLeft = (DWORD)((next - now) / 1000);
    CString s;
    if (secLeft >= 3600) s.Format(_T("%dh%02dm"), secLeft/3600, (secLeft%3600)/60);
    else if (secLeft >= 60) s.Format(_T("%dm%02ds"), secLeft/60, secLeft%60);
    else s.Format(_T("%ds"), secLeft);
    m_lblNextSync.SetWindowText(s);
}

void SyncPage::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == IDT_SYNC_TIMER) {
        ScheduleConfig cfg = getScheduleConfig();
        DWORD ms;
        if      (cfg.unitIdx == 0) ms = cfg.value * 60 * 1000;
        else if (cfg.unitIdx == 2) ms = cfg.value * 24 * 3600 * 1000;
        else                       ms = cfg.value * 3600 * 1000;
        m_nextSyncTick = (DWORD)(GetTickCount64() + ms);
        AppendLog(_T("Auto-sync triggered..."));
        DoFullSync();
    } else if (nIDEvent == IDT_NEXT_SYNC_DISPLAY) {
        UpdateNextSyncLabel();
    }
    CPropertyPage::OnTimer(nIDEvent);
}

// ============================================================
// Manual sync
// ============================================================
void SyncPage::OnBtnSyncNow()  { DoFullSync(); }
void SyncPage::OnBtnClearLog() { m_editLog.SetWindowText(_T("")); }

void SyncPage::OnChkAutoSync()
{
    if (m_chkAutoSync.GetCheck() == BST_CHECKED)
        StartAutoTimer();
    else
        StopAutoTimer();
}

// ============================================================
// Core sync logic — chunked processing to handle large tables
// Each chunk is read → converted → pushed → freed before the next.
// ============================================================
void SyncPage::DoFullSync()
{
    GetDlgItem(IDC_BTN_SYNC_NOW)->EnableWindow(FALSE);
    SetProgress(0);
    AppendLog(_T("===== Sync started ====="));

    DbManager &db = theApp.GetDbManager();

    // 1. Check source connection
    if (!db.isSourceConnected()) {
        AppendLog(_T("Source DB not connected. Aborted."));
        GetDlgItem(IDC_BTN_SYNC_NOW)->EnableWindow(TRUE);
        return;
    }

    // 2. Resolve table name and row count
    CString srcTable = theApp.GetSelectedTable();
    if (srcTable.IsEmpty()) {
        AppendLog(_T("No source table selected. Aborted."));
        GetDlgItem(IDC_BTN_SYNC_NOW)->EnableWindow(TRUE);
        return;
    }
    string tbl = CT2A(srcTable);

    CString logMsg;
    logMsg.Format(_T("Source table: %s"), (LPCTSTR)srcTable);
    AppendLog(logMsg);

    int totalRows = db.getSourceRowCount(tbl);
    if (totalRows < 0) totalRows = 0; // unknown, will still process
    logMsg.Format(_T("Total rows: %d"), totalRows);
    AppendLog(logMsg);
    SetProgress(5);

    // 3. Prepare converter and API config once
    FieldMap fm = theApp.GetMapPage().getFieldMap();
    m_conv->setFieldMap(fm);

    bool pushPlus = (m_chkPushPlus.GetCheck() == BST_CHECKED);
    PlusApiConfig apiCfg;
    bool apiReady = false;
    if (pushPlus) {
        apiCfg = theApp.GetDstPage().getPlusApiConfig();
        if (!apiCfg.enabled) {
            AppendLog(_T("PlusAPI push is disabled in configuration."));
            pushPlus = false;
        } else if (apiCfg.baseUrl.empty() || apiCfg.token.empty()) {
            AppendLog(_T("PlusAPI URL or Token not configured. Skipped."));
            pushPlus = false;
        } else {
            apiReady = true;
            AppendLog(_T("Pushing to PlusAPI..."));
        }
    }

    // 4. Chunked read → convert → push
    // Keep each chunk ≤ CHUNK_SIZE rows so memory stays bounded.
    const int CHUNK_SIZE = 1000;
    int offset = 0;
    int totalFetched   = 0;
    int totalConverted = 0;
    int totalPushed    = 0;
    bool firstChunk    = true;  // only the first chunk may send clearFirst=true
    bool anyError      = false;

    while (true) {
        // Build LIMIT/OFFSET suffix — source DB is NEVER modified
        char suffix[64];
        sprintf_s(suffix, "LIMIT %d OFFSET %d", CHUNK_SIZE, offset);

        auto rows = db.readSourceRows(tbl, suffix);
        if (rows.empty()) break;  // no more data

        int fetched = (int)rows.size();
        totalFetched += fetched;

        // Convert this chunk
        auto courses = m_conv->convert(rows);
        rows.clear(); // free raw rows immediately — source data no longer needed

        int converted = (int)courses.size();
        totalConverted += converted;

        // Update progress (5–95 range, weighted by rows processed)
        int pct = (totalRows > 0)
            ? 5 + (int)(90.0 * totalFetched / totalRows)
            : 50;
        SetProgress((std::min)(pct, 93));

        // Push this chunk
        if (pushPlus && apiReady && !courses.empty()) {
            string jsonArr = CourseConverter::toJsonArray(courses, apiCfg.semester);
            courses.clear(); // free converted records — JSON string now holds the data

            // Only the very first batch of the very first chunk clears existing data
            PlusApiConfig chunkCfg = apiCfg;
            if (!firstChunk) chunkCfg.clearFirst = false;

            PushResult pr = m_api->pushCourses(chunkCfg, jsonArr, nullptr);
            jsonArr.clear(); // free JSON string

            firstChunk = false;

            if (!pr.ok) {
                logMsg  = _T("PlusAPI push FAILED: ");
                logMsg += CA2T(pr.errorMsg.c_str());
                AppendLog(logMsg);
                anyError = true;
                break; // stop on push error
            }
            totalPushed += pr.pushedCount;
        } else {
            courses.clear();
            firstChunk = false;
        }

        offset += fetched;
        if (fetched < CHUNK_SIZE) break; // last chunk (fewer rows than requested)

        // Pump messages between chunks so the UI stays responsive
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto sync_done;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

sync_done:
    SetProgress(100);

    logMsg.Format(_T("Fetched %d rows, converted %d valid records."), totalFetched, totalConverted);
    AppendLog(logMsg);

    if (pushPlus) {
        if (!anyError) {
            logMsg.Format(_T("PlusAPI push OK: %d records pushed."), totalPushed);
            AppendLog(logMsg);
        }
    }

    CString stats;
    stats.Format(_T("src=%d  valid=%d  pushed=%d"),
                 totalFetched, totalConverted,
                 pushPlus ? totalPushed : 0);
    m_lblStats.SetWindowText(stats);
    AppendLog(_T("===== Sync complete ====="));

    GetDlgItem(IDC_BTN_SYNC_NOW)->EnableWindow(TRUE);
}

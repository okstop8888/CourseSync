#include "stdafx.h"
#include "SrcPage.h"
#include "CourseSync.h"

IMPLEMENT_DYNAMIC(SrcPage, CPropertyPage)

BEGIN_MESSAGE_MAP(SrcPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BTN_TEST_SRC,      OnBtnTestSrc)
    ON_BN_CLICKED(IDC_BTN_REFRESH_TABLE, OnBtnRefreshTable)
    ON_CBN_SELCHANGE(IDC_COMBO_SRC_DB,   OnCbnSelchangeDb)
    ON_CBN_SELCHANGE(IDC_COMBO_SRC_TABLE,OnCbnSelchangeTable)
END_MESSAGE_MAP()

SrcPage::SrcPage() : CPropertyPage(IDD_PAGE_SRC), m_db(nullptr) {}

void SrcPage::DoDataExchange(CDataExchange *pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_SRC_HOST,     m_editHost);
    DDX_Control(pDX, IDC_EDIT_SRC_PORT,     m_editPort);
    DDX_Control(pDX, IDC_EDIT_SRC_USER,     m_editUser);
    DDX_Control(pDX, IDC_EDIT_SRC_PASS,     m_editPass);
    DDX_Control(pDX, IDC_COMBO_SRC_DB,      m_cmbDb);
    DDX_Control(pDX, IDC_COMBO_SRC_TABLE,   m_cmbTable);
    DDX_Control(pDX, IDC_LIST_COL_PREVIEW,  m_listCols);
    DDX_Control(pDX, IDC_STATIC_SRC_STATUS, m_lblStatus);
}

BOOL SrcPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    m_db = &theApp.GetDbManager();

    m_listCols.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listCols.InsertColumn(0, _T("Column Name"),  LVCFMT_LEFT, 140);
    m_listCols.InsertColumn(1, _T("Data Type"),    LVCFMT_LEFT, 100);
    m_listCols.InsertColumn(2, _T("Sample Value"), LVCFMT_LEFT, 100);
    m_listCols.InsertColumn(3, _T("Mapped Field"), LVCFMT_LEFT, 120);

    applyConfig(theApp.m_cfgSrc);

    // Pre-fill DB combo with last saved DB name (no live connection yet)
    if (!theApp.m_cfgSrc.dbName.empty()) {
        CString dbName(CA2T(theApp.m_cfgSrc.dbName.c_str()));
        m_cmbDb.AddString(dbName);
        m_cmbDb.SetCurSel(0);
    }

    // Pre-fill table combo with last saved table name
    if (!theApp.m_cfgSrcTable.empty()) {
        CString tbl(CA2T(theApp.m_cfgSrcTable.c_str()));
        m_cmbTable.AddString(tbl);
        m_cmbTable.SetCurSel(0);
    }

    return TRUE;
}

DbConfig SrcPage::getConfig() const
{
    DbConfig cfg;
    CString s;
    m_editHost.GetWindowText(s); cfg.host = CT2A(s);
    m_editPort.GetWindowText(s); cfg.port = _ttoi(s); if (cfg.port <= 0) cfg.port = 3306;
    m_editUser.GetWindowText(s); cfg.user = CT2A(s);
    m_editPass.GetWindowText(s); cfg.pass = CT2A(s);
    m_cmbDb.GetWindowText(s);    cfg.dbName = CT2A(s);
    return cfg;
}

void SrcPage::applyConfig(const DbConfig &cfg)
{
    m_editHost.SetWindowText(CA2T(cfg.host.c_str()));
    char portBuf[16]; sprintf_s(portBuf, "%d", cfg.port);
    m_editPort.SetWindowText(CA2T(portBuf));
    m_editUser.SetWindowText(CA2T(cfg.user.c_str()));
    m_editPass.SetWindowText(CA2T(cfg.pass.c_str()));
    // DB combo populated after Test Connection
}

CString SrcPage::getSelectedTable() const
{
    CString s; m_cmbTable.GetWindowText(s); return s;
}

void SrcPage::setSelectedTable(const CString &table)
{
    if (!IsWindow(m_cmbTable.m_hWnd)) return;
    int idx = m_cmbTable.FindStringExact(-1, table);
    if (idx >= 0) m_cmbTable.SetCurSel(idx);
}

void SrcPage::setStatus(const CString &msg, COLORREF)
{
    m_lblStatus.SetWindowText(msg);
    m_lblStatus.InvalidateRect(nullptr);
}

// Step 1: connect without selecting a DB, then populate the DB combo
void SrcPage::OnBtnTestSrc()
{
    DbConfig cfg = getConfig();
    cfg.dbName = ""; // connect first without DB

    setStatus(_T("Connecting..."));
    if (!m_db->connectSource(cfg)) {
        CString err = CA2T(m_db->lastError().c_str());
        setStatus(_T("Error: ") + err);
        return;
    }

    setStatus(_T("Connected. Loading databases..."));

    vector<string> dbs = m_db->getDatabases();
    CString curSel;
    m_cmbDb.GetWindowText(curSel);
    m_cmbDb.ResetContent();
    for (auto &db : dbs)
        m_cmbDb.AddString(CA2T(db.c_str()));

    // Re-select previously chosen DB if it still exists
    if (!curSel.IsEmpty()) {
        int idx = m_cmbDb.FindStringExact(-1, curSel);
        if (idx >= 0) m_cmbDb.SetCurSel(idx);
    }

    setStatus(_T("Connected."));

    // If a DB is already selected, load its tables automatically
    if (m_cmbDb.GetCurSel() >= 0)
        OnCbnSelchangeDb();
}

// Step 2: user selects a DB → switch to it and populate table combo
void SrcPage::OnCbnSelchangeDb()
{
    CString dbName;
    m_cmbDb.GetWindowText(dbName);
    if (dbName.IsEmpty()) return;

    if (!m_db->isSourceConnected()) {
        setStatus(_T("Not connected. Please click Test Connection first."));
        return;
    }

    string db = CT2A(dbName);
    if (!m_db->selectDatabase(db)) {
        CString err = CA2T(m_db->lastError().c_str());
        setStatus(_T("Select DB error: ") + err);
        return;
    }

    vector<string> tables = m_db->getSourceTables();
    CString curTable;
    m_cmbTable.GetWindowText(curTable);
    m_cmbTable.ResetContent();
    for (auto &t : tables)
        m_cmbTable.AddString(CA2T(t.c_str()));

    if (!curTable.IsEmpty()) {
        int idx = m_cmbTable.FindStringExact(-1, curTable);
        m_cmbTable.SetCurSel(idx >= 0 ? idx : 0);
    } else if (m_cmbTable.GetCount() > 0) {
        m_cmbTable.SetCurSel(0);
    }

    OnCbnSelchangeTable();
}

void SrcPage::OnBtnRefreshTable()
{
    if (!m_db->isSourceConnected()) { OnBtnTestSrc(); return; }
    OnCbnSelchangeDb();
}

void SrcPage::OnCbnSelchangeTable()
{
    CString table;
    m_cmbTable.GetWindowText(table);
    if (!table.IsEmpty()) refreshColumnPreview(table);
}

void SrcPage::refreshColumnPreview(const CString &table)
{
    m_listCols.DeleteAllItems();
    if (!m_db->isSourceConnected()) return;

    string tbl = CT2A(table);
    auto sampleRows = m_db->readSourceRows(tbl, "LIMIT 1");
    vector<string> cols = m_db->getSourceColumns(tbl);

    map<string,string> sampleRow;
    if (!sampleRows.empty()) sampleRow = sampleRows[0];

    const FieldMap &fmap = theApp.GetConverter().fieldMap();

    for (size_t i = 0; i < cols.size(); ++i) {
        const string &col = cols[i];
        m_listCols.InsertItem((int)i, CA2T(col.c_str()));
        m_listCols.SetItemText((int)i, 1, _T("col"));

        auto it = sampleRow.find(col);
        CString val;
        if (it != sampleRow.end()) val = CA2T(it->second.substr(0, 30).c_str());
        m_listCols.SetItemText((int)i, 2, val);

        auto mi = fmap.find(col);
        CString mapped;
        if (mi != fmap.end()) mapped = CA2T(mi->second.c_str());
        m_listCols.SetItemText((int)i, 3, mapped);
    }
}

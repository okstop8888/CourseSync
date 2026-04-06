#include "stdafx.h"
#include "MapPage.h"
#include "CourseSync.h"

IMPLEMENT_DYNAMIC(MapPage, CPropertyPage)

BEGIN_MESSAGE_MAP(MapPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BTN_RESET_MAP,   OnBtnResetMap)
    ON_BN_CLICKED(IDC_BTN_PREVIEW_MAP, OnBtnPreviewMap)
    ON_BN_CLICKED(IDC_BTN_ADD_MAP,     OnBtnAddMap)
    ON_BN_CLICKED(IDC_BTN_DEL_MAP,     OnBtnDelMap)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_MAPPING, OnDblclkListMap)
END_MESSAGE_MAP()

MapPage::MapPage() : CPropertyPage(IDD_PAGE_MAP) {}

vector<string> MapPage::ctFields()
{
    return {
        "course_name", "teacher_name", "teacher_code", "teacher_id",
        "dayinweek", "start_section", "end_section",
        "week_mode", "start_week", "end_week",
        "assigned_weeks", "all_weeks",
        "room_id", "room_name", "course_id",
        "student_count", "org_name", "sjly",
        "auto_on", "auto_off", "adjust_status"
    };
}

void MapPage::DoDataExchange(CDataExchange *pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_MAPPING, m_listMap);
}

BOOL MapPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    m_listMap.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    // Source Column | Mapped Field (→ vDisk CC) | Sample Value
    m_listMap.InsertColumn(0, _T("Source Column"),       LVCFMT_LEFT, 150);
    m_listMap.InsertColumn(1, _T("Mapped Field (->vDisk CC)"), LVCFMT_LEFT, 170);
    m_listMap.InsertColumn(2, _T("Sample Value"),        LVCFMT_LEFT, 140);

    FieldMap fm = theApp.m_cfgFieldMap.empty()
                  ? CourseConverter::defaultFieldMap()
                  : theApp.m_cfgFieldMap;
    theApp.GetConverter().setFieldMap(fm);
    fillList(fm, m_srcCols);

    return TRUE;
}

void MapPage::fillList(const FieldMap &fm, const vector<string> &srcCols)
{
    m_listMap.DeleteAllItems();
    int row = 0;
    for (const string &col : srcCols) {
        m_listMap.InsertItem(row, CA2T(col.c_str()));
        auto it = fm.find(col);
        CString mapped;
        if (it != fm.end()) mapped = CA2T(it->second.c_str());
        m_listMap.SetItemText(row, 1, mapped);
        auto si = m_sampleRow.find(col);
        CString sample;
        if (si != m_sampleRow.end()) sample = CA2T(si->second.substr(0,30).c_str());
        m_listMap.SetItemText(row, 2, sample);
        ++row;
    }
    for (auto &kv : fm) {
        bool found = false;
        for (const string &col : srcCols) if (col == kv.first) { found = true; break; }
        if (!found) {
            m_listMap.InsertItem(row, CA2T(kv.first.c_str()));
            m_listMap.SetItemText(row, 1, CA2T(kv.second.c_str()));
            ++row;
        }
    }
}

void MapPage::refreshSourceColumns(const vector<string> &cols, const map<string,string> &sampleRow)
{
    m_srcCols   = cols;
    m_sampleRow = sampleRow;
    if (IsWindow(m_hWnd))
        fillList(theApp.GetConverter().fieldMap(), m_srcCols);
}

FieldMap MapPage::getFieldMap() const
{
    FieldMap fm;
    int n = m_listMap.GetItemCount();
    for (int i = 0; i < n; ++i) {
        CString src = m_listMap.GetItemText(i, 0);
        CString dst = m_listMap.GetItemText(i, 1);
        if (!src.IsEmpty() && !dst.IsEmpty()) {
            std::string sSrc = CT2A(src);
            std::string sDst = CT2A(dst);
            fm[sSrc] = sDst;
        }
    }
    return fm;
}

void MapPage::applyFieldMap(const FieldMap &m)
{
    theApp.GetConverter().setFieldMap(m);
    fillList(m, m_srcCols);
}

void MapPage::OnBtnResetMap()
{
    applyFieldMap(CourseConverter::defaultFieldMap());
}

void MapPage::OnBtnPreviewMap()
{
    if (!theApp.GetDbManager().isSourceConnected()) {
        AfxMessageBox(_T("Please connect to the source database first (Source DB tab)."));
        return;
    }
    CString table = theApp.GetSelectedTable();
    if (table.IsEmpty()) {
        AfxMessageBox(_T("Please select a source table first."));
        return;
    }

    std::string tbl = CT2A(table);
    auto rows = theApp.GetDbManager().readSourceRows(tbl, "LIMIT 3");
    FieldMap fm = getFieldMap();
    CourseConverter conv;
    conv.setFieldMap(fm);
    auto courses = conv.convert(rows);

    CString msg;
    msg.Format(_T("Source: %d rows  ->  Converted: %d CtCourse records\r\n\r\n"),
               (int)rows.size(), (int)courses.size());
    for (int i = 0; i < (int)(std::min)((size_t)3, courses.size()); ++i) {
        const CtCourse &c = courses[i];
        CString line;
        line.Format(_T("[%d] %s / %s / day=%d sec=%d-%d wk=%d-%d\r\n"),
                    i+1,
                    CA2T(c.course_name.c_str()),
                    CA2T(c.teacher_name.c_str()),
                    c.dayinweek, c.start_section, c.end_section,
                    c.start_week, c.end_week);
        msg += line;
    }
    AfxMessageBox(msg);
}

void MapPage::OnBtnAddMap()
{
    AfxMessageBox(_T("Double-click a row in the list to assign its target field."));
}

void MapPage::OnBtnDelMap()
{
    int sel = m_listMap.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) { AfxMessageBox(_T("Please select a row first.")); return; }
    m_listMap.DeleteItem(sel);
}

void MapPage::OnDblclkListMap(NMHDR *pNMHDR, LRESULT *pResult)
{
    editSelectedRow();
    *pResult = 0;
}

void MapPage::editSelectedRow()
{
    int sel = m_listMap.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) return;

    CMenu menu;
    menu.CreatePopupMenu();
    auto fields = ctFields();
    menu.AppendMenu(MF_STRING | MF_GRAYED, 0, _T("Select target field:"));
    menu.AppendMenu(MF_SEPARATOR);
    for (int i = 0; i < (int)fields.size(); ++i)
        menu.AppendMenu(MF_STRING, 1000 + i, CA2T(fields[i].c_str()));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, 999, _T("(clear mapping)"));

    CRect rc;
    m_listMap.GetSubItemRect(sel, 1, LVIR_BOUNDS, rc);
    m_listMap.ClientToScreen(&rc);

    int cmd = (int)menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, rc.left, rc.bottom, this, nullptr);
    if (cmd == 999) {
        m_listMap.SetItemText(sel, 1, _T(""));
    } else if (cmd >= 1000) {
        int idx = cmd - 1000;
        if (idx < (int)fields.size())
            m_listMap.SetItemText(sel, 1, CA2T(fields[idx].c_str()));
    }
}

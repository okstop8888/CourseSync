#include "stdafx.h"
#include "CourseSync.h"
#include "CCourseSyncSheet.h"
#include <fstream>
#include <sstream>

CCourseSyncApp theApp;

BEGIN_MESSAGE_MAP(CCourseSyncApp, CWinApp)
END_MESSAGE_MAP()

CCourseSyncApp::CCourseSyncApp() : m_pSheet(nullptr)
{}

BOOL CCourseSyncApp::InitInstance()
{
    CWinApp::InitInstance();

    HANDLE hMutex = CreateMutexA(nullptr, TRUE, "CourseSyncMutex_v2");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExist = FindWindow(nullptr, _T("CourseSync v2.0"));
        if (hExist) { ShowWindow(hExist, SW_SHOW); SetForegroundWindow(hExist); }
        CloseHandle(hMutex);
        return FALSE;
    }

    LoadSettings();

    m_pSheet = new CCourseSyncSheet(_T("CourseSync v2.0"));
    m_pSheet->AddPage(&m_srcPage);
    m_pSheet->AddPage(&m_dstPage);
    m_pSheet->AddPage(&m_mapPage);
    m_pSheet->AddPage(&m_syncPage);

    if (!m_pSheet->Create(nullptr, (DWORD)-1, WS_EX_APPWINDOW)) {
        AfxMessageBox(_T("PropertySheet Create failed."));
        return FALSE;
    }

    m_pSheet->CenterWindow();
    m_pMainWnd = m_pSheet;
    return TRUE;
}

int CCourseSyncApp::ExitInstance()
{
    SaveSettings();
    delete m_pSheet;
    m_pSheet = nullptr;
    return CWinApp::ExitInstance();
}

CString CCourseSyncApp::GetSelectedTable() const
{
    return m_srcPage.getSelectedTable();
}

// ============================================================
// JSON file path helper
// ============================================================
static string GetJsonPath()
{
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    string p = path;
    size_t pos = p.rfind('\\');
    if (pos != string::npos) p = p.substr(0, pos);
    return p + "\\CourseSync.json";
}

// ============================================================
// Minimal JSON helpers
// ============================================================
static string jsonEsc(const string &s)
{
    string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

// Extract the first string value for "key" from JSON text
static string jsonGetStr(const string &json, const string &key)
{
    string k = "\"" + key + "\":";
    size_t pos = json.find(k);
    if (pos == string::npos) return "";
    pos += k.size();
    while (pos < json.size() && (json[pos]==' '||json[pos]=='\t'||json[pos]=='\r'||json[pos]=='\n')) pos++;
    if (pos >= json.size() || json[pos] != '"') return "";
    pos++; // skip opening '"'
    string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            pos++;
            switch (json[pos]) {
            case '"':  result += '"';  break;
            case '\\': result += '\\'; break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            default:   result += json[pos]; break;
            }
        } else {
            result += json[pos];
        }
        pos++;
    }
    return result;
}

static int jsonGetInt(const string &json, const string &key, int def)
{
    string k = "\"" + key + "\":";
    size_t pos = json.find(k);
    if (pos == string::npos) return def;
    pos += k.size();
    while (pos < json.size() && (json[pos]==' '||json[pos]=='\t')) pos++;
    try { return std::stoi(json.substr(pos)); } catch (...) { return def; }
}

static bool jsonGetBool(const string &json, const string &key, bool def)
{
    string k = "\"" + key + "\":";
    size_t pos = json.find(k);
    if (pos == string::npos) return def;
    pos += k.size();
    while (pos < json.size() && (json[pos]==' '||json[pos]=='\t')) pos++;
    if (pos + 4 <= json.size() && json.substr(pos, 4) == "true")  return true;
    if (pos + 5 <= json.size() && json.substr(pos, 5) == "false") return false;
    return def;
}

// Extract the sub-object (or sub-array) string for the given key
static string jsonGetObject(const string &json, const string &key)
{
    string k = "\"" + key + "\":";
    size_t pos = json.find(k);
    if (pos == string::npos) return "";
    pos += k.size();
    while (pos < json.size() && (json[pos]==' '||json[pos]=='\t'||json[pos]=='\n'||json[pos]=='\r')) pos++;
    if (pos >= json.size() || json[pos] != '{') return "";

    // Find matching closing brace
    int depth = 0;
    size_t start = pos;
    while (pos < json.size()) {
        char c = json[pos];
        if (c == '{') depth++;
        else if (c == '}') { depth--; if (depth == 0) { pos++; break; } }
        else if (c == '"') { // skip string contents
            pos++;
            while (pos < json.size() && json[pos] != '"') {
                if (json[pos] == '\\') pos++;
                pos++;
            }
        }
        pos++;
    }
    return json.substr(start, pos - start);
}

// ============================================================
// Save settings to CourseSync.json
// ============================================================
void CCourseSyncApp::SaveSettings()
{
    std::ostringstream j;
    j << "{\n";

    // srcDb
    DbConfig src = m_srcPage.getConfig();
    j << "  \"srcDb\": {\n";
    j << "    \"host\": \""   << jsonEsc(src.host)   << "\",\n";
    j << "    \"port\": "     << src.port             << ",\n";
    j << "    \"db\": \""     << jsonEsc(src.dbName)  << "\",\n";
    j << "    \"user\": \""   << jsonEsc(src.user)    << "\",\n";
    j << "    \"pass\": \""   << jsonEsc(src.pass)    << "\",\n";
    {
        CT2A tbl(m_srcPage.getSelectedTable());
        j << "    \"table\": \"" << jsonEsc(string(tbl)) << "\"\n";
    }
    j << "  },\n";

    // plusApi
    PlusApiConfig api = m_dstPage.getPlusApiConfig();
    // Extract IP from baseUrl
    string ip;
    string url = api.baseUrl;
    if (url.size() > 7 && url.substr(0,7) == "http://") {
        string rest = url.substr(7);
        size_t sep = rest.find_first_of(":/");
        ip = (sep != string::npos) ? rest.substr(0, sep) : rest;
    }
    j << "  \"plusApi\": {\n";
    j << "    \"ip\": \""                << jsonEsc(ip)                      << "\",\n";
    j << "    \"token\": \""             << jsonEsc(api.token)               << "\",\n";
    j << "    \"semester\": \""          << jsonEsc(api.semester)            << "\",\n";
    j << "    \"semesterStartDate\": \"" << jsonEsc(api.semesterStartDate)   << "\",\n";
    j << "    \"clearFirst\": "          << (api.clearFirst ? "true" : "false") << ",\n";
    j << "    \"enabled\": "             << (api.enabled    ? "true" : "false") << "\n";
    j << "  },\n";

    // schedule
    ScheduleConfig sched = m_syncPage.getScheduleConfig();
    j << "  \"schedule\": {\n";
    j << "    \"enabled\": " << (sched.enabled ? "true" : "false") << ",\n";
    j << "    \"value\": "   << sched.value   << ",\n";
    j << "    \"unitIdx\": " << sched.unitIdx << "\n";
    j << "  },\n";

    // sync options
    SyncOptions opts = m_syncPage.getSyncOptions();
    j << "  \"sync\": {\n";
    j << "    \"skipOnConflict\": "  << (opts.skipOnConflict  ? "true" : "false") << ",\n";
    j << "    \"matchByCourseId\": " << (opts.matchByCourseId ? "true" : "false") << "\n";
    j << "  },\n";

    // fieldMap
    FieldMap fm = m_mapPage.getFieldMap();
    j << "  \"fieldMap\": {\n";
    bool firstEntry = true;
    for (auto &kv : fm) {
        if (!firstEntry) j << ",\n";
        j << "    \"" << jsonEsc(kv.first) << "\": \"" << jsonEsc(kv.second) << "\"";
        firstEntry = false;
    }
    j << "\n  }\n";

    j << "}\n";

    std::ofstream f(GetJsonPath());
    if (f.is_open())
        f << j.str();
}

// ============================================================
// Load settings from CourseSync.json (falls back to defaults)
// ============================================================
void CCourseSyncApp::LoadSettings()
{
    // Read entire JSON file into a string
    string json;
    {
        std::ifstream f(GetJsonPath());
        if (f.is_open()) {
            std::ostringstream buf;
            buf << f.rdbuf();
            json = buf.str();
        }
    }

    if (json.empty()) {
        // No settings file yet — apply compiled-in defaults
        m_cfgSrc    = DbConfig{};
        m_cfgApi    = PlusApiConfig{};
        m_cfgSched  = ScheduleConfig{};
        m_cfgSyncOpts = SyncOptions{};
        m_cfgFieldMap.clear();
        m_cfgSrcTable.clear();
        return;
    }

    // srcDb
    string srcObj = jsonGetObject(json, "srcDb");
    m_cfgSrc.host   = jsonGetStr(srcObj, "host");   if (m_cfgSrc.host.empty())   m_cfgSrc.host = "127.0.0.1";
    m_cfgSrc.port   = jsonGetInt(srcObj, "port", 3306);
    m_cfgSrc.dbName = jsonGetStr(srcObj, "db");
    m_cfgSrc.user   = jsonGetStr(srcObj, "user");   if (m_cfgSrc.user.empty())   m_cfgSrc.user = "root";
    m_cfgSrc.pass   = jsonGetStr(srcObj, "pass");
    m_cfgSrcTable   = jsonGetStr(srcObj, "table");

    // plusApi
    string apiObj = jsonGetObject(json, "plusApi");
    string ip = jsonGetStr(apiObj, "ip");
    m_cfgApi.baseUrl  = ip.empty() ? "" : ("http://" + ip + ":7080/plusapi");
    m_cfgApi.endpoint = "/api/external/courseSyncPush";
    m_cfgApi.token             = jsonGetStr(apiObj, "token");
    m_cfgApi.semester          = jsonGetStr(apiObj, "semester");
    m_cfgApi.semesterStartDate = jsonGetStr(apiObj, "semesterStartDate");
    m_cfgApi.clearFirst = jsonGetBool(apiObj, "clearFirst", false);
    m_cfgApi.enabled    = jsonGetBool(apiObj, "enabled",    true);

    // schedule
    string schedObj = jsonGetObject(json, "schedule");
    m_cfgSched.enabled = jsonGetBool(schedObj, "enabled", false);
    m_cfgSched.value   = jsonGetInt (schedObj, "value",   1);
    m_cfgSched.unitIdx = jsonGetInt (schedObj, "unitIdx", 1);
    if (m_cfgSched.unitIdx < 0 || m_cfgSched.unitIdx > 2) m_cfgSched.unitIdx = 1;

    // sync options
    string syncObj = jsonGetObject(json, "sync");
    m_cfgSyncOpts.skipOnConflict  = jsonGetBool(syncObj, "skipOnConflict",  false);
    m_cfgSyncOpts.matchByCourseId = jsonGetBool(syncObj, "matchByCourseId", true);

    // fieldMap (nested object — parse each string entry within it)
    m_cfgFieldMap.clear();
    string fmObj = jsonGetObject(json, "fieldMap");
    if (!fmObj.empty()) {
        // Iterate through "key":"value" pairs inside the object
        size_t pos = 0;
        while (pos < fmObj.size()) {
            // Find next opening quote for a key
            size_t ks = fmObj.find('"', pos);
            if (ks == string::npos) break;
            ks++;
            size_t ke = fmObj.find('"', ks);
            if (ke == string::npos) break;
            string k = fmObj.substr(ks, ke - ks);

            // Find ':' after key
            size_t colon = fmObj.find(':', ke + 1);
            if (colon == string::npos) break;

            // Find value string
            size_t vs = fmObj.find('"', colon + 1);
            if (vs == string::npos) break;
            vs++;
            // Read until closing quote (handle escapes)
            string v;
            while (vs < fmObj.size() && fmObj[vs] != '"') {
                if (fmObj[vs] == '\\' && vs + 1 < fmObj.size()) {
                    vs++;
                    switch (fmObj[vs]) {
                    case '"':  v += '"';  break;
                    case '\\': v += '\\'; break;
                    default:   v += fmObj[vs]; break;
                    }
                } else {
                    v += fmObj[vs];
                }
                vs++;
            }
            size_t ve = vs; // points to closing '"'

            if (!k.empty())
                m_cfgFieldMap[k] = v;

            pos = ve + 1;
        }
    }
}

#include "stdafx.h"
#include "ApiManager.h"
#include <sstream>

// ============================================================
// 轻量 JSON 字段提取（无需外部库）
// ============================================================
static int jsonExtractInt(const string &json, const string &key, int def)
{
    string k = "\"" + key + "\":";
    size_t pos = json.find(k);
    if (pos == string::npos) return def;
    pos += k.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    try { return std::stoi(json.substr(pos)); } catch (...) { return def; }
}

static bool jsonExtractBool(const string &json, const string &key, bool def)
{
    string k = "\"" + key + "\":";
    size_t pos = json.find(k);
    if (pos == string::npos) return def;
    pos += k.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos + 4 <= json.size() && json.substr(pos, 4) == "true")  return true;
    if (pos + 5 <= json.size() && json.substr(pos, 5) == "false") return false;
    return def;
}

ApiManager::ApiManager() : m_lastCode(0) {}
ApiManager::~ApiManager() {}

// ============================================================
// URL 解析：http(s)://host[:port]/path
// ============================================================
ApiManager::ParsedUrl ApiManager::parseUrl(const string &url)
{
    ParsedUrl p;
    string s = url;
    if (s.substr(0, 8) == "https://") { p.https = true; s = s.substr(8); p.port = 443; }
    else if (s.substr(0, 7) == "http://") { p.https = false; s = s.substr(7); p.port = 80; }

    // find host:port / path
    size_t slash = s.find('/');
    string hostport = (slash != string::npos) ? s.substr(0, slash) : s;
    p.path = (slash != string::npos) ? s.substr(slash) : "/";

    size_t colon = hostport.rfind(':');
    if (colon != string::npos) {
        p.host = hostport.substr(0, colon);
        try { p.port = std::stoi(hostport.substr(colon + 1)); } catch (...) {}
    } else {
        p.host = hostport;
    }
    return p;
}

// ============================================================
// 构造 Push 请求体
// ============================================================
string ApiManager::buildBody(const PlusApiConfig &cfg, const string &dataJsonArray, bool clearFirst)
{
    // 简单 JSON 拼接（数据已由 CourseConverter 序列化）
    auto esc = [](const string &s) -> string {
        string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else out += c;
        }
        return out;
    };

    std::ostringstream os;
    os << "{";
    if (!cfg.semester.empty())
        os << "\"semester\":\"" << esc(cfg.semester) << "\",";
    if (!cfg.semesterStartDate.empty())
        os << "\"semesterStartDate\":\"" << esc(cfg.semesterStartDate) << "\",";
    os << "\"clearFirst\":" << (clearFirst ? "true" : "false") << ",";
    os << "\"data\":" << dataJsonArray;
    os << "}";
    return os.str();
}

// ============================================================
// HTTP POST（WinHTTP 同步）
// ============================================================
string ApiManager::doPost(const PlusApiConfig &cfg, const string &body)
{
    m_lastCode  = 0;
    m_lastError = "";

    ParsedUrl pu = parseUrl(cfg.baseUrl);
    string fullPath = pu.path;
    // append endpoint
    string ep = cfg.endpoint;
    if (!ep.empty()) {
        if (fullPath == "/" && ep[0] == '/') fullPath = ep;
        else fullPath += ep;
    }
    if (fullPath.empty()) fullPath = "/";

    // Convert to wide strings
    int wlen = MultiByteToWideChar(CP_UTF8, 0, pu.host.c_str(), -1, nullptr, 0);
    vector<wchar_t> whost(wlen);
    MultiByteToWideChar(CP_UTF8, 0, pu.host.c_str(), -1, whost.data(), wlen);

    int wplen = MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, nullptr, 0);
    vector<wchar_t> wpath(wplen);
    MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, wpath.data(), wplen);

    string bearerHdr = "Authorization: Bearer " + cfg.token + "\r\n"
                       "Content-Type: application/json;charset=utf-8\r\n";
    int whlen = MultiByteToWideChar(CP_UTF8, 0, bearerHdr.c_str(), -1, nullptr, 0);
    vector<wchar_t> wheader(whlen);
    MultiByteToWideChar(CP_UTF8, 0, bearerHdr.c_str(), -1, wheader.data(), whlen);

    HINTERNET hSession = WinHttpOpen(L"CourseSync/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { m_lastError = "WinHttpOpen failed"; return ""; }

    HINTERNET hConnect = WinHttpConnect(hSession, whost.data(), (INTERNET_PORT)pu.port, 0);
    if (!hConnect) { m_lastError = "WinHttpConnect failed"; WinHttpCloseHandle(hSession); return ""; }

    DWORD flags = pu.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath.data(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        m_lastError = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return "";
    }

    // 忽略 SSL 证书错误（自签证书环境）
    if (pu.https) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    BOOL ok = WinHttpSendRequest(hRequest, wheader.data(), (DWORD)-1L,
                                 (LPVOID)body.c_str(), (DWORD)body.size(),
                                 (DWORD)body.size(), 0);
    if (!ok) {
        m_lastError = "WinHttpSendRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "";
    }

    ok = WinHttpReceiveResponse(hRequest, nullptr);
    if (!ok) {
        m_lastError = "WinHttpReceiveResponse failed";
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "";
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
    m_lastCode = (int)statusCode;

    string responseBody;
    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0) {
        vector<char> buf(bytesAvail + 1, 0);
        DWORD bytesRead = 0;
        WinHttpReadData(hRequest, buf.data(), bytesAvail, &bytesRead);
        responseBody.append(buf.data(), bytesRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return responseBody;
}

// ============================================================
// 测试连通性
// ============================================================
string ApiManager::testPlusApi(const PlusApiConfig &cfg)
{
    string testBody = buildBody(cfg, "[]", false);
    string resp = doPost(cfg, testBody);
    if (resp.empty() && !m_lastError.empty())
        return "Connection failed: " + m_lastError;
    char buf[32];
    sprintf_s(buf, "HTTP %d\n", m_lastCode);
    return string(buf) + resp.substr(0, 600);
}

// ============================================================
// 批量推送（每批 200 条，第一批 clearFirst，后续 false）
// ============================================================
PushResult ApiManager::pushCourses(const PlusApiConfig &cfg, const string &jsonArray,
                                   std::function<void(int,int)> onProgress)
{
    PushResult result;

    // 将 jsonArray 字符串拆成单条记录，按 BATCH_SIZE 分批
    // jsonArray 格式: [{...},{...},...]
    // 简单解析：按顶层 { } 切分
    vector<string> items;
    {
        int depth = 0;
        size_t start = string::npos;
        for (size_t i = 0; i < jsonArray.size(); ++i) {
            char c = jsonArray[i];
            if (c == '{') {
                if (depth == 0) start = i;
                ++depth;
            } else if (c == '}') {
                --depth;
                if (depth == 0 && start != string::npos) {
                    items.push_back(jsonArray.substr(start, i - start + 1));
                    start = string::npos;
                }
            }
        }
    }

    if (items.empty()) { result.ok = true; return result; }

    const int BATCH = 200;
    int total = (int)items.size();
    int sent  = 0;
    bool firstBatch = true;

    while (sent < total) {
        int end = (std::min)(sent + BATCH, total);
        string batchArr = "[";
        for (int i = sent; i < end; ++i) {
            if (i > sent) batchArr += ",";
            batchArr += items[i];
        }
        batchArr += "]";

        bool doClear = firstBatch && cfg.clearFirst;
        string body = buildBody(cfg, batchArr, doClear);
        string resp = doPost(cfg, body);
        firstBatch = false;

        if (resp.empty() && !m_lastError.empty()) {
            result.ok = false;
            result.errorMsg = m_lastError;
            return result;
        }
        if (m_lastCode < 200 || m_lastCode >= 300) {
            result.ok = false;
            result.httpCode = m_lastCode;
            result.errorMsg = "HTTP " + std::to_string(m_lastCode) + ": " + resp.substr(0, 300);
            result.response = result.errorMsg;
            return result;
        }

        // HTTP 200 received — but also verify the backend actually saved records.
        // courseSyncPush returns {"data":{"success":true/false,"saved":N,"failed":M,...}}
        // even on logical failure, so we must inspect the body.
        int serverFailed = jsonExtractInt(resp, "failed", 0);
        bool serverSuccess = jsonExtractBool(resp, "success", true);
        int serverSaved   = jsonExtractInt(resp, "saved", end - sent);

        if (!serverSuccess || serverFailed > 0) {
            result.ok = false;
            result.httpCode = m_lastCode;
            result.errorMsg = "HTTP " + std::to_string(m_lastCode)
                            + " but backend reported failure (failed=" + std::to_string(serverFailed)
                            + "): " + resp.substr(0, 400);
            result.response = result.errorMsg;
            return result;
        }

        sent = end;
        result.pushedCount += serverSaved;
        if (onProgress) onProgress(sent, total);
    }

    result.ok       = true;
    result.httpCode = m_lastCode;
    // pushedCount is accumulated per-batch above; keep it
    char buf[64];
    sprintf_s(buf, "Push complete, %d records", result.pushedCount);
    result.response = buf;
    return result;
}

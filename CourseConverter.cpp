#include "stdafx.h"
#include "CourseConverter.h"
#include <sstream>
#include <algorithm>
#include <cstring>

// ============================================================
// 默认字段映射
// ============================================================
FieldMap CourseConverter::defaultFieldMap()
{
    FieldMap m;
    m["course_name"]   = "course_name";
    m["courseName"]    = "course_name";
    m["KCMC"]          = "course_name";
    m["kc_mc"]         = "course_name";
    m["courseTitle"]   = "course_name";

    m["teacher_name"]  = "teacher_name";
    m["teacherName"]   = "teacher_name";
    m["JSXM"]          = "teacher_name";
    m["js_xm"]         = "teacher_name";

    m["teacher_id"]    = "teacher_id";
    m["teacherId"]     = "teacher_id";

    m["teacher_code"]  = "teacher_code";
    m["teacherCode"]   = "teacher_code";
    m["JSBH"]          = "teacher_code";
    m["js_bh"]         = "teacher_code";

    m["dayinweek"]     = "dayinweek";
    m["day_in_week"]   = "dayinweek";
    m["weekDay"]       = "dayinweek";
    m["XQSJ"]          = "dayinweek";
    m["xq"]            = "dayinweek";

    m["start_Section"] = "start_section";
    m["start_section"] = "start_section";
    m["startSection"]  = "start_section";
    m["KSJC"]          = "start_section";
    m["begin_jc"]      = "start_section";

    m["end_Section"]   = "end_section";
    m["end_section"]   = "end_section";
    m["endSection"]    = "end_section";
    m["JSJC"]          = "end_section";
    m["end_jc"]        = "end_section";

    m["week_mode"]     = "week_mode";
    m["weekMode"]      = "week_mode";
    m["DDSZ"]          = "week_mode";
    m["zc_type"]       = "week_mode";

    m["start_week"]    = "start_week";
    m["startWeek"]     = "start_week";
    m["KSZC"]          = "start_week";
    m["ks_zc"]         = "start_week";

    m["end_week"]      = "end_week";
    m["endWeek"]       = "end_week";
    m["JSZC"]          = "end_week";
    m["js_zc"]         = "end_week";

    m["assigned_weeks"]= "assigned_weeks";
    m["assignedWeeks"] = "assigned_weeks";
    m["all_weeks"]     = "all_weeks";

    m["room_id"]       = "room_id";
    m["roomId"]        = "room_id";
    m["room_name"]     = "room_name";
    m["roomName"]      = "room_name";
    m["JXLMC"]         = "room_name";
    m["classroomName"] = "room_name";

    m["course_id"]     = "course_id";
    m["courseId"]      = "course_id";
    m["KCBH"]          = "course_id";
    m["course_code"]   = "course_id";

    m["student_count"] = "student_count";
    m["studentCount"]  = "student_count";
    m["XKRS"]          = "student_count";

    m["org_name"]      = "org_name";
    m["orgName"]       = "org_name";
    m["XYMC"]          = "org_name";
    m["department"]    = "org_name";

    m["sjly"]          = "sjly";
    m["adjust_status"] = "adjust_status";
    m["auto_on"]       = "auto_on";
    m["auto_off"]      = "auto_off";
    return m;
}

CourseConverter::CourseConverter()
    : m_fieldMap(defaultFieldMap())
{}

// ============================================================
// 辅助
// ============================================================
vector<string> CourseConverter::keysFor(const string &dbField) const
{
    vector<string> keys;
    for (auto &kv : m_fieldMap)
        if (kv.second == dbField)
            keys.push_back(kv.first);
    if (std::find(keys.begin(), keys.end(), dbField) == keys.end())
        keys.push_back(dbField);
    return keys;
}

string CourseConverter::getStr(const map<string,string> &row, const string &dbField) const
{
    for (const string &k : keysFor(dbField)) {
        auto it = row.find(k);
        if (it != row.end() && !it->second.empty())
            return it->second;
    }
    return "";
}

int CourseConverter::getInt(const map<string,string> &row, const string &dbField) const
{
    string s = getStr(row, dbField);
    if (s.empty()) return 0;
    try { return std::stoi(s); } catch (...) { return 0; }
}

// ============================================================
// 转换
// ============================================================
CtCourse CourseConverter::convertOne(const map<string,string> &row)
{
    CtCourse c;
    c.course_name   = getStr(row, "course_name");
    c.teacher_name  = getStr(row, "teacher_name");
    c.teacher_code  = getStr(row, "teacher_code");
    c.teacher_id    = getInt(row, "teacher_id");
    c.dayinweek     = getInt(row, "dayinweek");
    c.start_section = getInt(row, "start_section");
    c.end_section   = getInt(row, "end_section");
    c.room_id       = getInt(row, "room_id");
    c.room_name     = getStr(row, "room_name");
    c.start_week    = getInt(row, "start_week");
    c.end_week      = getInt(row, "end_week");
    c.course_id     = getStr(row, "course_id");
    c.student_count = getStr(row, "student_count");
    c.org_name      = getStr(row, "org_name");
    c.sjly          = getStr(row, "sjly");
    c.auto_on       = getInt(row, "auto_on");
    c.auto_off      = getInt(row, "auto_off");

    string adj = getStr(row, "adjust_status");
    c.adjust_status = adj.empty() ? -1 : std::atoi(adj.c_str());

    c.week_mode = normalizeWeekMode(getStr(row, "week_mode"));

    string rawAw  = getStr(row, "assigned_weeks");
    string rawAll = getStr(row, "all_weeks");

    if (!rawAw.empty()) {
        vector<int> wl = parseWeeks(rawAw);
        if (c.week_mode.empty())
            c.week_mode = inferWeekMode(c.start_week, c.end_week, wl);
        if (!wl.empty()) {
            std::ostringstream os;
            for (int i = 0; i < (int)wl.size(); ++i) {
                if (i) os << ',';
                os << wl[i];
            }
            c.assigned_weeks = os.str();
        } else {
            c.assigned_weeks = rawAw;
        }
        c.all_weeks = rawAll.empty() ? calcAllWeeks(parseWeeks(c.assigned_weeks)) : rawAll;
    } else if (c.start_week > 0 && c.end_week > 0) {
        if (c.week_mode.empty()) c.week_mode = "\xe5\x85\xa8"; // 全 UTF-8
        c.assigned_weeks = calcAssignedWeeks(c.start_week, c.end_week, c.week_mode);
        c.all_weeks = calcAllWeeks(parseWeeks(c.assigned_weeks));
    }
    return c;
}

vector<CtCourse> CourseConverter::convert(const vector<map<string,string>> &rows)
{
    vector<CtCourse> result;
    for (auto &row : rows) {
        CtCourse c = convertOne(row);
        if (c.isValid()) result.push_back(c);
    }
    return result;
}

// ============================================================
// 序列化：CtCourse -> ImportSchedule JSON（推送 PlusAPI 用）
// 所有中文字符串使用 UTF-8 编码
// schedule 格式: "周三 第1-2节"
// weeks 格式:    "1-15" / "1-15单" / "1-15双"
// ============================================================
string CourseConverter::jsonEscape(const string &s)
{
    string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += (char)c;
        }
    }
    return out;
}

string CourseConverter::toJsonObject(const CtCourse &c, const string &semester)
{
    // 星期映射 UTF-8: 1=周一 ... 7=周日
    static const char* dayNames[] = {
        "",
        "\xe5\x91\xa8\xe4\xb8\x80",  // 周一
        "\xe5\x91\xa8\xe4\xba\x8c",  // 周二
        "\xe5\x91\xa8\xe4\xb8\x89",  // 周三
        "\xe5\x91\xa8\xe5\x9b\x9b",  // 周四
        "\xe5\x91\xa8\xe4\xba\x94",  // 周五
        "\xe5\x91\xa8\xe5\x85\xad",  // 周六
        "\xe5\x91\xa8\xe6\x97\xa5"   // 周日
    };

    // schedule: "周X 第Y-Z节" — all UTF-8
    // 第 UTF-8: E7 AC AC    节 UTF-8: E8 8A 82
    string schedule;
    if (c.dayinweek >= 1 && c.dayinweek <= 7)
        schedule = string(dayNames[c.dayinweek]) + " ";
    if (c.start_section > 0) {
        int endSec = (c.end_section >= c.start_section) ? c.end_section : c.start_section;
        string period;
        period += "\xe7\xac\xac"; // 第 UTF-8
        char nums[32];
        sprintf_s(nums, "%d-%d", c.start_section, endSec);
        period += nums;
        period += "\xe8\x8a\x82"; // 节 UTF-8
        schedule += period;
    }

    // weeks: "start-end" / "start-end单" / "start-end双" — UTF-8
    // 单 UTF-8: E5 8D 95    双 UTF-8: E5 8F 8C
    string weeks;
    if (c.start_week > 0 && c.end_week > 0) {
        char buf[32];
        sprintf_s(buf, "%d-%d", c.start_week, c.end_week);
        weeks = buf;
        if (c.week_mode.size() >= 3) {
            unsigned char b0 = (unsigned char)c.week_mode[0];
            unsigned char b1 = (unsigned char)c.week_mode[1];
            unsigned char b2 = (unsigned char)c.week_mode[2];
            if (b0==0xE5 && b1==0x8D && b2==0x95)       // 单 UTF-8
                weeks += "\xe5\x8d\x95";
            else if (b0==0xE5 && b1==0x8F && b2==0x8C)  // 双 UTF-8
                weeks += "\xe5\x8f\x8c";
        }
    } else if (!c.assigned_weeks.empty()) {
        weeks = c.assigned_weeks;
    }

    // Build JSON object
    std::ostringstream json;
    json << "{";
    json << "\"courseName\":\""   << jsonEscape(c.course_name) << "\"";
    json << ",\"courseCode\":\""  << jsonEscape(c.course_id)   << "\"";
    json << ",\"teacherName\":\"" << jsonEscape(c.teacher_name)<< "\"";
    if (!c.teacher_code.empty())
        json << ",\"teacherCode\":\"" << jsonEscape(c.teacher_code) << "\"";
    json << ",\"schedule\":\""    << jsonEscape(schedule)      << "\"";
    json << ",\"weeks\":\""       << jsonEscape(weeks)         << "\"";
    if (!c.room_name.empty())
        json << ",\"classroom\":\"" << jsonEscape(c.room_name) << "\"";
    if (c.start_section > 0) json << ",\"startPeriod\":"  << c.start_section;
    if (c.end_section   > 0) json << ",\"endPeriod\":"    << c.end_section;
    if (!c.org_name.empty())
        json << ",\"department\":\"" << jsonEscape(c.org_name) << "\"";
    if (!c.student_count.empty()) {
        try { json << ",\"classStudents\":" << std::stoi(c.student_count); }
        catch (...) {}
    }
    if (!semester.empty())
        json << ",\"semester\":\"" << jsonEscape(semester) << "\"";
    json << ",\"dataSource\":\"api\"";
    json << "}";
    return json.str();
}

string CourseConverter::toJsonArray(const vector<CtCourse> &courses, const string &semester)
{
    string arr = "[";
    bool first = true;
    for (const CtCourse &c : courses) {
        if (!first) arr += ",";
        arr += toJsonObject(c, semester);
        first = false;
    }
    arr += "]";
    return arr;
}

// ============================================================
// 周次计算
// ============================================================
string CourseConverter::normalizeWeekMode(const string &raw)
{
    if (raw.empty()) return "";

    // UTF-8 三字节检测（数据库 UTF-8 连接返回的中文）
    if (raw.size() >= 3) {
        unsigned char b0 = (unsigned char)raw[0];
        unsigned char b1 = (unsigned char)raw[1];
        unsigned char b2 = (unsigned char)raw[2];
        if (b0==0xE5 && b1==0x85 && b2==0xA8) return "\xe5\x85\xa8"; // 全
        if (b0==0xE5 && b1==0x8D && b2==0x95) return "\xe5\x8d\x95"; // 单
        if (b0==0xE5 && b1==0x8F && b2==0x8C) return "\xe5\x8f\x8c"; // 双
    }

    // GBK 两字节兼容（旧数据 / 非 UTF-8 连接）
    if (raw.size() == 2) {
        unsigned char b0 = (unsigned char)raw[0], b1 = (unsigned char)raw[1];
        if (b0==0xC8 && b1==0xAB) return "\xe5\x85\xa8"; // 全 GBK→UTF-8
        if (b0==0xB5 && b1==0xA5) return "\xe5\x8d\x95"; // 单 GBK→UTF-8
        if (b0==0xCB && b1==0xAB) return "\xe5\x8f\x8c"; // 双 GBK→UTF-8
    }

    // ASCII / 数字后备
    string low = raw;
    for (char &ch : low) ch = (char)tolower((unsigned char)ch);
    if (low=="0"||low=="all"||low=="full") return "\xe5\x85\xa8"; // 全
    if (low=="1"||low=="odd")             return "\xe5\x8d\x95"; // 单
    if (low=="2"||low=="even")            return "\xe5\x8f\x8c"; // 双
    return raw;
}

// 单周 UTF-8: E5 8D 95
static bool isOddMode(const string &m) {
    return m.size()==3
        && (unsigned char)m[0]==0xE5
        && (unsigned char)m[1]==0x8D
        && (unsigned char)m[2]==0x95;
}
// 双周 UTF-8: E5 8F 8C
static bool isEvenMode(const string &m) {
    return m.size()==3
        && (unsigned char)m[0]==0xE5
        && (unsigned char)m[1]==0x8F
        && (unsigned char)m[2]==0x8C;
}

string CourseConverter::calcAssignedWeeks(int sw, int ew, const string &mode)
{
    vector<int> weeks;
    for (int w = sw; w <= ew; ++w) {
        bool add = false;
        if      (isOddMode(mode))  add = (w % 2 != 0);
        else if (isEvenMode(mode)) add = (w % 2 == 0);
        else                       add = true;
        if (add) weeks.push_back(w);
    }
    std::ostringstream os;
    for (int i = 0; i < (int)weeks.size(); ++i) {
        if (i) os << ',';
        os << weeks[i];
    }
    return os.str();
}

string CourseConverter::calcAllWeeks(const vector<int> &weeks)
{
    if (weeks.empty()) return "";
    string s = "#";
    for (int w : weeks) { s += std::to_string(w); s += "#"; }
    return s;
}

vector<int> CourseConverter::parseWeeks(const string &weeksStr)
{
    vector<int> result;
    if (weeksStr.empty()) return result;
    string s = weeksStr;
    for (char &c : s) if (c=='#') c=',';
    std::istringstream ss(s);
    string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) continue;
        try {
            int v = std::stoi(token);
            if (v > 0 && std::find(result.begin(),result.end(),v)==result.end())
                result.push_back(v);
        } catch (...) {}
    }
    std::sort(result.begin(), result.end());
    return result;
}

string CourseConverter::inferWeekMode(int sw, int ew, const vector<int> &weeks)
{
    vector<int> all, odd, even;
    for (int w = sw; w <= ew; ++w) {
        all.push_back(w);
        if (w%2!=0) odd.push_back(w);
        else        even.push_back(w);
    }
    if (weeks == all)  return "\xe5\x85\xa8"; // 全 UTF-8
    if (weeks == odd)  return "\xe5\x8d\x95"; // 单 UTF-8
    if (weeks == even) return "\xe5\x8f\x8c"; // 双 UTF-8
    return "\xe5\x85\xa8";                    // 默认全
}

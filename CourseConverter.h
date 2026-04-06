#pragma once
#include "CourseData.h"

class CourseConverter
{
public:
    CourseConverter();

    void     setFieldMap(const FieldMap &m) { m_fieldMap = m; }
    FieldMap fieldMap() const               { return m_fieldMap; }

    // 来源表行（列名->值 map 的 vector）-> CtCourse 列表
    vector<CtCourse> convert(const vector<map<string,string>> &rows);
    CtCourse         convertOne(const map<string,string> &row);

    // CtCourse -> JSON 字符串（ImportSchedule 格式，供推送 PlusAPI）
    // 输出单条 JSON object 字符串（不含外层花括号外的逗号）
    static string toJsonObject(const CtCourse &c, const string &semester = "");

    // 整个列表 -> JSON 数组字符串
    static string toJsonArray(const vector<CtCourse> &courses, const string &semester = "");

    // 周次计算（static）
    static string        calcAssignedWeeks(int sw, int ew, const string &mode);
    static string        calcAllWeeks(const vector<int> &weeks);
    static vector<int>   parseWeeks(const string &weeksStr);
    static string        normalizeWeekMode(const string &raw);
    static string        inferWeekMode(int sw, int ew, const vector<int> &weeks);

    // 默认字段映射
    static FieldMap defaultFieldMap();

private:
    FieldMap m_fieldMap;

    string getStr(const map<string,string> &row, const string &dbField) const;
    int    getInt(const map<string,string> &row, const string &dbField) const;
    vector<string> keysFor(const string &dbField) const;

    // JSON 辅助：转义特殊字符
    static string jsonEscape(const string &s);
};

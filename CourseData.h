#pragma once
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

// -------------------------------------------------------
// 来源/目标数据库连接参数
// -------------------------------------------------------
struct DbConfig {
    string host     = "127.0.0.1";
    int    port     = 3306;
    string dbName;
    string user     = "root";
    string pass;
};

// -------------------------------------------------------
// ct_course 目标表数据结构（也是中间转换格式）
// -------------------------------------------------------
struct CtCourse {
    string course_name;
    string teacher_name;
    int    teacher_id    = 0;
    string teacher_code;
    int    dayinweek     = 0;   // 1=周一 ... 7=周日
    int    start_section = 0;
    int    end_section   = 0;
    string week_mode;           // 全 / 单 / 双
    int    start_week    = 0;
    int    end_week      = 0;
    string assigned_weeks;      // 逗号分隔，如 "1,3,5,7"
    int    room_id       = 0;
    string room_name;
    string all_weeks;           // #1#3#5# 格式
    string course_id;
    string student_count;
    string org_name;
    string sjly;
    int    auto_on       = 0;
    int    auto_off      = 0;
    int    adjust_status = -1;

    bool isValid() const {
        return !course_name.empty() && dayinweek > 0 && start_section > 0;
    }
};

// -------------------------------------------------------
// 字段映射：来源表列名 -> ct_course 字段名
// -------------------------------------------------------
typedef map<string, string> FieldMap;

// -------------------------------------------------------
// PlusAPI 推送目标配置
// -------------------------------------------------------
struct PlusApiConfig {
    string name;
    string baseUrl;           // 如 http://192.168.1.1:5666
    string endpoint       = "/api/external/courseSyncPush";
    string token;
    string semester;          // 学年学期，如 2024-2025-2
    string semesterStartDate; // 学期开始日期，如 2025-02-17
    bool   clearFirst     = true;
    bool   enabled        = true;
};

// -------------------------------------------------------
// 本地数据库同步选项
// -------------------------------------------------------
struct SyncOptions {
    string targetTable    = "ct_course";
    string operatorUser   = "CourseSync";
    bool   skipOnConflict = false;
    bool   matchByCourseId = true;
};

// -------------------------------------------------------
// 同步结果统计
// -------------------------------------------------------
struct SyncResult {
    int inserted   = 0;
    int updated    = 0;
    int skipped    = 0;
    int errors     = 0;
    bool pushOk    = false;
    int  pushCount = 0;
    string pushMsg;
};

// -------------------------------------------------------
// 定时配置
// -------------------------------------------------------
struct ScheduleConfig {
    bool   enabled  = false;
    int    value    = 1;
    int    unitIdx  = 1;    // 0=分钟, 1=小时, 2=天
};

// -------------------------------------------------------
// 推送结果
// -------------------------------------------------------
struct PushResult {
    bool   ok          = false;
    int    httpCode    = 0;
    int    pushedCount = 0;
    string response;
    string errorMsg;
};

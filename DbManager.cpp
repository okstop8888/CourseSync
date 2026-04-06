#include "stdafx.h"
#include "DbManager.h"
#include <sstream>

DbManager::DbManager() : m_src(nullptr), m_dst(nullptr) {}

DbManager::~DbManager()
{
    disconnectSource();
    disconnectTarget();
}

// ============================================================
// 来源库
// ============================================================
bool DbManager::connectSource(const DbConfig &cfg)
{
    disconnectSource();
    m_src = mysql_init(nullptr);
    if (!m_src) { m_lastError = "mysql_init failed"; return false; }

    unsigned int timeout = 10;
    mysql_options(m_src, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(m_src, MYSQL_SET_CHARSET_NAME, "utf8");

    // Pass nullptr as dbname when empty to connect without selecting a database
    const char* dbNamePtr = cfg.dbName.empty() ? nullptr : cfg.dbName.c_str();
    if (!mysql_real_connect(m_src, cfg.host.c_str(), cfg.user.c_str(),
                            cfg.pass.c_str(), dbNamePtr,
                            (unsigned int)cfg.port, nullptr, 0)) {
        m_lastError = mysql_error(m_src);
        mysql_close(m_src); m_src = nullptr;
        return false;
    }
    mysql_set_character_set(m_src, "utf8");
    return true;
}

vector<string> DbManager::getDatabases()
{
    vector<string> dbs;
    if (!m_src) return dbs;
    if (!execQuery(m_src, "SHOW DATABASES")) return dbs;
    MYSQL_RES *res = mysql_store_result(m_src);
    if (!res) return dbs;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr)
        if (row[0]) dbs.push_back(row[0]);
    mysql_free_result(res);
    return dbs;
}

bool DbManager::selectDatabase(const string &dbName)
{
    if (!m_src || dbName.empty()) return false;
    string sql = "USE `" + dbName + "`";
    return execQuery(m_src, sql);
}

void DbManager::disconnectSource()
{
    if (m_src) { mysql_close(m_src); m_src = nullptr; }
}

// ============================================================
// 目标库
// ============================================================
bool DbManager::connectTarget(const DbConfig &cfg)
{
    disconnectTarget();
    m_dst = mysql_init(nullptr);
    if (!m_dst) { m_lastError = "mysql_init failed"; return false; }

    unsigned int timeout = 10;
    mysql_options(m_dst, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(m_dst, MYSQL_SET_CHARSET_NAME, "utf8");

    if (!mysql_real_connect(m_dst, cfg.host.c_str(), cfg.user.c_str(),
                            cfg.pass.c_str(), cfg.dbName.c_str(),
                            (unsigned int)cfg.port, nullptr, 0)) {
        m_lastError = mysql_error(m_dst);
        mysql_close(m_dst); m_dst = nullptr;
        return false;
    }
    mysql_set_character_set(m_dst, "utf8");
    return true;
}

bool DbManager::connectTargetSameAsSource()
{
    // 不重新建连接，dst 指向同一个 MYSQL 句柄
    disconnectTarget();
    m_dst = m_src; // 共享句柄（注意析构时不要重复 close）
    return m_dst != nullptr;
}

void DbManager::disconnectTarget()
{
    // 如果 dst == src，不要 close（close 由 src 负责）
    if (m_dst && m_dst != m_src) { mysql_close(m_dst); }
    m_dst = nullptr;
}

// ============================================================
// 查询辅助
// ============================================================
bool DbManager::execQuery(MYSQL *db, const string &sql)
{
    if (mysql_query(db, sql.c_str()) != 0) {
        m_lastError = mysql_error(db);
        return false;
    }
    return true;
}

string DbManager::escapeStr(MYSQL *db, const string &s)
{
    vector<char> buf(s.size() * 2 + 1);
    mysql_real_escape_string(db, buf.data(), s.c_str(), (unsigned long)s.size());
    return string(buf.data());
}

// ============================================================
// 来源库：获取行数
// ============================================================
int DbManager::getSourceRowCount(const string &table)
{
    if (!m_src || table.empty()) return -1;
    string sql = "SELECT COUNT(*) FROM `" + table + "`";
    if (!execQuery(m_src, sql)) return -1;
    MYSQL_RES *res = mysql_store_result(m_src);
    if (!res) return -1;
    int count = -1;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) count = std::atoi(row[0]);
    mysql_free_result(res);
    return count;
}

// ============================================================
// 来源库：获取表列表
// ============================================================
vector<string> DbManager::getSourceTables()
{
    vector<string> tables;
    if (!m_src) return tables;
    if (!execQuery(m_src, "SHOW TABLES")) return tables;
    MYSQL_RES *res = mysql_store_result(m_src);
    if (!res) return tables;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr)
        if (row[0]) tables.push_back(row[0]);
    mysql_free_result(res);
    return tables;
}

// ============================================================
// 来源库：获取表的列名
// ============================================================
vector<string> DbManager::getSourceColumns(const string &table)
{
    vector<string> cols;
    if (!m_src || table.empty()) return cols;
    string sql = "SHOW COLUMNS FROM `" + table + "`";
    if (!execQuery(m_src, sql)) return cols;
    MYSQL_RES *res = mysql_store_result(m_src);
    if (!res) return cols;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr)
        if (row[0]) cols.push_back(row[0]);
    mysql_free_result(res);
    return cols;
}

// ============================================================
// 来源库：读取行数据
// ============================================================
vector<map<string,string>> DbManager::readSourceRows(const string &table, const string &suffix)
{
    vector<map<string,string>> rows;
    if (!m_src || table.empty()) return rows;

    string sql = "SELECT * FROM `" + table + "`";
    if (!suffix.empty()) sql += " " + suffix;

    if (!execQuery(m_src, sql)) return rows;
    MYSQL_RES *res = mysql_store_result(m_src);
    if (!res) return rows;

    int numFields = mysql_num_fields(res);
    MYSQL_FIELD *fields = mysql_fetch_fields(res);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        map<string,string> rowMap;
        unsigned long *lengths = mysql_fetch_lengths(res);
        for (int i = 0; i < numFields; ++i) {
            string key = fields[i].name ? fields[i].name : "";
            string val = (row[i] && lengths[i]) ? string(row[i], lengths[i]) : "";
            rowMap[key] = val;
        }
        rows.push_back(rowMap);
    }
    mysql_free_result(res);
    return rows;
}

// ============================================================
// 目标库：upsert 辅助
// ============================================================
map<string,int> DbManager::loadExistingCids(MYSQL *db, const string &table)
{
    map<string,int> cache;
    string sql = "SELECT cid, course_id, course_name, dayinweek, start_section, room_name FROM `" + table + "`";
    if (!execQuery(db, sql)) return cache;
    MYSQL_RES *res = mysql_store_result(db);
    if (!res) return cache;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        int cid = row[0] ? std::atoi(row[0]) : 0;
        // key = course_id 或 复合键
        string courseId   = row[1] ? row[1] : "";
        string courseName = row[2] ? row[2] : "";
        string day        = row[3] ? row[3] : "";
        string sec        = row[4] ? row[4] : "";
        string room       = row[5] ? row[5] : "";
        if (!courseId.empty()) cache[courseId] = cid;
        string compKey = courseName + "|" + day + "|" + sec + "|" + room;
        cache[compKey] = cid;
    }
    mysql_free_result(res);
    return cache;
}

int DbManager::findExistingCid(const map<string,int> &cache, const CtCourse &c, bool byCourseId)
{
    if (byCourseId && !c.course_id.empty()) {
        auto it = cache.find(c.course_id);
        if (it != cache.end()) return it->second;
    }
    string compKey = c.course_name + "|" + std::to_string(c.dayinweek)
                   + "|" + std::to_string(c.start_section) + "|" + c.room_name;
    auto it = cache.find(compKey);
    return (it != cache.end()) ? it->second : -1;
}

bool DbManager::insertCourse(MYSQL *db, const string &table, const CtCourse &c, const string &op)
{
    std::ostringstream sql;
    sql << "INSERT INTO `" << table << "` "
        << "(course_name,teacher_name,teacher_id,teacher_code,dayinweek,start_Section,end_Section,"
        << "week_mode,start_week,end_week,assigned_weeks,all_weeks,room_id,room_name,"
        << "course_id,student_count,org_name,sjly,auto_on,auto_off,create_user,update_user) VALUES ("
        << "'" << escapeStr(db, c.course_name)   << "',"
        << "'" << escapeStr(db, c.teacher_name)  << "',"
        << c.teacher_id << ","
        << "'" << escapeStr(db, c.teacher_code)  << "',"
        << c.dayinweek    << ","
        << c.start_section << ","
        << c.end_section   << ","
        << "'" << escapeStr(db, c.week_mode)      << "',"
        << c.start_week    << ","
        << c.end_week      << ","
        << "'" << escapeStr(db, c.assigned_weeks) << "',"
        << "'" << escapeStr(db, c.all_weeks)      << "',"
        << c.room_id << ","
        << "'" << escapeStr(db, c.room_name)      << "',"
        << "'" << escapeStr(db, c.course_id)      << "',"
        << "'" << escapeStr(db, c.student_count)  << "',"
        << "'" << escapeStr(db, c.org_name)       << "',"
        << "'" << escapeStr(db, c.sjly)           << "',"
        << c.auto_on  << ","
        << c.auto_off << ","
        << "'" << escapeStr(db, op) << "',"
        << "'" << escapeStr(db, op) << "')";
    return execQuery(db, sql.str());
}

bool DbManager::updateCourse(MYSQL *db, const string &table, int cid, const CtCourse &c, const string &op)
{
    std::ostringstream sql;
    sql << "UPDATE `" << table << "` SET "
        << "course_name='"   << escapeStr(db, c.course_name)   << "',"
        << "teacher_name='"  << escapeStr(db, c.teacher_name)  << "',"
        << "teacher_id="     << c.teacher_id   << ","
        << "teacher_code='"  << escapeStr(db, c.teacher_code)  << "',"
        << "dayinweek="      << c.dayinweek    << ","
        << "start_Section="  << c.start_section << ","
        << "end_Section="    << c.end_section   << ","
        << "week_mode='"     << escapeStr(db, c.week_mode)     << "',"
        << "start_week="     << c.start_week   << ","
        << "end_week="       << c.end_week     << ","
        << "assigned_weeks='"<< escapeStr(db, c.assigned_weeks)<< "',"
        << "all_weeks='"     << escapeStr(db, c.all_weeks)     << "',"
        << "room_id="        << c.room_id      << ","
        << "room_name='"     << escapeStr(db, c.room_name)     << "',"
        << "course_id='"     << escapeStr(db, c.course_id)     << "',"
        << "student_count='" << escapeStr(db, c.student_count) << "',"
        << "org_name='"      << escapeStr(db, c.org_name)      << "',"
        << "auto_on="        << c.auto_on  << ","
        << "auto_off="       << c.auto_off << ","
        << "update_user='"   << escapeStr(db, op) << "'"
        << " WHERE cid=" << cid;
    return execQuery(db, sql.str());
}

// ============================================================
// 目标库：同步（upsert）
// ============================================================
SyncResult DbManager::syncCourses(const vector<CtCourse> &courses, const SyncOptions &opts)
{
    SyncResult result;
    if (!m_dst) { m_lastError = "Target DB not connected"; return result; }

    map<string,int> cache = loadExistingCids(m_dst, opts.targetTable);

    for (const CtCourse &c : courses) {
        int cid = findExistingCid(cache, c, opts.matchByCourseId);
        if (cid > 0) {
            if (opts.skipOnConflict) { result.skipped++; continue; }
            if (updateCourse(m_dst, opts.targetTable, cid, c, opts.operatorUser))
                result.updated++;
            else {
                result.errors++;
                m_lastError = mysql_error(m_dst);
            }
        } else {
            if (insertCourse(m_dst, opts.targetTable, c, opts.operatorUser))
                result.inserted++;
            else {
                result.errors++;
                m_lastError = mysql_error(m_dst);
            }
        }
    }
    return result;
}

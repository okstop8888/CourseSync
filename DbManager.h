#pragma once
#include "CourseData.h"
#include <mysql.h>

class DbManager
{
public:
    DbManager();
    ~DbManager();

    // ---- 来源库 ----
    bool   connectSource(const DbConfig &cfg);
    void   disconnectSource();
    bool   isSourceConnected() const { return m_src != nullptr; }

    vector<string>             getDatabases();
    bool                       selectDatabase(const string &dbName);
    vector<string>             getSourceTables();
    vector<string>             getSourceColumns(const string &table);
    // Returns total row count; -1 on error
    int                        getSourceRowCount(const string &table);
    // suffix can be "LIMIT n OFFSET m" — reads in chunks to bound memory
    vector<map<string,string>> readSourceRows(const string &table, const string &suffix = "");

    // ---- 目标库 ----
    bool   connectTarget(const DbConfig &cfg);
    bool   connectTargetSameAsSource();
    void   disconnectTarget();
    bool   isTargetConnected() const { return m_dst != nullptr; }

    SyncResult syncCourses(const vector<CtCourse> &courses, const SyncOptions &opts);

    string lastError() const { return m_lastError; }

private:
    MYSQL *m_src;
    MYSQL *m_dst;
    string m_lastError;

    bool execQuery(MYSQL *db, const string &sql);
    string escapeStr(MYSQL *db, const string &s);

    // 目标库 upsert 辅助
    map<string, int> loadExistingCids(MYSQL *db, const string &table);
    bool insertCourse(MYSQL *db, const string &table, const CtCourse &c, const string &op);
    bool updateCourse(MYSQL *db, const string &table, int cid, const CtCourse &c, const string &op);
    int  findExistingCid(const map<string,int> &cache, const CtCourse &c, bool byCourseId);
};

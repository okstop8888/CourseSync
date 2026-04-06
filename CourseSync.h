#pragma once
#include "DbManager.h"    // still needed for SrcPage / sync logic
#include "ApiManager.h"
#include "CourseConverter.h"
#include "SrcPage.h"
#include "DstPage.h"
#include "MapPage.h"
#include "SyncPage.h"

class CCourseSyncSheet;

class CCourseSyncApp : public CWinApp
{
public:
    CCourseSyncApp();

    // 全局对象访问器
    DbManager       &GetDbManager()  { return m_db;   }
    ApiManager      &GetApiManager() { return m_api;  }
    CourseConverter &GetConverter()  { return m_conv; }
    SrcPage         &GetSrcPage()    { return m_srcPage; }
    DstPage         &GetDstPage()    { return m_dstPage; }
    MapPage         &GetMapPage()    { return m_mapPage; }
    SyncPage        &GetSyncPage()   { return m_syncPage; }

    CString GetSelectedTable() const;

    void SaveSettings();
    void LoadSettings();

    virtual BOOL InitInstance();
    virtual int  ExitInstance();
    DECLARE_MESSAGE_MAP()

private:
    DbManager       m_db;
    ApiManager      m_api;
    CourseConverter m_conv;
    SrcPage         m_srcPage;
    DstPage         m_dstPage;
    MapPage         m_mapPage;
    SyncPage        m_syncPage;

    CCourseSyncSheet *m_pSheet;

public:
    // ---- Config cache (LoadSettings → each Page's OnInitDialog applies to controls) ----
    DbConfig       m_cfgSrc;
    PlusApiConfig  m_cfgApi;
    SyncOptions    m_cfgSyncOpts;
    ScheduleConfig m_cfgSched;
    FieldMap       m_cfgFieldMap;
    string         m_cfgSrcTable;
};

extern CCourseSyncApp theApp;

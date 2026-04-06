#pragma once

// ---- 图标 ----
#define IDI_APP                 100
#define IDI_TRAY                101

// ---- 对话框模板（CPropertyPage 页面） ----
#define IDD_PAGE_SRC            201
#define IDD_PAGE_DST            202
#define IDD_PAGE_MAP            203
#define IDD_PAGE_SYNC           204

// ---- Tab1 来源数据库 ----
#define IDC_EDIT_SRC_HOST       1001
#define IDC_EDIT_SRC_PORT       1002
// IDC_EDIT_SRC_DB (1003) removed — replaced by combo below
#define IDC_EDIT_SRC_USER       1004
#define IDC_EDIT_SRC_PASS       1005
#define IDC_BTN_TEST_SRC        1006
#define IDC_STATIC_SRC_STATUS   1007
#define IDC_COMBO_SRC_TABLE     1008
#define IDC_BTN_REFRESH_TABLE   1009
#define IDC_LIST_COL_PREVIEW    1010
#define IDC_COMBO_SRC_DB        1011   // database selector (populated after Test Connection)

// ---- Tab2 目标 & PlusAPI ----
#define IDC_CHK_SAME_DB         2001
#define IDC_EDIT_DST_HOST       2002
#define IDC_EDIT_DST_PORT       2003
#define IDC_EDIT_DST_DB         2004
#define IDC_EDIT_DST_USER       2005
#define IDC_EDIT_DST_PASS       2006
#define IDC_BTN_TEST_DST        2007
#define IDC_STATIC_DST_STATUS   2008
// IDC_EDIT_PLUS_URL (2009) removed — replaced by IP field below
// IDC_EDIT_PLUS_ENDPOINT (2010) removed — hardcoded internally
#define IDC_EDIT_PLUS_TOKEN     2011
#define IDC_EDIT_PLUS_SEMESTER  2012
#define IDC_EDIT_PLUS_SEMSTART  2013
#define IDC_CHK_PLUS_CLEAR      2014
#define IDC_CHK_PLUS_ENABLED    2015
#define IDC_BTN_TEST_PLUS       2016
#define IDC_STATIC_PLUS_STATUS  2017
#define IDC_EDIT_PLUS_IP        2018   // server IP (e.g. 192.168.90.8); port/path hardcoded

// ---- Tab3 字段映射 ----
#define IDC_LIST_MAPPING        3001
#define IDC_BTN_RESET_MAP       3002
#define IDC_BTN_PREVIEW_MAP     3003
#define IDC_BTN_ADD_MAP         3004
#define IDC_BTN_DEL_MAP         3005

// ---- Tab4 同步 & 定时 ----
#define IDC_CHK_WRITE_DB        4001
#define IDC_CHK_PUSH_PLUS       4002
#define IDC_CHK_SKIP_CONFLICT   4003
#define IDC_CHK_MATCH_BY_ID     4004
#define IDC_EDIT_SCHED_VALUE    4005
#define IDC_COMBO_SCHED_UNIT    4006
#define IDC_CHK_AUTO_SYNC       4007
#define IDC_BTN_SYNC_NOW        4008
#define IDC_PROGRESS            4009
#define IDC_EDIT_LOG            4010
#define IDC_STATIC_NEXT_SYNC    4011
#define IDC_BTN_CLEAR_LOG       4012
#define IDC_STATIC_SYNC_STATS   4013

// ---- 定时器 ----
#define IDT_SYNC_TIMER          9001
#define IDT_NEXT_SYNC_DISPLAY   9002

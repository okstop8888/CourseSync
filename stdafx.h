#pragma once

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#define WINVER       0x0601
#define _WIN32_IE    0x0600   // required for CDateTimeCtrl and other IE 6+ controls

#include <afxwin.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxdtctl.h>         // CDateTimeCtrl
#include <afxmt.h>
#include <afxdisp.h>

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <functional>

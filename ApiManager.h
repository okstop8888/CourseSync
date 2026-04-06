#pragma once
#include "CourseData.h"
#include <winhttp.h>
#include <functional>

class ApiManager
{
public:
    ApiManager();
    ~ApiManager();

    // 测试连通性，返回响应文本摘要
    string testPlusApi(const PlusApiConfig &cfg);

    // 批量推送课表到 /api/external/courseSyncPush
    // jsonArray: CourseConverter::toJsonArray() 返回的 JSON 数组字符串
    // onProgress(sent, total): 进度回调（可为 nullptr）
    PushResult pushCourses(const PlusApiConfig &cfg, const string &jsonArray,
                           std::function<void(int,int)> onProgress = nullptr);

    int    lastHttpCode() const { return m_lastCode; }
    string lastError()    const { return m_lastError; }

private:
    int    m_lastCode;
    string m_lastError;

    struct ParsedUrl {
        bool  https = false;
        string host;
        int    port = 80;
        string path;
    };
    ParsedUrl parseUrl(const string &url);

    // 发一次 POST，返回响应体；失败返回空字符串
    string doPost(const PlusApiConfig &cfg, const string &body);

    // 构造请求体：{"semester":"...","clearFirst":bool,"data":[...]}
    string buildBody(const PlusApiConfig &cfg, const string &dataJsonArray, bool clearFirst);
};

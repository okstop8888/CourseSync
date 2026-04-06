# vDisk云桌面 · 课表对接规范

> **用语**：本文档用 **vDisk云桌面** 指课表数据的接收与管理端；实现与客户端配置中仍常见 **PlusAPI**、URL 路径中的 `plusapi` 等历史命名，含义与本文「云桌面」侧一致。  
> **用途**：描述上述系统课表模块的数据结构与对外接口规范。  
> AI 读取本文档后，可直接实现任意来源系统向本系统推送课表数据的对接程序，无需再查代码。

---

## 一、系统背景

**vDisk云桌面**（教室智能管理 / 集控侧）使用课表数据用于：
- 判断当前教室是否正在上课（根据当前时间、星期、周次匹配）
- 向终端设备下发课表展示
- 统计教室使用率

课表数据存储在 `import_schedule` 表，每条记录对应"一门课程、一个教室、一段时间段、一段周次范围"。

---

## 二、认证方式

所有外部对接接口使用 **API Token** 认证，有两种传法（选其一）：

```
方式1（推荐）：HTTP 请求头
Authorization: Bearer <your_api_token>

方式2：URL 参数
?apiToken=<your_api_token>
```

Token 在 vDisk云桌面 管理后台 → API Token 管理 中创建和查询。

---

## 三、核心接口：批量推送课表

### 接口信息

```
URL:    POST /api/external/courseSyncPush
认证:   Bearer Token（见第二节）
格式:   Content-Type: application/json
```

### 请求体结构

```json
{
  "semester":          "2024-2025-2",
  "semesterStartDate": "2025-02-17",
  "clearFirst":        true,
  "data": [
    { ...课表记录1... },
    { ...课表记录2... }
  ]
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `semester` | string | 建议填 | 学年学期，格式 `YYYY-YYYY-N`，如 `2024-2025-2`。不填则每条记录需自带 semester 字段 |
| `semesterStartDate` | string | 建议填 | 学期第一天，格式 `YYYY-MM-DD`，如 `2025-02-17`。用于系统推算日期对应第几周 |
| `clearFirst` | boolean | 否 | `true`=推送前先删除 `semester` 对应的全部旧数据（**全量同步**推荐 true）；`false`=追加 |
| `data` | array | 是 | 课表记录数组，最多建议每批 500 条，可多次推送 |

### data 数组中每条记录的字段

#### 必填字段

| 字段 | 类型 | 说明 | 示例 |
|------|------|------|------|
| `courseName` | string | 课程名称 | `"Python程序设计"` |
| `schedule` | string | 上课时间段（星期+节次） | `"周三 第1-2节"` |
| `weeks` | string | 周次范围（见第四节详细格式） | `"1-15"` / `"1-15单"` / `"1-15双"` |
| `classroom` | string | 上课教室名称（须与系统中的教室名匹配） | `"7F706"` |

#### 强烈建议填写

| 字段 | 类型 | 说明 | 示例 |
|------|------|------|------|
| `courseCode` | string | 课程编号/代码 | `"CS101"` |
| `teacherName` | string | 授课教师姓名 | `"张三"` |
| `teacherCode` | string | 教师工号（职工号） | `"T10086"` |
| `semester` | string | 学年学期（可覆盖外层 semester） | `"2024-2025-2"` |
| `startPeriod` | int | 开始节次（数字） | `1` |
| `endPeriod` | int | 结束节次（数字） | `2` |

#### 可选字段

| 字段 | 类型 | 说明 | 示例 |
|------|------|------|------|
| `department` | string | 开课院系/部门 | `"计算机与人工智能学院"` |
| `classStudents` | int | 选课人数/实际人数 | `45` |
| `classCapacity` | int | 教学班容量 | `60` |
| `campus` | string | 校区 | `"主校区"` |
| `major` | string | 专业 | `"计算机科学与技术"` |
| `grade` | string | 年级 | `"2022级"` |
| `teachingClass` | string | 教学班名称 | `"计算机2301"` |
| `courseNature` | string | 课程性质 | `"必修"` |
| `courseType` | string | 课程类型 | `"专业课"` |
| `courseAttribute` | string | 课程属性 | `"理论+实践"` |

### 响应体

```json
{
  "status": 0,
  "content": {
    "success": true,
    "saved":   120,
    "failed":  0,
    "total":   120,
    "message": "推送完成：成功120条，失败0条"
  }
}
```

`status=0` 表示接口调用成功；`success=true` 表示数据全部入库。

---

## 四、关键字段格式规范

### 4.1 schedule 字段（上课时间段）

**格式：`周X 第Y-Z节`**

```
周一 第1-2节    ← 周一第1、2节
周三 第3-4节    ← 周三第3、4节
周五 第9-10节   ← 周五第9、10节
```

规则：
- 星期用**中文+数字序号**，写法：`周一` `周二` `周三` `周四` `周五` `周六` `周日`
- 节次**必须写范围格式 `Y-Z`**，即使只有一节也写成 `第1-1节`（不能只写 `第1节`）
- 空格隔开星期和节次

错误写法（不识别）：
```
星期三第1-2节   ← 没有空格
周三 第1节      ← 单节不写范围，系统无法判断当前是否上课
周三1-2        ← 缺少"第"和"节"
```

### 4.2 weeks 字段（周次范围）

支持三种格式：

**格式1：连续全部周**
```
"1-16"          ← 第1到16周，每周都有
"3-15"          ← 第3到15周
```

**格式2：连续单周（奇数周）**
```
"1-15单"        ← 第1~15周中的奇数周（第1、3、5...15周）
"2-14单"        ← 第2~14周中的奇数周（第3、5...13周）
```

**格式3：连续双周（偶数周）**
```
"2-16双"        ← 第2~16周中的偶数周（第2、4、6...16周）
"1-15双"        ← 第1~15周中的偶数周（第2、4...14周）
```

**格式4：离散周次列表**
```
"1,3,5,7,9,11,13,15"   ← 指定具体哪些周，逗号分隔
```

系统判断"当前是否有课"时，会拿当前日期算出第几周，再与 weeks 匹配。

### 4.3 semester 字段（学年学期）

格式：`YYYY-YYYY-N`
- 第一个 YYYY：起始学年（秋季学期所在年）
- 第二个 YYYY：结束学年
- N：1=上学期（秋季），2=下学期（春季）

```
"2024-2025-1"   ← 2024-2025学年第一学期（秋季，2024年9月开始）
"2024-2025-2"   ← 2024-2025学年第二学期（春季，2025年2月开始）
```

---

## 五、完整请求示例

```http
POST /api/external/courseSyncPush
Authorization: Bearer abc123def456
Content-Type: application/json

{
  "semester": "2024-2025-2",
  "semesterStartDate": "2025-02-17",
  "clearFirst": true,
  "data": [
    {
      "courseName":    "Python程序设计",
      "courseCode":    "CS101",
      "teacherName":   "张三",
      "teacherCode":   "T10086",
      "schedule":      "周三 第1-2节",
      "weeks":         "1-15",
      "classroom":     "7F706",
      "startPeriod":   1,
      "endPeriod":     2,
      "department":    "计算机与人工智能学院",
      "classStudents": 45,
      "campus":        "主校区",
      "semester":      "2024-2025-2"
    },
    {
      "courseName":    "数据结构",
      "courseCode":    "CS201",
      "teacherName":   "李四",
      "teacherCode":   "T10087",
      "schedule":      "周一 第3-4节",
      "weeks":         "1-15单",
      "classroom":     "A101",
      "startPeriod":   3,
      "endPeriod":     4,
      "department":    "计算机与人工智能学院",
      "classStudents": 60,
      "campus":        "主校区",
      "semester":      "2024-2025-2"
    }
  ]
}
```

成功响应：
```json
{
  "status": 0,
  "content": {
    "success": true,
    "saved": 2,
    "failed": 0,
    "total": 2,
    "message": "推送完成：成功2条，失败0条"
  }
}
```

---

## 六、数据查询接口（只读，供调试）

以下接口需要系统登录 Token（非 API Token），仅供后台调试。  
外部对接只需使用第三节的推送接口即可。

```
POST /api/schedule/list          查询课表列表（支持分页过滤）
POST /api/schedule/classroom     查询某教室的全部课表
POST /api/schedule/current       查询当前正在上课的教室列表
```

---

## 七、对接工作流程（给 AI 的步骤参考）

如果你需要对接一个新的课表来源系统（如教务系统 API、数据库、Excel 等），按以下步骤实现：

```
第一步：读取来源数据
  - 从来源系统获取课表原始数据（数据库 SQL / HTTP API / 文件）
  - 每条原始记录包含：课程名、教师、上课星期、节次、周次、教室等信息

第二步：字段转换
  - courseName  ← 来源的课程名称字段
  - teacherName ← 来源的教师姓名字段
  - teacherCode ← 来源的教师工号字段
  - schedule    ← 根据星期数字(1-7)和节次范围构造 "周X 第Y-Z节"
                  星期映射：1→周一，2→周二，3→周三，4→周四，5→周五，6→周六，7→周日
  - weeks       ← 根据起始周、结束周、单双周标志构造周次字符串
                  全部周：  "{startWeek}-{endWeek}"          如 "1-15"
                  单周：    "{startWeek}-{endWeek}单"        如 "1-15单"
                  双周：    "{startWeek}-{endWeek}双"        如 "2-14双"
                  离散周：  逗号分隔具体周次               如 "1,3,5,7"
  - classroom   ← 来源的教室名称（注意大小写需与 vDisk云桌面 中教室名一致）
  - semester    ← 学年学期，格式 "YYYY-YYYY-N"
  - startPeriod ← 开始节次数字
  - endPeriod   ← 结束节次数字
  - department  ← 来源的院系/部门字段（可选）
  - classStudents ← 来源的学生人数字段（可选）

第三步：批量推送
  - URL: POST http://<服务器IP>:<端口>/api/external/courseSyncPush
  - Header: Authorization: Bearer <api_token>
  - Body: 见第五节示例
  - 建议每批不超过 500 条，数据量大时分批推送
  - clearFirst=true 时第一批推送，后续批次改为 false（避免覆盖已推送数据）

第四步：验证
  - 检查响应 success=true
  - 登录 vDisk云桌面 管理后台 → 课表管理 中查看导入结果
```

---

## 八、注意事项

1. **教室名称匹配**：`classroom` 字段值须与 vDisk云桌面 中已有教室名称**完全一致**（大小写、全半角），否则系统会创建一条新教室记录，可能导致与已有教室重复。

2. **全量同步**：如果来源数据经常变动（如每天重新导出），推荐每次推送时 `clearFirst=true` + 提供 `semester`，系统会先清除旧数据再写入，保证数据与来源一致。

3. **增量同步**：如果只推送新增或修改的记录，用 `clearFirst=false`，系统追加写入。但注意目前没有基于 courseCode 的 upsert 逻辑，重复推送同一条记录会产生重复数据。

4. **学期建立**：首次推送某个 `semester` 时，务必同时提供 `semesterStartDate`，系统会据此自动建立学期记录并计算当前第几周。不提供则系统无法判断当前周次。

5. **单节课节次格式**：`schedule` 字段节次部分必须是范围格式 `第Y-Z节`，单节课写 `第1-1节` 而非 `第1节`，否则系统判断"当前是否上课"时的正则匹配失败，导致该课程无法被识别为当前进行中。

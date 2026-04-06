@echo off
chcp 936 >nul
setlocal

echo ============================================
echo  CourseSync MFC Build Script
echo ============================================

:: ---- 配置 VS2013 环境 ----
set "VS_COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\"
if exist "%VS_COMNTOOLS%vsvars32.bat" (
    call "%VS_COMNTOOLS%vsvars32.bat" >nul 2>&1
    echo [INFO] VS2013 环境加载成功
) else (
    echo [WARN] 未找到 VS2013，尝试通过 PATH 使用 msbuild...
)

:: ---- 检查本地 MySQL/MariaDB 依赖 ----
if not exist "lib\mysql\libmariadb.lib" (
    echo [ERROR] 未找到 lib\mysql\libmariadb.lib
    echo         请确认 lib\mysql\ 目录完整（应包含 mysql.h 和 libmariadb.lib/dll）
    pause
    exit /b 1
)
echo [INFO] MySQL/MariaDB Connector 已就绪 (lib\mysql\)

:: ---- 构建 ----
set "CONFIG=Release"
if /i "%1"=="debug" set "CONFIG=Debug"

echo [INFO] 正在编译 %CONFIG% 配置...
msbuild CourseSync.vcxproj /p:Configuration=%CONFIG% /p:Platform=Win32 /nologo /m
if errorlevel 1 (
    echo [ERROR] 编译失败！
    pause
    exit /b 1
)

:: ---- 输出目录 ----
set "OUTDIR=build\%CONFIG%"
echo [INFO] 编译成功，输出目录: %OUTDIR%

:: ---- 复制 libmariadb.dll ----
copy /y "lib\mysql\libmariadb.dll" "%OUTDIR%\" >nul
echo [INFO] 已复制 libmariadb.dll

:: ---- 复制 MFC DLL（如果需要在无 VS 的机器运行） ----
echo.
echo [INFO] 构建完成: %OUTDIR%\CourseSync.exe
echo [INFO] 运行前确保目标机器安装了:
echo         - Visual C++ 2013 Redistributable (x86)
echo         - libmariadb.dll (已复制到输出目录，无需额外安装 MySQL Connector)
echo.

pause

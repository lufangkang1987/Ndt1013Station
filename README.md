# NDT1013 Qt Quick 工作站

Qt 6.5+ / C++17 / Windows x64 / MSVC。当前为可编译的工程基础版，不是完整需求书的全部功能交付。

## 构建

### Qt Creator / qmake

可直接打开 `Ndt1013Station.pro`，选择 Qt 6.5+ 的 Windows x64 MSVC Kit，使用独立的影子构建目录。界面、源码及 SDK 接口与 CMake 工程共用；构建完成后程序位于构建目录 `bin/Ndt1013Station.exe`，SDK 和设备工作目录自动复制到其旁边。Qt Creator 运行时使用所选 Kit 的 Qt 环境。

也可继续打开 `CMakeLists.txt`。两种工程请使用不同构建目录，不要混用缓存。

### CMake

本机 Qt：`D:/Qt/6.11.1/msvc2022_64`。在本目录执行：

本机已验证的构建入口为 **build-msvc.cmd**（VS 2026 MSVC + Ninja）；编译后双击 **run-local.cmd** 启动，默认模拟模式。该启动脚本使用本机 Qt 环境，不是独立分发包。以下 VS 生成器命令作为其他已配置环境的参考；本机 VS 生成器未能识别编译器，已通过 vcvars64 + Ninja 完成编译。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/msvc2022_64
cmake --build build --config Release
& D:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe --qmldir qml build/Release/Ndt1013Station.exe
& ./build/Release/Ndt1013Station.exe
```

若本机安装 Visual Studio 2026，将生成器改为 `Visual Studio 18 2026`。SDK 已包含在项目的 `third_party/iray`，qmake 和 CMake 均使用项目内文件，无需原始发行包目录。移动项目时保留整个 `third_party` 目录；Qt 和 MSVC 仍需在目标电脑安装并配置 Kit。本机快捷脚本中的 Qt/MSVC 安装路径也需按目标电脑调整。

当前中文源码路径触发 Qt qmlimportscanner 路径识别问题，工程针对共享 Qt 禁用构建时导入扫描，通过本机启动脚本提供 QML 模块路径。独立部署需确保 QtQuick、Controls、Layouts 及其依赖模块齐全，并单独验证；当前不宣称已完成安装包部署。

SDK 文档对照见 `docs/SDK适配说明.md`。

## 已接入

- QML 主界面、默认模拟模式、16 位模拟图像、原始精度 RAW 和 JSON 保存（用户文档目录/Ndt1013Station）。
- QLibrary 加载 FpdSys.dll；Create/Destroy、Invoke、GetAttr，连接及任务回调。
- 图像回调深拷贝、队列投递到界面、有界单帧预览邮箱、日志长度限制。
- 连接异步完成确认、30 秒命令超时保护、通信失败提示。
- 构建时从项目内 `third_party/iray` 复制 x64 运行文件及 NDT1013LA 工作配置，保留厂商原始文件。文件来源和 SHA-256 清单见该目录的 README 与 manifest.json。

## 实机使用

1. 需要时手动安装项目内 `third_party/iray/redist/vcredist_x64_vs2013.exe`。Qt 程序还需要匹配的 MSVC 运行环境。
2. 先使用 iDetector 验证探测器、固件、型号后缀、触发方式、IP 和校正模板。
3. 本程序默认模拟模式。关闭模拟开关，选择已验证的设备工作目录，再连接。真实校正模板须来自对应设备，发行包配置不代表已校正。
4. 当前实机采集按钮仅发送 `Cmd_ClearAcq`，参照 SDK 单张示例入口，但不自动改写触发方式或 UserRAM。必须确认适用流程后使用，不能将按钮提交等同于曝光完成。本程序不控制射线源。
5. 日志显示原始错误码。连接失败或超时后使用断开/重置重新建立会话。

## 验证

### 打开已保存的 RAW

先断开设备，点击“打开 RAW 图像…”，选择 `.raw` 文件。程序自动读取同目录、同名 `.json`；必须包含整数 `width`、`height`、`bytesPerPixel`（1 或 2）、`byteOrder`（little-endian 或 big-endian）、布尔值 `simulation`。支持单帧、无文件头、无行填充的 8/16 位无符号灰度，导入后保留像素精度，预览仍采用 8 位显示。

JSON 缺失、格式错误、尺寸超限或 RAW 字节数不匹配时，日志说明原因，原有图像保持不变。外部 RAW 必须提供上述元数据，不自动猜测尺寸或来源。导入的非模拟标识仅来自文件声明，不代表已经验证设备身份或检测有效性。

导入测试：`python tests/test_raw_import.py build-qmake/bin/Ndt1013Station.exe`，覆盖 8/16 位、两种字节序、奇数宽度及损坏/缺失元数据等情况。

`Ndt1013Station.exe --smoke-test` 启动 QML，连接模拟设备并生成一帧，约 1.2 秒后退出；非零退出码表示失败。真实设备流程必须单独实机测试。

## 当前限制与后续

当前 SDK 调用在 GUI 线程提交，若厂家接口阻塞会影响界面；完整采集版应迁移到专用工作线程并加入关闭超时策略。SDK Destroy 必须保证回调终止，其返回值与异常销毁场景需要实机验证。当前单设备全局回调路由只用于基础项目。

预览将高位灰度映射到 8 位，不包含窗宽窗位功能。保存的是最近显示帧对应的 SDK 输出数据，不保证是校正前传感器原始数据；尚未持久记录完整设备参数与模板状态。RAW 与 JSON 分别原子写入，非跨文件事务，元数据失败会提示归档不完整。切勿作为正式生产归档机制。

待开发：能力查询和参数范围、持续采集及独立写盘队列、帧丢弃统计、校正向导、窗宽窗位、任务数据库、测量标注、权限审计和报告。模拟图像不用于缺陷评定。实机采集、校正、性能、断线恢复均尚待验证。

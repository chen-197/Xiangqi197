# Xiangqi（中国象棋 with Qt6）

一个基于 **Qt 6 Widgets** 的中国象棋小程序：支持 **PVP（人人对战）/ PVE（人机）/ EVE（机机）**。

- PVE/EVE 使用 **chessdb.cn 中国象棋云库（Cloudbook）REST API** 进行走法查询（联网）。
- UI 支持 **高 DPI** 与 **窗口缩放自适应**（棋盘坐标、落点吸附、棋子尺寸随窗口变化而重新计算）。

---

## 功能特性

- **PVP**：本地双人对战（鼠标拖拽/点选走子）。
- **PVE**：与云库对弈（云库给出候选着法/自动出步）。
- **EVE**：双方都由云库走子（演示/观战）。
- **难度选择**：界面下拉框提供 `简单 / 中等 / 困难 / 地狱`（不同难度会影响自动出步策略）。
- **悔棋**：支持撤销上一手（含还原被吃子）。
- **存档/读档**：保存为 `.can` 文件，便于复盘；存档目录默认 `./Saved/`。
- **回放**：可选择回放速度（秒/步）。
- **云库棋规裁定（可选）**：对局过程中可启用 `queryrule`，将最近若干步历史作为 `movelist` 发送给云库做棋规裁定，并将裁定出的 `ban` 结果回传到后续查询中。

---

## 运行与构建

### 依赖

- **Qt 6.x**（需模块：`Qt Widgets`、`Qt Network`）
- C++ 编译器（建议支持 C++17）：
  - Windows：MSVC 或 MinGW
  - Linux：g++/clang++

本项目使用 **qmake**（`Xiangqi.pro`），可在 Qt Creator 或命令行编译。

### Qt Creator 构建

1. 安装 Qt 6（包含 Widgets/Network）。
2. 使用 Qt Creator 打开 `Xiangqi.pro`。
3. 选择一个 Kit：
   - Windows：`Desktop Qt 6.x MSVC...` 或 `Desktop Qt 6.x MinGW...`
   - Linux：`Desktop Qt 6.x GCC...`
4. Build & Run。

### 命令行构建

#### Linux（示例：Ubuntu / Debian）

安装依赖（包名会随发行版略有变化）：

```bash
sudo apt update
sudo apt install -y qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools build-essential
```

构建：

```bash
cd Xiangqi197-main
qmake6 Xiangqi.pro
make -j
./Xiangqi
```

#### Windows（Qt 自带命令行环境 / Developer Prompt）

- **MSVC**：打开 “x64 Native Tools Command Prompt for VS”，并确保 Qt 的 `bin` 在 PATH 中。
- **MinGW**：使用 Qt 安装目录下的 “Qt 6.x for Desktop (MinGW)”。

构建：

```bat
cd Xiangqi197-main
qmake6 Xiangqi.pro
nmake   REM MSVC
REM 或者 mingw32-make  REM MinGW
```

生成的可执行文件在构建目录中（Qt Creator 默认是 `build-...` 目录）。

---

## 云库（chessdb.cn）说明

PVE/EVE 模式会联网访问云库 REST API（例如 `queryall/query/querybest/queryrule` 等）。

- 官方接口文档：
  - https://www.chessdb.cn/cloudbook_api.html

### 云库设置

菜单：**“云库设置”**（`Cloudbook Settings`）可配置并持久化保存（`QSettings`）：

- **启用棋规裁定（queryrule）**：
  - 开启后，在向云库请求走法前会先根据最近历史着法构造 `board + movelist`，调用 `queryrule`，得到 `ban/draw/none`。
- **历史步数（movelist）**：
  - 发送给 `queryrule` 的最近历史着法步数，范围 `4~200`（云库要求至少 4 步）。
- **reptimes（1~10）**：
  - 从第几次循环开始裁定。
- **避免和棋着法**：
  - 若开启，会将 `rule:draw` 也视为“禁止”，从而尽量规避和棋循环（注意这会改变对局风格）。

配置保存位置（Qt 默认行为）：

- Windows：注册表 `HKEY_CURRENT_USER\Software\Xiangqi197\Xiangqi`
- Linux：`~/.config/Xiangqi197/Xiangqi.conf`（或等价路径）

### 禁着与和棋规避

- 云库 `queryrule` 返回格式类似：
  - `move:c3c4,rule:none|move:b2e2,rule:ban|...`
- 本程序会把 `rule:ban`（以及你开启“避免和棋”时的 `rule:draw`）提取出来，拼成：
  - `ban=move:xxxx|move:zzzz|...`
- 后续查询（如 `queryall`）会携带该 `ban` 参数，让云库不要返回这些着法。

---

## 界面自适应与缩放

### 启动默认窗口大小（全局变量）

在 `mainwindow.cpp` 顶部有全局配置：

```cpp
// Edit these two numbers to change the default window size at startup.
static int g_uiStartWidth  = 1299;
static int g_uiStartHeight = 796;

// Keep aspect ratio when scaling the whole UI.
static bool g_uiKeepAspect = true;
```

- 你只需要改 `g_uiStartWidth / g_uiStartHeight` 就能改变启动大小。
- `g_uiKeepAspect=true` 表示等比缩放；若你希望“横向拉伸时棋盘也跟着拉伸”，可改为 `false`。

### 窗口缩放时会发生什么

- 所有控件的基准几何（在 `.ui` 设计时的大小）会被记录为 “Base Geometry”。
- 运行中当窗口大小变化，会按比例缩放控件位置与尺寸。
- **棋盘交点坐标**（`qipanCoordinates`）、**棋子初始坐标**（`qiziCoordinate`）、以及“鼠标松开时的落点吸附”都使用缩放后的几何重新计算。

如果你遇到“棋盘被放大得离谱/只放得下一个棋子”，通常是缩放基准尺寸取值异常（首帧尺寸未稳定）。当前版本已通过延迟首帧布局与更稳健的设计稿尺寸推导避免该问题。

### 高 DPI（HiDPI）

`main.cpp` 中启用了 Qt6 的高 DPI 相关设置：

- `QGuiApplication::setHighDpiScaleFactorRoundingPolicy(PassThrough)`
- `Qt::AA_UseHighDpiPixmaps`

这能改善 Windows 125%/150% 缩放下的显示质量。

---

## 存档格式（.can）

`.can` 是一个纯文本文件：

- 第 1 行：轮到哪方（`Chu` / `Han` / `none`）
- 后续每一行：一步着法字符串

着法字符串格式（程序内部使用）：

- `move:<from><to><cap>`
- `<from>` 与 `<to>` 为 4 字符坐标（类似 `a0b0`），列为 `a~i`，行 `0~9`（与云库/UCCI 常见坐标一致）。
- `<cap>` 表示被吃的棋子：
  - `NU`：没有吃子
  - `00~31`：被吃棋子的内部编号（两位数字）

示例：

- `move:h2e2NU`
- `move:c3c407`

> 注意：这是项目内部格式，并非通用 PGN/CCB 格式。如果你计划和其他工具互通，建议新增导入/导出模块。

---

## 代码结构（快速导航）

- `main.cpp`：程序入口、翻译加载、高 DPI 设置。
- `mainwindow.ui`：Qt Designer 布局（控件与菜单）。
- `mainwindow.h/.cpp`：绝大多数逻辑（UI + 规则 + 走法生成 + 网络请求 + 存档回放）。
- `XiangqiImg.qrc` + `Img/`：棋盘与棋子图片资源。
- `Xiangqi_zh_CN.ts`：翻译文件。

---

## 常见问题（FAQ）

### 1) PVE/EVE 没反应 / 一直显示“unknown / invalid board”

- 确认网络可访问 `www.chessdb.cn`。
- 云库接口依赖 FEN 与走法坐标的正确性；若你改动过走法编码或 FEN 生成，请优先检查 `getFEN()` 与云库请求 URL。

### 2) 存档加载后走子异常

- `.can` 文件若被手工编辑、缺行或乱码，会导致解析失败。
- 当前版本对“行长度/前缀”做了基本校验，但并不会做完全容错。

### 3) Windows 下棋子图片缩放模糊

- 这是位图资源在缩放时常见的现象。
- 若你追求更清晰显示，建议换成更高分辨率的 PNG，或改用矢量资源（SVG）。

---

## 许可证

本项目使用 **GNU GPL v3.0**，详见 `LICENSE.txt`。

---
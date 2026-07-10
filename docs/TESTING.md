# 测试指南

## 测试范围

项目默认使用 `data/curved/` 曲线路网。原始数据包含 104 个道路节点、161 条原始边、768 个原始曲线控制点和 6 个 POI；加载 POI、合并重复物理边并清理几何数据后，程序运行时包含 110 个节点、166 条逻辑边和 777 个有效几何渲染点。

旧版 `data/map.txt` 仅作为兼容格式和手工测试数据保留，不是当前默认地图。

CTest 注册以下 9 个自动化测试：

- `graph`
- `dijkstra`
- `astar`
- `storage`
- `scu_map`
- `curved_campus`
- `input`
- `edge_cases`
- `path_result`

## 普通版本

需要 CMake 3.16 或更高版本和支持 C11 的编译器。

### Visual Studio Build Tools 2022

在 Developer PowerShell for VS 2022 中运行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMSP_ENABLE_3D=OFF
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\map_shortest_path.exe
```

### MinGW

先确保 `gcc`、`mingw32-make` 和 `cmake` 已加入 `PATH`，不要在项目文档或脚本中写死本机安装路径。

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DMSP_ENABLE_3D=OFF
cmake --build build
ctest --test-dir build --output-on-failure
.\build\map_shortest_path.exe
```

### Linux / macOS

```bash
cmake -S . -B build -DMSP_ENABLE_3D=OFF
cmake --build build
ctest --test-dir build --output-on-failure
./build/map_shortest_path
```

构建过程会把整个 `data` 目录复制到可执行文件同级目录。程序启动时优先读取可执行文件同级的 `data/curved/`，然后兼容当前工作目录中的 `data/curved/`，所以从项目根目录或构建目录启动均应成功。

## 3D 版本

系统已安装 raylib 5.0+ 时会优先使用本机包，否则 CMake 会尝试下载 raylib 5.5。CI 的基础构建关闭 3D，不启动图形窗口。

```powershell
cmake -S . -B build-3d -DMSP_ENABLE_3D=ON
cmake --build build-3d
ctest --test-dir build-3d --output-on-failure
.\build-3d\map_shortest_path.exe --3d
```

多配置生成器需要为构建和测试添加 `--config Debug` 与 `-C Debug`，可执行文件位于 `build-3d/Debug/`。

## 工作目录验证

单配置生成器至少验证以下两种启动方式，并在菜单输入 `0` 退出：

```powershell
.\build\map_shortest_path.exe
Push-Location build
.\map_shortest_path.exe
Pop-Location
```

多配置生成器使用对应的 `build/Debug/` 目录执行同样验证。

## 手工检查

启动程序后检查：默认地图和 6 个 POI 正常加载；Dijkstra 与 A* 均能生成路径；速度选择、暂停/继续、地图文件加载以及曲线和 3D 导航功能保持可用。在 3D 地图空白处按住左键拖动，或按住中键拖动时，地图应平滑平移且不改变起终点。非法节点、相同起终点、不可达路径、错误 CSV 字段、缺失 POI 入口和超容量几何点应安全失败，不能越界或崩溃。

## 提交前静态检查

```powershell
git diff --check
git status --short
```

不要提交 `build/`、`build-3d/`、生成的可执行文件或下载的 raylib 内容。GitHub Actions 会在 Ubuntu 和 Windows 上分别执行 CMake 配置、编译和全部 CTest 测试。

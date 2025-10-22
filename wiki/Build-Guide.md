# 构建指南

本指南将帮助您构建和运行"溯洄遗梦"卡牌游戏项目。

## 系统要求

### 操作系统
- Windows 10 或更高版本
- Windows 11 推荐

### 开发工具
- Microsoft Visual Studio 2022 Community 或更高版本
- SDL2 开发库
- MSBuild 工具

### 依赖库
- SDL2
- SDL2_ttf
- SDL2_image (可能需要)

## 项目结构

项目使用Visual Studio解决方案文件进行管理：

```
Tracer.sln                 # Visual Studio 解决方案文件
├── Tracer/                # 主项目
│   └── src/               # 源代码
├── build.bat             # 构建脚本
└── compile.bat           # 编译脚本
```

## 构建步骤

### 方法一：使用Visual Studio

1. 打开 `Tracer.sln` 解决方案文件
2. 选择构建配置（Debug 或 Release）
3. 选择平台（x64 推荐）
4. 点击"生成" → "生成解决方案"

### 方法二：使用命令行脚本

项目提供了两个构建脚本：

1. **compile.bat** - 编译项目
   ```cmd
   compile.bat
   ```

2. **build.bat** - 构建项目
   ```cmd
   build.bat
   ```

这两个脚本都会调用MSBuild来构建项目。

### 方法三：直接使用MSBuild

如果已安装Visual Studio，可以直接使用MSBuild命令：

```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Tracer.sln /p:Configuration=Debug /p:Platform=x64
```

## 依赖库配置

### SDL2库安装

1. 从[SDL官网](https://www.libsdl.org/)下载SDL2开发库
2. 解压到合适的位置（如 `C:\SDL2`）
3. 在Visual Studio中配置包含目录和库目录：
   - 包含目录：`C:\SDL2\include`
   - 库目录：`C:\SDL2\lib\x64`

### 链接器设置

确保在项目设置中链接以下库文件：
- `SDL2.lib`
- `SDL2main.lib`
- `SDL2_ttf.lib`

## 运行项目

构建成功后，可执行文件将生成在输出目录中（通常是 `x64/Debug/` 或 `x64/Release/`）。

### 运行方式

1. 直接运行生成的可执行文件
2. 在Visual Studio中按F5调试运行
3. 使用命令行运行：
   ```cmd
   cd x64\Debug
   Tracer.exe
   ```

## 常见问题和解决方案

### 1. 缺少SDL2库

**错误信息**：无法找到SDL2头文件或库文件

**解决方案**：
- 确保已正确安装SDL2开发库
- 检查Visual Studio项目设置中的包含目录和库目录配置
- 确认链接器设置中包含了SDL2相关库文件

### 2. 运行时缺少DLL文件

**错误信息**：缺少SDL2.dll等动态链接库

**解决方案**：
- 将SDL2.dll等文件复制到可执行文件所在目录
- 或将SDL2的bin目录添加到系统PATH环境变量

### 3. 编码问题

**问题**：中文显示乱码

**解决方案**：
- 确保源代码文件使用UTF-8编码
- 在代码中正确设置字体编码

### 4. 构建配置问题

**问题**：构建失败，平台不匹配

**解决方案**：
- 确保选择正确的平台（x64）
- 检查项目属性中的平台配置

## 调试建议

1. **启用调试信息**：在Debug配置下构建以获得完整的调试信息
2. **使用调试器**：在Visual Studio中设置断点进行调试
3. **日志输出**：在关键位置添加日志输出以跟踪程序执行
4. **内存检查**：使用Visual Studio的诊断工具检查内存泄漏

## 性能优化建议

1. **Release构建**：发布版本使用Release配置以获得最佳性能
2. **优化设置**：在项目属性中启用适当的优化选项
3. **资源管理**：合理管理纹理、字体等资源的加载和释放
4. **渲染优化**：避免不必要的重绘，使用双缓冲技术

通过遵循本指南，您应该能够成功构建和运行"溯洄遗梦"项目。如果遇到其他问题，请检查错误信息并参考SDL2官方文档。
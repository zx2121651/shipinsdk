# 跨平台短视频/特效渲染引擎 (VFX Engine SDK)

本项目是一款现代化的、具备跨平台显式图形抽象 (RHI) 的短视频与特效渲染引擎。系统架构以高性能、跨端互通为核心，旨在解决移动端复杂的实时滤镜、多轨渲染及高性能视频导出等需求。

## 核心架构特性

1. **严格异步的渲染线程模型 (RenderThread)**
   * **禁止主线程拥塞**：系统天然设计了独立的渲染线程 (`RenderThread`) 与任务调度队列 (`TaskQueue`)，强制将所有重量级计算（如图形管线编译、硬解码协商）从 UI 主线程或业务浅层抽离。
   * **事件闭包分发**：支持从 JNI 或上层业务安全地向渲染引擎异步投递任务，彻底解决由于状态竞争或渲染耗时导致的 UI 卡顿。

2. **跨平台显式图形抽象 (RHI - Render Hardware Interface)**
   * **统一 API**：使用标准 C++20 构建了一套统一的底层图形接口 (`IRHI`)，目前基于现代 OpenGL ES (动态协商 GLES 3.2 到 2.0)，并预留了 Vulkan/Metal 的接入抽象。
   * **动态降级**：通过 `HardwareCapabilities` 进行设备能力探测，自动规避不支持的硬件扩展。
   * **多目标渲染**：底层原生支持同时绑定屏幕预览表面和后台硬件编码器表面，实现无感知的后台导出。

3. **高性能零拷贝视频管线 (Zero-Copy Pipeline)**
   * **硬件级直通**：结合 Android CameraX 获取的 `SurfaceTexture`，使用 `GL_TEXTURE_EXTERNAL_OES` 将硬件摄像头流直接送入 GPU 处理，全程 CPU 零参与，无任何像素回读与内存拷贝开销。
   * **RenderGraph 特效渲染图**：采用节点式的渲染架构 (`IFilter`)，利用 FBO (Framebuffer Object) 乒乓缓冲策略实现多滤镜链式叠加，并严格隔离每一步的 OpenGL 状态。

4. **NDK 原生硬件加速视频录制**
   * **独立编码器**：抛弃传统 Java 层封装，基于 Android NDK 原生 API (`AMediaCodec` + `AMediaMuxer`) 构建了 C++ 层的 `VideoEncoder`。
   * **H.265 (HEVC) 高规格支持**：支持动态协商开启 H.265 硬件编码（如果设备不支持会自动回退至 H.264），将 GPU 渲染结果通过底层共享 Surface 直接送入编码器队列。

## Android Demo (剪映风格极简复刻)

仓库内包含一个 `android_demo` 工程，用于验证 C++ SDK (JNI) 和渲染管线的能力：

* **单 Activity Jetpack Compose 架构**：彻底抛弃了 XML 与多 Activity，使用纯 Compose + Navigation 实现了完全现代化的声明式 UI。
* **沉浸式交互**：高度还原了类似“剪映 (CapCut)”的主页控制台和全屏录像页面布局，包含动态反馈的录制按钮等。
* **解耦模块**：包含引擎封装库 (`vfx-core`)、视频预览组件 (`vfx-preview`) 和录制控制面板 (`vfx-record`)。

## 编译与运行指南

### C++ 核心库编译
依赖 CMake 构建系统：
```bash
mkdir build && cd build
cmake ..
make
# 运行底层单元测试
make test
```

### Android Demo 编译
在安装了 Android SDK / NDK 和 Gradle 之后，进入 Android 工程目录：
```bash
cd android_demo
# 构建 Debug 版本 APK
gradle assembleDebug
# 运行单元测试
gradle test
```

*注意：本应用需在 Android 设备上申请 Camera 权限以启动摄像头进行 OES 纹理实时预览及后续的录制功能体验。*

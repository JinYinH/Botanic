# 打包 / 分享说明 · Packaging & Sharing

## Windows（已有现成版本，直接发这个）

发给朋友：**`dist` 文件夹**，或者更推荐发我压好的 **`dist/Botanic-Windows.zip`**（约 37 MB，只含插件本体）。

zip 里面是：

```
Botanic.vst3              ← 插件本体（VST3，文件夹形式，不要拆开）
Botanic Standalone.exe    ← 独立运行版，不装宿主也能试听
Install-VST3.bat          ← 右键“以管理员身份运行”即可安装
install.ps1               ← 安装脚本（bat 内部调用）
安装说明.txt
README.md
```

对方收到后：

1. 解压到任意位置（**不要把 `Botanic.vst3` 里的内容单独拿出来**）；
2. 右键 `Install-VST3.bat` → **以管理员身份运行** → 插件会装到
   `C:\Program Files\Common Files\VST3\Botanic.vst3`；
   （没有管理员权限时脚本会自动装到 `%LOCALAPPDATA%\Programs\Common\VST3\`）
3. 在 FL Studio / Studio One / Reaper / Cubase 里重新扫描插件；
4. 如果 Windows 提示“已保护你的电脑”，点 **更多信息 → 仍要运行**（插件没有代码签名）。

> 不同 Windows 版本/宿主也能直接手动安装：把 `Botanic.vst3` 整个文件夹复制到
> `C:\Program Files\Common Files\VST3\` 即可。

## macOS（目前**没有**现成版本，需要在 Mac 上编译）

我这边是 Windows + MinGW 环境，只能产出 Windows 的 `.vst3`（里面是 `Contents/x86_64-win`），
**无法在 Windows 上生成 macOS 的二进制**：macOS 需要 Xcode 的 clang、macOS SDK 和 `codesign`。

工程已经调整为跨平台可直接编译（macOS 上会自动额外生成 **AU**，Logic / GarageBand 可用）。
只要朋友（或你）有一台装了 Xcode 的 Mac，按下面三步就能得到 Mac 版：

```bash
# 1. 安装依赖（一次）
xcode-select --install
brew install cmake

# 2. 配置（在工程根目录）
cmake -S . -B build -G Xcode \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13

# 3. 编译
cmake --build build --config Release
```

产物位置：

```
build/Botanic_artefacts/Release/VST3/Botanic.vst3       ← 发给用 VST3 的宿主
build/Botanic_artefacts/Release/AU/Botanic.component   ← Logic / GarageBand
build/Botanic_artefacts/Release/Standalone/Botanic.app ← 独立版
```

安装：

```bash
cp -R build/Botanic_artefacts/Release/VST3/Botanic.vst3   ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Botanic_artefacts/Release/AU/Botanic.component ~/Library/Audio/Plug-Ins/Components/
```

如果是从网上下载的包被 Gatekeeper 拦截，清除一次隔离属性：

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Botanic.vst3
```

> 说明：JUCE 工程里唯一的 Windows 专属设置（`JUCE_DIRECT2D=0`、静态链接参数）都包在
> `if(MINGW)` 里，macOS 构建不受影响；界面用软件渲染，不依赖 Direct2D。

# Botanic · 简介 / Introduction

> 一个会呼吸的混合合成器：温暖的声音，长在自然里。

---

## 一句话

**Botanic** 是一款混合波形表合成器——温暖的模拟质感、颗粒与故障的纹理实验，
以及一个会随声音呼吸、随音色变形的主视觉。中文界面，开箱即用。

---

## 短版（发朋友圈 / 论坛 / 群里）

Botanic 是我做的一款混合波形表合成器（VST3 + 独立版）。

它有三个振荡器（每个最多 8 层 unison）、梯形/SVF 滤波器、10×19 调制矩阵、
可自由排序的效果链，还有一个"纹理实验室"：颗粒云、故障卡带、以及能
在 A-E-I-O-U 之间变形的元音共振峰——自然的声音和数字的崩坏可以拧在一起。

里面带了 38 个音色，从温暖电钢、柔软拨弦、厚实贝斯，到林间铺底和会说话的故障纹理。
界面中英文一键切换，旋钮右键就能挂调制。中间那朵花会跟着你的音色变色、跟着声音震荡，
弹琴的时候它会一圈圈泛起涟漪。

Windows 版现在就能用，Mac 版需要在 Mac 上编译（工程已备好）。

---

## 长版（产品页 / 介绍文档）

### 声音的两种生长方式

大多数合成器要么"干净"，要么"脏"。Botanic 想做的是**可以互相生长的那一种**：
一条温暖厚实的模拟式发声链，长出一层颗粒与故障的枝叶，再用一个元音滤波器把它"唱"出来。

### 发声核心

- **三个波形表振荡器**，每个支持 **1–8 层 Unison**、独立失谐/相位/声像/键位；
- **8 张波形表**（波形表 / 正弦 / 三角 / 锯齿 / 方波 / 玻璃 / 共振峰 / 钟琴），每张 16 帧可连续变形；
- 全频段**抗混叠**（每张表 9 级 mipmap），整条发声链在 **2 倍采样率**下运行，
  非线性失真不再折叠回可听频段；
- **TPT 梯形低通 LP24**（带驱动饱和）与 **SVF**（低通/带通/高通/陷波）双滤波架构；
- 双 ADSR 包络 + 双 LFO（9 种波形，含混沌与脉冲）+ **10 源 × 19 目标**调制矩阵。

### 纹理实验室（这是它的性格）

| 模块 | 它能做什么 |
| --- | --- |
| **GRAIN 颗粒** | 2 秒立体声缓冲、48 个粒子槽、冻结、喷洒、反向、随机音高——把任何音符变成漂浮的云 |
| **GLITCH 故障** | 位深压缩、降采样、跟拍速的 stutter / 磁带停转 / 门限 / 丢帧 / 数字碎屑，混沌控制事件密度 |
| **VOWEL 元音** | 三组共振峰在 A→E→I→O→U 之间连续变形，**不改变音高**，并可被 LFO/包络调制 |

### 演奏与控制

- **PERFORM 页**：可分配的 XY 宏，两个轴各选一个目标（滤波、共振、驱动、元音偏移、
  粒子干湿/位置、故障干湿/位深、混响、Tilt），并带 **MOTION** 自动运动；
- **琶音器**：上行/下行/上下行/随机/按演奏顺序/和弦，1/4–1/32，可摆动；
- **A/B 对比**、**40 步撤销**、**MIDI Learn**、**全控件悬停提示**；
- 屏幕钢琴支持鼠标滑奏，也支持电脑键盘演奏。

### 会呼吸的界面

主视觉不是一张静态图：它跟着**滤波截止与泛音**变亮变暗（深绿 → 青柠），
跟着**颗粒/故障/混响**长出更密的花粉、被撕成错位的横向条带，
每弹一个音就从花心扩散出一圈涟漪，电平越大涟漪越快、光晕越亮。
60 帧动画，中文 / English 一键切换。

### 规格

| | |
| --- | --- |
| 格式 | VST3 + 独立版（Windows）；macOS 可在 Mac 上编译出 VST3 / AU / 独立版 |
| 复音 | 1–32 声部，3 振荡器 × 8 unison |
| 预设 | 38 个工厂音色（温暖常规音色 + 故障/粒子/元音实验音色）+ 用户预设 |
| 延迟 | 16 采样（2 倍过采样降采样滤波器，宿主自动补偿） |
| 界面 | 1672×941 矢量式界面，可缩放；中文 / English |
| 平台 | Windows 10/11 x64（VST3 / 独立版） |

---

## English

**Botanic** is a hybrid wavetable synthesizer for people who like their sounds
warm, alive, and slightly overgrown.

Three wavetable oscillators with up to eight unison voices each, a TPT ladder
and SVF filter pair, a 10×19 modulation matrix and a reorderable five-slot FX
chain — plus a texture lab where a 48-grain granular engine, a tempo-locked
glitch sequencer and a morphing A-E-I-O-U formant bank can be grown into the
sound or tear it apart.

The whole voice path runs at 2× the host sample rate and is band-limited back,
so drive and filter saturation stay clean instead of folding into the audible
band. The interface is animated at 60 fps and reacts to the patch: brighter
timbres shift the artwork towards lime, grainy patches tear it into displaced
bands, every note sends a ripple out of the flower.

38 factory presets, A/B compare, 40-step undo, MIDI learn, and a full Chinese
translation alongside the English UI. VST3 and standalone on Windows; the
project also builds VST3 / AU / standalone on macOS.

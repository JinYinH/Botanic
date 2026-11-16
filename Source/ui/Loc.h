#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <string>
#include <unordered_map>

namespace ui
{

/** Very small runtime localiser: the English string is the key. */
class Locale
{
public:
    static void setLanguage (int lang) noexcept { current.store (lang); }
    static int getLanguage() noexcept { return current.load(); }
    static bool isChinese() noexcept { return current.load() == 1; }

    static juce::String t (const char* englishKey)
    {
        if (current.load() == 0)
            return juce::String (englishKey);

        const auto& map = table();
        const auto it = map.find (englishKey);

        if (it != map.end())
            return juce::String::fromUTF8 (it->second);

        return juce::String (englishKey);
    }

private:
    static const std::unordered_map<std::string, const char*>& table()
    {
        static const std::unordered_map<std::string, const char*> translations
        {
            // navigation + panels
            { "Home", "主页" }, { "Oscillator", "振荡器" }, { "Filter", "滤波器" },
            { "Perform", "演奏" }, { "PERFORM", "演奏" }, { "MOTION", "自动运动" },
            { "Vowel", "元音" }, { "Tilt", "倾斜" }, { "Air", "空气感" }, { "Shift", "偏移" }, { "Key", "键位" }, { "Vowel On", "元音开关" },
            { "Vowel Shift", "元音偏移" },
            { "three formant bands morph between A E I O U without changing pitch",
              "三组共振峰在 A E I O U 之间变形，不改变音高" },
            { "drag the pad to move both macros - right column assigns what they control",
              "拖动方框同时移动两个宏 · 右侧选择它们控制什么" },
            { "drag to adjust - shift for fine - right click to modulate",
              "拖动调节 · Shift 微调 · 右键调制" },
            { "A preset with that name already exists", "同名预设已存在" },
            { "Please enter a name", "请输入名字" },
            { "SETTINGS", "设置" }, { "VOWEL", "元音" }, { "Q", "Q" },
            { "ON", "开" }, { "Step", "步进" }, { "Stutter", "卡带" },
            { "Poly", "复音" }, { "Mono", "单音" }, { "Legato", "连奏" },
            { "Wavetable", "波形表" }, { "Sine", "正弦" }, { "Triangle", "三角波" },
            { "Saw", "锯齿波" }, { "Square", "方波" }, { "Glass", "玻璃" },
            { "Formant", "共振峰" }, { "Bells", "钟琴" },
            { "LP 24dB", "低通 24" }, { "LP 12dB", "低通 12" },
            { "BP 12dB", "带通 12" }, { "HP 12dB", "高通 12" }, { "Notch", "陷波" },
            { "Saw Up", "锯齿上" }, { "Saw Down", "锯齿下" }, { "S & H", "采样保持" },
            { "Up", "上行" }, { "Down", "下行" }, { "Up-Down", "上下行" },
            { "As Played", "按演奏顺序" }, { "Chord", "和弦" },
            { "Botanic 1.4  |  hybrid wavetable synthesizer", "Botanic 1.4  |  混合波形表合成器" },
            { "Panic", "紧急停止" },
            { "Compare slot A", "对比 A" }, { "Compare slot B", "对比 B" },
            { "Filter Cutoff", "滤波截止" }, { "Filter Drive", "滤波驱动" },
            { "Grain Mix", "粒子干湿" }, { "Grain Position", "粒子位置" },
            { "Glitch Mix", "故障干湿" }, { "Glitch Bits", "故障位深" },
            { "Reverb Mix", "混响干湿" }, { "Off", "关闭" },
            { "Modulation", "调制" }, { "Effects", "效果" }, { "Texture", "纹理" },
            { "Arpeggiator", "琶音器" }, { "Settings", "设置" },
            { "OSCILLATOR", "振荡器" }, { "FILTER", "滤波器" }, { "ENV", "包络" },
            { "MODULATION", "调制" }, { "EFFECTS", "效果" }, { "FX CHAIN", "效果链" },
            { "VOLUME", "音量" }, { "MASTER", "主输出" },

            // tabs
            { "GRAIN", "粒子" }, { "GLITCH", "故障" }, { "Reverb", "混响" },
            { "Delay", "延迟" }, { "Chorus", "合唱" }, { "Comp", "压缩" },
            { "AMP", "振幅" }, { "ENV 1", "包络 1" }, { "ENV 2", "包络 2" },
            { "Compressor", "压缩器" }, { "LFO 1", "LFO 1" }, { "LFO 2", "LFO 2" },

            // knob captions
            { "Tune", "音准" }, { "Detune", "失谐" }, { "Level", "电平" },
            { "Cutoff", "截止频率" }, { "Resonance", "共振" }, { "Drive", "驱动" },
            { "Mix", "干湿比" }, { "Size", "尺寸" }, { "Damp", "阻尼" },
            { "Rate", "速率" }, { "Depth", "深度" }, { "Phase", "相位" },
            { "Attack", "起音" }, { "Decay", "衰减" }, { "Sustain", "延音" },
            { "Release", "释音" }, { "Width", "宽度" }, { "Time", "时间" },
            { "Feedback", "反馈" }, { "Threshold", "阈值" }, { "Ratio", "比率" },
            { "Makeup", "补偿" }, { "Low", "低频" }, { "Mid", "中频" }, { "High", "高频" },
            { "Octaves", "八度" }, { "Gate", "门限" }, { "Swing", "摇摆" },
            { "Latch", "保持" }, { "Unison", "齐奏" }, { "Spread", "扩散" },
            { "Pan", "声像" }, { "Reverse", "反向" }, { "Density", "密度" },
            { "Position", "位置" }, { "Spray", "散布" }, { "Pitch", "音高" },
            { "Pitch Rnd", "随机音高" }, { "Bits", "位深" }, { "Stutter", "卡带" },
            { "Dropout", "掉音" }, { "Tape", "磁带" }, { "Repeats", "重复" },
            { "Chaos", "混沌" }, { "Noise", "噪声" }, { "Env Amt", "包络量" },
            { "Key Track", "键位跟踪" }, { "Glide", "滑音" }, { "Bend", "弯音" },
            { "Polyphony", "复音数" }, { "Tuning A4", "调音 A4" }, { "Type", "类型" },
            { "Division", "时值" }, { "Step", "步进" }, { "Grain On", "粒子开关" },
            { "Freeze", "冻结" }, { "Glitch On", "故障开关" }, { "Vel > Amp", "力度→音量" },
            { "Sync", "同步" }, { "Random", "随机" },

            // pages and titles
            { "MODULATION MATRIX", "调制矩阵" }, { "ARPEGGIATOR", "琶音器" },
            { "PRESETS", "预设库" }, { "OSCILLATOR DETAILS", "振荡器细节" },
            { "FILTER & VOICE", "滤波器与声部" }, { "EFFECT CHAIN DETAILS", "效果链细节" },
            { "TEXTURE LAB", "纹理实验室" },
            { "drag a cell vertically to set depth - double click to clear",
              "上下拖动格子设置深度 · 双击清除" },
            { "steps follow host tempo", "步进跟随宿主速度" },
            { "grains of the synth bus, frozen in time - position drags through the buffer",
              "把合成器总线切成粒子 · Position 在缓冲中拖动" },
            { "bit crush + tempo locked accidents - chaos scales how often they happen",
              "位深压缩 + 跟随速度的随机事故 · Chaos 控制发生频率" },
            { "Right-click any control to assign modulation or MIDI learn.\n"
              "Drag the centre artwork away with the nav: Modulation opens the\n"
              "matrix, Arpeggiator and Settings open their pages.",
              "右键任意控件可分配调制或 MIDI 学习。\n"
              "左侧导航：调制打开矩阵，纹理、琶音器、设置会打开对应页面。\n"
              "中间的花朵与标题可以拖动、点击。" },
            { "Botanic 1.0  |  hybrid wavetable synthesizer", "Botanic 1.0  |  混合波表合成器" },

            // buttons
            { "TRIGGER", "触发" }, { "RANDOM", "随机" }, { "Init patch", "初始化音色" },
            { "Save as preset...", "保存为预设…" }, { "Save preset", "保存预设" },
            { "Enter a name for this patch", "输入这个音色的名字" }, { "Name", "名称" },
            { "Save", "保存" }, { "Cancel", "取消" },

            // menus
            { "Generative randomiser", "生成式随机化" }, { "Everything", "全部" },
            { "Oscillators", "振荡器" }, { "Filter + envelopes", "滤波与包络" },
            { "Texture (grain / glitch)", "纹理（粒子/故障）" },
            { "Reset to default", "恢复默认" }, { "Modulate", "调制" },
            { "Clear all modulation", "清除全部调制" }, { "MIDI Learn", "MIDI 学习" },
            { "Clear MIDI mapping", "清除 MIDI 映射" }, { "Not a modulation target", "不是调制目标" },
            { "Randomise...", "随机化…" }
        };

        return translations;
    }

    inline static std::atomic<int> current { 0 };
};

inline juce::String T (const char* key) { return Locale::t (key); }

} // namespace ui

#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>
#include <string>
#include <thread>

#include "asciiscope.hpp"

#ifdef _WIN32
#include <conio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

bool hasArg(int argc, char** argv, std::string_view target) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == target) {
            return true;
        }
    }
    return false;
}

std::optional<std::string_view> argValue(int argc, char** argv, std::string_view target) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == target) {
            return argv[i + 1];
        }
    }

    return std::nullopt;
}

std::optional<int> modeFromName(std::string_view mode) {
    if (mode == "1" || mode == "bloom") {
        return 0;
    }
    if (mode == "2" || mode == "tunnel") {
        return 1;
    }
    if (mode == "3" || mode == "particles" || mode == "particle") {
        return 2;
    }
    if (mode == "4" || mode == "spectral" || mode == "ribbon") {
        return 3;
    }

    return std::nullopt;
}

std::string_view glyphStyleName(int style) {
    switch (style % 4) {
    case 0:
        return "classic";
    case 1:
        return "dense";
    case 2:
        return "blocks";
    default:
        return "wire";
    }
}

std::string_view glyphRampForStyle(int style) {
    switch (style % 4) {
    case 0:
        return " .:-=+*#%@";
    case 1:
        return " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    case 2:
        return "  ..::--==++**##%%@@";
    default:
        return " .`-~:+<>\\/|{}[]()1tfLCG08@";
    }
}

std::optional<int> glyphStyleFromName(std::string_view style) {
    if (style == "classic" || style == "scope") {
        return 0;
    }
    if (style == "dense" || style == "crt") {
        return 1;
    }
    if (style == "blocks" || style == "block") {
        return 2;
    }
    if (style == "wire" || style == "vector") {
        return 3;
    }

    return std::nullopt;
}

std::string_view paletteName(int palette) {
    switch (palette % 4) {
    case 0:
        return "neon";
    case 1:
        return "ember";
    case 2:
        return "acid";
    default:
        return "ice";
    }
}

std::optional<int> paletteFromName(std::string_view palette) {
    if (palette == "neon" || palette == "default") {
        return 0;
    }
    if (palette == "ember" || palette == "fire") {
        return 1;
    }
    if (palette == "acid" || palette == "green") {
        return 2;
    }
    if (palette == "ice" || palette == "blue") {
        return 3;
    }

    return std::nullopt;
}

std::optional<int> positiveIntValue(int argc, char** argv, std::string_view target) {
    const auto value = argValue(argc, argv, target);
    if (!value.has_value() || value->empty()) {
        return std::nullopt;
    }

    char* end{};
    const auto parsed = std::strtol(value->data(), &end, 10);
    if (end == value->data() || *end != '\0' || parsed <= 0) {
        return std::nullopt;
    }

    return static_cast<int>(parsed);
}

std::optional<std::uint32_t> seedValue(int argc, char** argv, std::string_view target) {
    const auto value = argValue(argc, argv, target);
    if (!value.has_value() || value->empty()) {
        return std::nullopt;
    }

    char* end{};
    const auto parsed = std::strtoull(value->data(), &end, 0);
    if (end == value->data() || *end != '\0' || parsed > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(parsed);
}

int boundedIntOption(int argc, char** argv, std::string_view target, int fallback, int minimum, int maximum) {
    if (const auto value = positiveIntValue(argc, argv, target)) {
        return std::clamp(*value, minimum, maximum);
    }

    return fallback;
}

std::optional<double> doubleValue(int argc, char** argv, std::string_view target) {
    const auto value = argValue(argc, argv, target);
    if (!value.has_value() || value->empty()) {
        return std::nullopt;
    }

    char* end{};
    const auto parsed = std::strtod(value->data(), &end);
    if (end == value->data() || *end != '\0') {
        return std::nullopt;
    }

    return parsed;
}

double boundedDoubleOption(int argc, char** argv, std::string_view target, double fallback, double minimum, double maximum) {
    if (const auto value = doubleValue(argc, argv, target)) {
        return std::clamp(*value, minimum, maximum);
    }

    return fallback;
}

void printHelp() {
    std::cout
      << "asciiscope - terminal-native signal visuals\n\n"
      << "Options:\n"
      << "  --help                 show this help\n"
      << "  --once                 run a short 90-frame smoke demo\n"
      << "  --frames N             run exactly N frames\n"
      << "  --seconds N            run for N seconds at the selected fps\n"
      << "  --warmup N             draw N hidden frames before recording\n"
      << "  --fps N                presentation rate, 1 to 240\n"
      << "  --seed N               repeatable demo seed, decimal or 0x hex\n"
      << "  --reel                 8s canvas capture defaults for social clips\n"
      << "  --mode NAME            bloom | tunnel | particles | spectral\n"
      << "  --preset NAME          bloom-reel | neon-tunnel | particle-storm | ghost-spectral\n"
      << "  --width N              canvas width, 40 to 220\n"
      << "  --height N             canvas height, 16 to 80\n"
      << "  --speed N              visual speed, 0.15 to 4.0\n"
      << "  --density N            signal density, 0.25 to 2.0\n"
      << "  --zoom N               visual zoom, 0.25 to 4.0\n"
      << "  --trail N              trail fade amount, 1 to 8\n"
      << "  --glyphs NAME          classic | dense | blocks | wire\n"
      << "  --palette NAME         neon | ember | acid | ice\n"
      << "  --title TEXT           title shown in the header\n"
      << "  --canvas-only          hide header, border, and footer\n"
      << "  --no-hud               hide the footer readout\n"
      << "  --no-color             monochrome output\n\n"
      << "Examples:\n"
      << "  asciiscope --preset neon-tunnel\n"
      << "  asciiscope --preset particle-storm --reel\n"
      << "  asciiscope --mode spectral --seconds 8 --fps 30\n";
}

struct Controls {
    bool running{ true };
    bool paused{ false };
    bool help{ false };
    bool color{ true };
    int mode{ -1 };
    double speed{ 1.0 };
    double density{ 1.0 };
    double zoom{ 1.0 };
    int fade{ 2 };
    int glyphStyle{ 0 };
    int palette{ 0 };
    bool clearRequested{ false };
    std::string lastAdjustment{ "ready" };
    std::string inputStatus{ "input pending" };
};

bool applyPreset(std::string_view preset, Controls& controls, int& frameLimit, int& width, int& height) {
    frameLimit = 240;

    if (preset == "bloom-reel" || preset == "bloom") {
        controls.mode = 0;
        controls.speed = 1.1;
        controls.density = 1.2;
        controls.zoom = 1.1;
        controls.fade = 2;
        controls.glyphStyle = 0;
        controls.palette = 0;
    } else if (preset == "neon-tunnel" || preset == "tunnel") {
        controls.mode = 1;
        controls.speed = 1.2;
        controls.density = 1.1;
        controls.zoom = 1.15;
        controls.fade = 2;
        controls.glyphStyle = 1;
        controls.palette = 0;
        width = 120;
        height = 36;
    } else if (preset == "particle-storm" || preset == "particles") {
        controls.mode = 2;
        controls.speed = 1.4;
        controls.density = 1.6;
        controls.zoom = 1.25;
        controls.fade = 3;
        controls.glyphStyle = 2;
        controls.palette = 1;
    } else if (preset == "ghost-spectral" || preset == "spectral") {
        controls.mode = 3;
        controls.speed = 0.85;
        controls.density = 1.25;
        controls.zoom = 1.0;
        controls.fade = 4;
        controls.glyphStyle = 3;
        controls.palette = 3;
        width = 128;
        height = 36;
    } else {
        frameLimit = 0;
        return false;
    }

    controls.lastAdjustment = "preset";
    return true;
}

void handleKey(int key, Controls& controls) {
    if (key == 'Z') {
        controls.zoom = std::min(4.0, controls.zoom + 0.08);
        controls.lastAdjustment = "zoom";
        return;
    }

    const auto lower = static_cast<char>(std::tolower(key));

    switch (lower) {
    case 'q':
    case 27:
        controls.running = false;
        break;
    case ' ':
        controls.paused = !controls.paused;
        controls.lastAdjustment = controls.paused ? "pause" : "resume";
        break;
    case '0':
        controls.mode = -1;
        controls.lastAdjustment = "mode auto";
        break;
    case '1':
        controls.mode = 0;
        controls.lastAdjustment = "mode bloom";
        break;
    case '2':
        controls.mode = 1;
        controls.lastAdjustment = "mode tunnel";
        break;
    case '3':
        controls.mode = 2;
        controls.lastAdjustment = "mode particles";
        break;
    case '4':
        controls.mode = 3;
        controls.lastAdjustment = "mode spectral";
        break;
    case '+':
    case '=':
        controls.speed = std::min(4.0, controls.speed + 0.15);
        controls.lastAdjustment = "speed";
        break;
    case '-':
    case '_':
        controls.speed = std::max(0.15, controls.speed - 0.15);
        controls.lastAdjustment = "speed";
        break;
    case ']':
        controls.density = std::min(2.0, controls.density + 0.1);
        controls.lastAdjustment = "density";
        break;
    case '[':
        controls.density = std::max(0.25, controls.density - 0.1);
        controls.lastAdjustment = "density";
        break;
    case '.':
    case '>':
        controls.fade = std::min(8, controls.fade + 1);
        controls.lastAdjustment = "trail";
        break;
    case ',':
    case '<':
        controls.fade = std::max(1, controls.fade - 1);
        controls.lastAdjustment = "trail";
        break;
    case 'z':
        controls.zoom = std::max(0.25, controls.zoom - 0.08);
        controls.lastAdjustment = "zoom";
        break;
    case 'c':
        controls.color = !controls.color;
        controls.lastAdjustment = "color";
        break;
    case 'g':
        controls.glyphStyle = (controls.glyphStyle + 1) % 4;
        controls.lastAdjustment = "glyphs";
        break;
    case 'p':
        controls.palette = (controls.palette + 1) % 4;
        controls.lastAdjustment = "palette";
        break;
    case 'x':
    case 'r':
        controls.clearRequested = true;
        controls.lastAdjustment = "clear";
        break;
    case 'h':
    case '?':
        controls.help = !controls.help;
        break;
    default:
        break;
    }
}

void nudgeZoom(Controls& controls, int direction) {
    const double ratio = direction > 0 ? 1.08 : 1.0 / 1.08;
    controls.zoom = std::clamp(controls.zoom * ratio, 0.25, 4.0);
    controls.lastAdjustment = "mousewheel zoom";
}

#ifdef _WIN32
struct ConsoleInputState {
    HANDLE input{ INVALID_HANDLE_VALUE };
    DWORD originalMode{};
    bool configured{};
};

ConsoleInputState configureConsoleInput() {
    ConsoleInputState state;
    state.input = GetStdHandle(STD_INPUT_HANDLE);
    if (state.input == INVALID_HANDLE_VALUE) {
        return state;
    }

    if (GetConsoleMode(state.input, &state.originalMode) == 0) {
        return state;
    }

    DWORD mode = state.originalMode;
    mode |= ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT;
    mode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    state.configured = SetConsoleMode(state.input, mode) != 0;
    return state;
}

void restoreConsoleInput(const ConsoleInputState& state) {
    if (state.configured) {
        SetConsoleMode(state.input, state.originalMode);
    }
}
#endif

void pollControls(Controls& controls
#ifdef _WIN32
                  ,
                  const ConsoleInputState& inputState
#endif
) {
#ifdef _WIN32
    if (inputState.configured) {
        controls.inputStatus = "input win32";
        DWORD eventCount = 0;
        while (GetNumberOfConsoleInputEvents(inputState.input, &eventCount) != 0 && eventCount > 0) {
            INPUT_RECORD record{};
            DWORD readCount = 0;
            if (ReadConsoleInput(inputState.input, &record, 1, &readCount) == 0 || readCount == 0) {
                break;
            }

            if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
                const char key = record.Event.KeyEvent.uChar.AsciiChar;
                if (key != 0) {
                    handleKey(key, controls);
                } else {
                    switch (record.Event.KeyEvent.wVirtualKeyCode) {
                    case VK_UP:
                        handleKey('+', controls);
                        break;
                    case VK_DOWN:
                        handleKey('-', controls);
                        break;
                    case VK_RIGHT:
                        handleKey(']', controls);
                        break;
                    case VK_LEFT:
                        handleKey('[', controls);
                        break;
                    default:
                        break;
                    }
                }
            } else if (record.EventType == MOUSE_EVENT && record.Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
                const auto wheel = static_cast<short>(HIWORD(record.Event.MouseEvent.dwButtonState));
                nudgeZoom(controls, wheel > 0 ? 1 : -1);
            }
        }

        return;
    }

    controls.inputStatus = "input kbhit";
    while (_kbhit() != 0) {
        int key = _getch();
        if (key == 0 || key == 224) {
            key = _getch();
            switch (key) {
            case 72:
                handleKey('+', controls);
                break;
            case 80:
                handleKey('-', controls);
                break;
            case 77:
                handleKey(']', controls);
                break;
            case 75:
                handleKey('[', controls);
                break;
            default:
                break;
            }
        } else {
            handleKey(key, controls);
        }
    }
#endif
}

std::string footerFor(const Controls& controls, const std::optional<asciiscope::SignalStats>& stats) {
    char footer[280]{};
    const char* mode = controls.mode < 0 ? "auto" : (controls.mode == 0 ? "bloom" : (controls.mode == 1 ? "tunnel" : (controls.mode == 2 ? "particles" : "spectral")));

    if (controls.help) {
        return "1/2/3/4 modes  0 auto  space pause  +/- speed  wheel/z/Z zoom  [/]/arrows density  </> trails  g glyphs  p palette  c color  r clear  q quit";
    }

    if (stats.has_value()) {
        std::snprintf(
          footer,
          sizeof(footer),
          "%s | %s | %.2fx den %.2fx zoom %.2fx trail %d | %s glyphs %s palette | sig rms %.2f pk %.2f min %.2f max %.2f | %s | last %s | h help",
          controls.paused ? "PAUSED" : "LIVE",
          mode,
          controls.speed,
          controls.density,
          controls.zoom,
          controls.fade,
          glyphStyleName(controls.glyphStyle).data(),
          paletteName(controls.palette).data(),
          stats->rms,
          stats->peak,
          stats->min,
          stats->max,
          controls.inputStatus.c_str(),
          controls.lastAdjustment.c_str());
        return footer;
    }

    std::snprintf(
      footer,
      sizeof(footer),
      "%s | mode %s | speed %.2fx | density %.2fx | zoom %.2fx | trail %d | %s glyphs %s palette | color %s | %s | last %s | h help | q quit",
      controls.paused ? "PAUSED" : "LIVE",
      mode,
      controls.speed,
      controls.density,
      controls.zoom,
      controls.fade,
      glyphStyleName(controls.glyphStyle).data(),
      paletteName(controls.palette).data(),
      controls.color ? "on" : "off",
      controls.inputStatus.c_str(),
      controls.lastAdjustment.c_str());
    return footer;
}

} // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    if (hasArg(argc, argv, "--help") || hasArg(argc, argv, "-h")) {
        printHelp();
        return 0;
    }

    Controls controls;
    int frameLimit = 0;
    int width = 112;
    int height = 34;
    int warmupFrames = 0;
    std::uint32_t seed = 0xA5C115C0;
    const bool reelMode = hasArg(argc, argv, "--reel");
    bool showHud = !hasArg(argc, argv, "--no-hud");
    bool showChrome = !hasArg(argc, argv, "--canvas-only") && !reelMode;
    std::string title = "ASCIISCOPE / SOEMDSP";

    if (const auto titleArg = argValue(argc, argv, "--title")) {
        title = std::string(*titleArg);
    }

    if (const auto preset = argValue(argc, argv, "--preset")) {
        applyPreset(*preset, controls, frameLimit, width, height);
    }

    controls.color = !hasArg(argc, argv, "--no-color");
    controls.speed = boundedDoubleOption(argc, argv, "--speed", controls.speed, 0.15, 4.0);
    controls.density = boundedDoubleOption(argc, argv, "--density", controls.density, 0.25, 2.0);
    controls.zoom = boundedDoubleOption(argc, argv, "--zoom", controls.zoom, 0.25, 4.0);
    controls.fade = boundedIntOption(argc, argv, "--trail", controls.fade, 1, 8);
    if (const auto glyphArg = argValue(argc, argv, "--glyphs")) {
        if (const auto glyphStyle = glyphStyleFromName(*glyphArg)) {
            controls.glyphStyle = *glyphStyle;
            controls.lastAdjustment = "glyphs cli";
        }
    }
    if (const auto paletteArg = argValue(argc, argv, "--palette")) {
        if (const auto palette = paletteFromName(*paletteArg)) {
            controls.palette = *palette;
            controls.lastAdjustment = "palette cli";
        }
    }
    if (const auto modeArg = argValue(argc, argv, "--mode")) {
        if (const auto mode = modeFromName(*modeArg)) {
            controls.mode = *mode;
            controls.lastAdjustment = "mode cli";
        }
    }
    if (hasArg(argc, argv, "--once")) {
        frameLimit = 90;
    }
    const int fps = boundedIntOption(argc, argv, "--fps", reelMode ? 30 : 60, 1, 240);
    if (reelMode) {
        frameLimit = fps * 8;
        controls.lastAdjustment = "reel";
    }
    if (const auto seconds = doubleValue(argc, argv, "--seconds")) {
        if (*seconds > 0.0) {
            frameLimit = std::clamp(static_cast<int>((*seconds * static_cast<double>(fps)) + 0.999), 1, 864000);
            controls.lastAdjustment = "seconds cli";
        }
    }
    if (const auto frames = positiveIntValue(argc, argv, "--frames")) {
        frameLimit = *frames;
    }
    warmupFrames = boundedIntOption(argc, argv, "--warmup", reelMode ? 90 : 0, 0, 3600);
    if (warmupFrames > 0) {
        controls.lastAdjustment = "warmup cli";
    }
    if (const auto seedArg = seedValue(argc, argv, "--seed")) {
        seed = *seedArg;
        controls.lastAdjustment = "seed cli";
    }
    const auto frameDelay = std::chrono::milliseconds(std::max(1, 1000 / fps));
    width = boundedIntOption(argc, argv, "--width", width, 40, 220);
    height = boundedIntOption(argc, argv, "--height", height, 16, 80);

    asciiscope::ConsoleRenderer renderer({ .width = width, .height = height, .maxAge = 13, .color = controls.color });
    asciiscope::DemoSignalInput signalInput(seed);
    asciiscope::AnimationScene scene(renderer);

#ifdef _WIN32
    const auto inputState = configureConsoleInput();
#endif

    std::optional<asciiscope::SignalStats> latestStats;
    double visualFrame = 0.0;
    int presentedFrames = 0;

    auto drawNextFrame = [&]() {
        const int frame = static_cast<int>(visualFrame);
        const auto sampleCount = static_cast<std::size_t>(std::clamp(5400.0 * controls.density, 1200.0, 10800.0));
        auto signalFrame = signalInput.nextFrame(static_cast<std::uint64_t>(frame), sampleCount);
        if (const auto* source = signalFrame.findSource("main")) {
            latestStats = source->stats;
        }
        asciiscope::SceneSettings settings{ .mode = controls.mode, .density = controls.density, .zoom = controls.zoom, .fade = controls.fade };
        scene.draw(signalFrame, settings);
        visualFrame += controls.speed;
        return frame;
    };

    renderer.setColor(controls.color);
    renderer.setChrome(showChrome);
    renderer.setGlyphRamp(glyphRampForStyle(controls.glyphStyle));
    renderer.setPalette(controls.palette);
    for (int i = 0; i < warmupFrames; ++i) {
        drawNextFrame();
    }

    while (controls.running && (frameLimit == 0 || presentedFrames < frameLimit)) {
        pollControls(controls
#ifdef _WIN32
                     ,
                     inputState
#endif
        );
        renderer.setColor(controls.color);
        renderer.setChrome(showChrome);
        renderer.setGlyphRamp(glyphRampForStyle(controls.glyphStyle));
        renderer.setPalette(controls.palette);

        if (controls.clearRequested) {
            renderer.clear();
            controls.clearRequested = false;
        }

        int frame = static_cast<int>(visualFrame);

        if (!controls.paused) {
            frame = drawNextFrame();
        }

        const std::string footer = showHud && showChrome ? footerFor(controls, latestStats) : std::string{};
        renderer.present(title, scene.modeName(frame, controls.mode), footer, frame);
        std::this_thread::sleep_for(frameDelay);
        ++presentedFrames;
    }

#ifdef _WIN32
    restoreConsoleInput(inputState);
#endif
    renderer.restoreTerminal();
    return 0;
}

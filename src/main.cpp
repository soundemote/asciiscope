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

constexpr double kMinZoom = 0.25;
constexpr double kMaxZoom = 16.0;
constexpr double kMinCircleFrequency = 0.005;
constexpr double kMaxCircleFrequency = 10.0;
constexpr double kMinBrightness = 0.25;
constexpr double kMaxBrightness = 1.5;
constexpr int kMinBlackFloor = 0;
constexpr int kMaxBlackFloor = 31;

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
    if (mode == "5" || mode == "circle" || mode == "sincos") {
        return 4;
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
      << "  --list-presets         show preset identities and capture recipes\n"
      << "  --describe             show resolved launch settings and exit\n"
      << "  --once                 run a short 90-frame smoke demo\n"
      << "  --frames N             run exactly N frames\n"
      << "  --seconds N            run for N seconds at the selected fps\n"
      << "  --warmup N             draw N hidden frames before recording\n"
      << "  --hold N               keep final frame visible for N seconds\n"
      << "  --fps N                presentation rate, 1 to 240\n"
      << "  --seed N               repeatable demo seed, decimal or 0x hex\n"
      << "  --reel                 8s canvas capture defaults for social clips\n"
      << "  --tour                 cycle the strongest looks in one capture take\n"
      << "  --tour-seconds N       seconds per tour look, 1 to 30\n"
      << "  --mode NAME            bloom | tunnel | particles | spectral | circle\n"
      << "  --preset NAME          bloom-reel | neon-tunnel | particle-storm | ghost-spectral\n"
      << "  --width N              canvas width, 40 to 220\n"
      << "  --height N             canvas height, 16 to 80\n"
      << "  --speed N              visual speed, 0.15 to 4.0\n"
      << "  --density N            signal density, 0.25 to 2.0\n"
      << "  --zoom N               visual zoom, 0.25 to 16.0\n"
      << "  --circle-hz N          circle trace frequency, 0.005 to 10.0 Hz\n"
      << "  --brightness N         trace brightness, 0.25 to 1.5\n"
      << "  --black-floor N        ages at or below this draw as empty cells, 0 to 31\n"
      << "  --trail N              trail fade amount, 1 to 8\n"
      << "  --glyphs NAME          classic | dense | blocks | wire\n"
      << "  --palette NAME         neon | ember | acid | ice\n"
      << "  --title TEXT           title shown in the header\n"
      << "  --canvas-only          hide header, border, and footer\n"
      << "  --no-hud               hide the footer readout\n"
      << "  --no-color             monochrome output\n\n"
      << "  --native-color         use fast 16-color console renderer instead of ANSI truecolor\n"
      << "Examples:\n"
      << "  asciiscope --preset neon-tunnel\n"
      << "  asciiscope --preset particle-storm --reel\n"
      << "  asciiscope --tour --seconds 16 --hold 2\n"
      << "  asciiscope --preset ghost-spectral --reel --describe\n"
      << "  asciiscope --mode spectral --seconds 8 --fps 30\n\n"
      << "Live controls:\n"
      << "  h or ?                hide/show the in-canvas control panel\n"
      << "  1 2 3 4 5 / 0         modes / automatic mode rotation\n"
      << "  Space                 pause or resume\n"
      << "  + - or Up Down        speed\n"
      << "  [ ] or Left Right     density\n"
      << "  Mouse wheel or z Z    zoom without clearing trails\n"
      << "  Left-drag             pan the visual center\n"
      << "  f F                   slower or faster circle trace\n"
      << "  b B                   dim or brighten trace\n"
      << "  v V                   lower or raise black trail floor\n"
      << "  < >                   trail length\n"
      << "  g / p / c             glyphs / palette / color\n"
      << "  o                     recenter view\n"
      << "  r or x                clear trails\n"
      << "  q or Esc              quit\n";
}

void printPresets() {
    std::cout
      << "asciiscope presets\n\n"
      << "  bloom-reel      classic glyphs | neon palette | attractor bloom\n"
      << "  neon-tunnel     dense glyphs   | neon palette | wide phasor tunnel\n"
      << "  particle-storm  block glyphs   | ember palette | high density particles\n"
      << "  ghost-spectral  wire glyphs    | ice palette | wide spectral ribbon\n\n"
      << "capture recipes\n\n"
      << "  asciiscope --preset particle-storm --reel\n"
      << "  asciiscope --preset ghost-spectral --reel --seconds 12\n"
      << "  asciiscope --tour --seconds 16 --hold 2\n"
      << "  asciiscope --preset neon-tunnel --canvas-only --warmup 90 --seconds 8 --fps 30\n";
}

struct Controls {
    bool running{ true };
    bool paused{ false };
    bool help{ true };
    bool color{ true };
    bool smoothColor{ true };
    int mode{ 4 };
    double speed{ 1.0 };
    double density{ 1.0 };
    double zoom{ 1.0 };
    double centerX{ 0.0 };
    double centerY{ 0.0 };
    double circleFrequencyHz{ 1.25 };
    double brightness{ 1.0 };
    int blackFloor{ 0 };
    int fade{ 2 };
    int glyphStyle{ 0 };
    int palette{ 0 };
    bool clearRequested{ false };
    bool mouseDragging{ false };
    int mouseX{};
    int mouseY{};
    std::string lastAdjustment{ "ready" };
    std::string inputStatus{ "input pending" };
};

void writeControlHelp(asciiscope::ConsoleRenderer& renderer,
                      const Controls& controls,
                      const std::optional<asciiscope::SignalStats>& stats) {
    const int width = renderer.width();
    if (width < 40) {
        return;
    }

    char line[128]{};
    const char* mode = controls.mode < 0 ? "auto" : (controls.mode == 0 ? "bloom" : (controls.mode == 1 ? "tunnel" : (controls.mode == 2 ? "particles" : (controls.mode == 3 ? "spectral" : "circle"))));
    const int x = 2;
    int y = 1;

    renderer.writeText(x, y++, "CONTROLS  h/? hide  q/esc quit", 13);
    renderer.writeText(x, y++, "1 bloom  2 tunnel  3 particles  4 spectral  5 circle  0 auto", 11);
    renderer.writeText(x, y++, "space pause  +/- or up/down speed  wheel/z/Z zoom", 11);
    renderer.writeText(x, y++, "left-click drag pans center  f/F circle frequency  b/B brightness", 11);
    renderer.writeText(x, y++, "v/V black floor  [/] or left/right density  </> trails", 11);
    renderer.writeText(x, y++, "g glyphs  p palette  c color  o center view  r/x clear trails", 11);

    std::snprintf(line,
                  sizeof(line),
                  "mode %-9s speed %.2fx  density %.2fx  zoom %.2fx  circle %.3fhz  bright %.2fx  black %d",
                  mode,
                  controls.speed,
                  controls.density,
                  controls.zoom,
                  controls.circleFrequencyHz,
                  controls.brightness,
                  controls.blackFloor);
    renderer.writeText(x, y++, line, 13);

    std::snprintf(line,
                  sizeof(line),
                  "center %.2f %.2f  trail %d  glyphs %-7.*s palette %-5.*s color %s  last %s",
                  controls.centerX,
                  controls.centerY,
                  controls.fade,
                  static_cast<int>(glyphStyleName(controls.glyphStyle).size()),
                  glyphStyleName(controls.glyphStyle).data(),
                  static_cast<int>(paletteName(controls.palette).size()),
                  paletteName(controls.palette).data(),
                  controls.color ? "on" : "off",
                  controls.lastAdjustment.c_str());
    renderer.writeText(x, y++, line, 13);

    if (stats.has_value()) {
        std::snprintf(line,
                      sizeof(line),
                      "signal rms %.2f  peak %.2f  min %.2f  max %.2f",
                      stats->rms,
                      stats->peak,
                      stats->min,
                      stats->max);
        renderer.writeText(x, y, line, 10);
    }
}

void writeControlHint(asciiscope::ConsoleRenderer& renderer) {
    if (renderer.width() >= 40) {
        renderer.writeText(2, 1, "h/? controls", 8);
    }
}

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

std::string_view tourCueName(int cue) {
    switch (cue % 4) {
    case 0:
        return "tour bloom";
    case 1:
        return "tour tunnel";
    case 2:
        return "tour particles";
    default:
        return "tour spectral";
    }
}

void applyTourCue(int cue, Controls& controls) {
    switch (cue % 4) {
    case 0:
        controls.mode = 0;
        controls.speed = 1.08;
        controls.density = 1.2;
        controls.zoom = 1.1;
        controls.fade = 2;
        controls.glyphStyle = 0;
        controls.palette = 0;
        break;
    case 1:
        controls.mode = 1;
        controls.speed = 1.25;
        controls.density = 1.15;
        controls.zoom = 1.18;
        controls.fade = 2;
        controls.glyphStyle = 1;
        controls.palette = 2;
        break;
    case 2:
        controls.mode = 2;
        controls.speed = 1.42;
        controls.density = 1.65;
        controls.zoom = 1.28;
        controls.fade = 3;
        controls.glyphStyle = 2;
        controls.palette = 1;
        break;
    default:
        controls.mode = 3;
        controls.speed = 0.88;
        controls.density = 1.3;
        controls.zoom = 1.0;
        controls.fade = 4;
        controls.glyphStyle = 3;
        controls.palette = 3;
        break;
    }

    controls.lastAdjustment = std::string(tourCueName(cue));
}

void handleKey(int key, Controls& controls) {
    if (key == 'Z') {
        controls.zoom = std::min(kMaxZoom, controls.zoom + 0.12);
        controls.lastAdjustment = "zoom";
        return;
    }
    if (key == 'F') {
        controls.circleFrequencyHz = std::min(kMaxCircleFrequency, controls.circleFrequencyHz * 1.12);
        controls.lastAdjustment = "circle frequency";
        return;
    }
    if (key == 'B') {
        controls.brightness = std::min(kMaxBrightness, controls.brightness + 0.05);
        controls.lastAdjustment = "brightness";
        return;
    }
    if (key == 'V') {
        controls.blackFloor = std::min(kMaxBlackFloor, controls.blackFloor + 1);
        controls.lastAdjustment = "black floor";
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
    case '5':
        controls.mode = 4;
        controls.lastAdjustment = "mode circle";
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
        controls.zoom = std::max(kMinZoom, controls.zoom - 0.12);
        controls.lastAdjustment = "zoom";
        break;
    case 'c':
        controls.color = !controls.color;
        controls.lastAdjustment = "color";
        break;
    case 'f':
        controls.circleFrequencyHz = std::max(kMinCircleFrequency, controls.circleFrequencyHz / 1.12);
        controls.lastAdjustment = "circle frequency";
        break;
    case 'b':
        controls.brightness = std::max(kMinBrightness, controls.brightness - 0.05);
        controls.lastAdjustment = "brightness";
        break;
    case 'v':
        controls.blackFloor = std::max(kMinBlackFloor, controls.blackFloor - 1);
        controls.lastAdjustment = "black floor";
        break;
    case 'g':
        controls.glyphStyle = (controls.glyphStyle + 1) % 4;
        controls.lastAdjustment = "glyphs";
        break;
    case 'p':
        controls.palette = (controls.palette + 1) % 4;
        controls.lastAdjustment = "palette";
        break;
    case 'o':
        controls.centerX = 0.0;
        controls.centerY = 0.0;
        controls.lastAdjustment = "center reset";
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
    const double ratio = direction > 0 ? 1.18 : 1.0 / 1.18;
    controls.zoom = std::clamp(controls.zoom * ratio, kMinZoom, kMaxZoom);
    controls.lastAdjustment = "mousewheel zoom";
}

void nudgeCenter(Controls& controls, int dx, int dy, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    controls.centerX = std::clamp(controls.centerX + (static_cast<double>(dx) * 2.0 / static_cast<double>(width)), -4.0, 4.0);
    controls.centerY = std::clamp(controls.centerY - (static_cast<double>(dy) * 2.0 / static_cast<double>(height)), -4.0, 4.0);
    controls.lastAdjustment = "mouse drag center";
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

void pollControls(Controls& controls,
                  int canvasWidth,
                  int canvasHeight
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
            } else if (record.EventType == MOUSE_EVENT) {
                const bool leftDown = (record.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
                const int mouseX = static_cast<int>(record.Event.MouseEvent.dwMousePosition.X);
                const int mouseY = static_cast<int>(record.Event.MouseEvent.dwMousePosition.Y);

                if (leftDown && controls.mouseDragging) {
                    nudgeCenter(controls, mouseX - controls.mouseX, mouseY - controls.mouseY, canvasWidth, canvasHeight);
                } else if (leftDown) {
                    controls.lastAdjustment = "mouse drag start";
                }

                controls.mouseDragging = leftDown;
                controls.mouseX = mouseX;
                controls.mouseY = mouseY;
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
    char footer[360]{};
    const char* mode = controls.mode < 0 ? "auto" : (controls.mode == 0 ? "bloom" : (controls.mode == 1 ? "tunnel" : (controls.mode == 2 ? "particles" : (controls.mode == 3 ? "spectral" : "circle"))));

    if (controls.help) {
        return "control help visible | h/? hide controls | wheel zoom | left-drag center | q quit";
    }

    if (stats.has_value()) {
        std::snprintf(
          footer,
          sizeof(footer),
          "%s | %s | %.2fx den %.2fx zoom %.2fx center %.2f %.2f circle %.3fhz bright %.2fx black %d trail %d | %s glyphs %s palette | sig rms %.2f pk %.2f min %.2f max %.2f | %s | last %s | h help",
          controls.paused ? "PAUSED" : "LIVE",
          mode,
          controls.speed,
          controls.density,
          controls.zoom,
          controls.centerX,
          controls.centerY,
          controls.circleFrequencyHz,
          controls.brightness,
          controls.blackFloor,
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
      "%s | mode %s | speed %.2fx | density %.2fx | zoom %.2fx | center %.2f %.2f | circle %.3fhz | bright %.2fx | black %d | trail %d | %s glyphs %s palette | color %s | %s | last %s | h help | q quit",
      controls.paused ? "PAUSED" : "LIVE",
      mode,
      controls.speed,
      controls.density,
      controls.zoom,
      controls.centerX,
      controls.centerY,
      controls.circleFrequencyHz,
      controls.brightness,
      controls.blackFloor,
      controls.fade,
      glyphStyleName(controls.glyphStyle).data(),
      paletteName(controls.palette).data(),
      controls.color ? "on" : "off",
      controls.inputStatus.c_str(),
      controls.lastAdjustment.c_str());
    return footer;
}

std::string_view modeLabel(int mode) {
    if (mode < 0) {
        return "auto";
    }

    switch (mode % 5) {
    case 0:
        return "bloom";
    case 1:
        return "tunnel";
    case 2:
        return "particles";
    case 3:
        return "spectral";
    default:
        return "circle";
    }
}

void printLaunchDescription(const Controls& controls,
                            int frameLimit,
                            int warmupFrames,
                            int fps,
                            int holdSeconds,
                            int width,
                            int height,
                            bool showChrome,
                            bool showHud,
                            std::uint32_t seed,
                            bool tourMode,
                            int tourSeconds) {
    std::cout
      << "asciiscope launch\n\n"
      << "  mode       " << modeLabel(controls.mode) << "\n"
      << "  fps        " << fps << "\n"
      << "  frames     " << (frameLimit == 0 ? std::string("unlimited") : std::to_string(frameLimit)) << "\n"
      << "  warmup     " << warmupFrames << "\n"
      << "  hold       " << holdSeconds << "s\n"
      << "  canvas     " << width << "x" << height << "\n"
      << "  tour       " << (tourMode ? "on" : "off") << "\n"
      << "  tour step  " << tourSeconds << "s\n"
      << "  speed      " << controls.speed << "\n"
      << "  density    " << controls.density << "\n"
      << "  zoom       " << controls.zoom << "\n"
      << "  center     " << controls.centerX << ", " << controls.centerY << "\n"
      << "  circle hz  " << controls.circleFrequencyHz << "\n"
      << "  brightness " << controls.brightness << "\n"
      << "  blackfloor " << controls.blackFloor << "\n"
      << "  trail      " << controls.fade << "\n"
      << "  glyphs     " << glyphStyleName(controls.glyphStyle) << "\n"
      << "  palette    " << paletteName(controls.palette) << "\n"
      << "  color mode " << (controls.smoothColor ? "truecolor" : "native") << "\n"
      << "  chrome     " << (showChrome ? "on" : "off") << "\n"
      << "  hud        " << (showHud && showChrome ? "on" : "off") << "\n"
      << "  color      " << (controls.color ? "on" : "off") << "\n"
      << "  seed       0x" << std::hex << std::uppercase << seed << std::dec << std::nouppercase << "\n";
}

} // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    if (hasArg(argc, argv, "--help") || hasArg(argc, argv, "-h")) {
        printHelp();
        return 0;
    }
    if (hasArg(argc, argv, "--list-presets")) {
        printPresets();
        return 0;
    }

    Controls controls;
    int frameLimit = 0;
    int width = 112;
    int height = 34;
    int warmupFrames = 0;
    std::uint32_t seed = 0xA5C115C0;
    const bool reelMode = hasArg(argc, argv, "--reel");
    const bool tourMode = hasArg(argc, argv, "--tour");
    bool showHud = !hasArg(argc, argv, "--no-hud");
    bool showChrome = !hasArg(argc, argv, "--canvas-only") && !reelMode && !tourMode;
    std::string title = "ASCIISCOPE / SOEMDSP";

    if (const auto titleArg = argValue(argc, argv, "--title")) {
        title = std::string(*titleArg);
    }

    if (const auto preset = argValue(argc, argv, "--preset")) {
        applyPreset(*preset, controls, frameLimit, width, height);
    }

    controls.color = !hasArg(argc, argv, "--no-color");
    controls.smoothColor = !hasArg(argc, argv, "--native-color");
    controls.speed = boundedDoubleOption(argc, argv, "--speed", controls.speed, 0.15, 4.0);
    controls.density = boundedDoubleOption(argc, argv, "--density", controls.density, 0.25, 2.0);
    controls.zoom = boundedDoubleOption(argc, argv, "--zoom", controls.zoom, kMinZoom, kMaxZoom);
    controls.circleFrequencyHz = boundedDoubleOption(argc, argv, "--circle-hz", controls.circleFrequencyHz, kMinCircleFrequency, kMaxCircleFrequency);
    controls.brightness = boundedDoubleOption(argc, argv, "--brightness", controls.brightness, kMinBrightness, kMaxBrightness);
    controls.blackFloor = boundedIntOption(argc, argv, "--black-floor", controls.blackFloor, kMinBlackFloor, kMaxBlackFloor);
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
    const int fps = boundedIntOption(argc, argv, "--fps", (reelMode || tourMode) ? 30 : 60, 1, 240);
    const int tourSeconds = boundedIntOption(argc, argv, "--tour-seconds", 4, 1, 30);
    if (reelMode) {
        frameLimit = fps * 8;
        controls.lastAdjustment = "reel";
    }
    if (tourMode) {
        width = 128;
        height = 36;
        frameLimit = fps * tourSeconds * 4;
        applyTourCue(0, controls);
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
    warmupFrames = boundedIntOption(argc, argv, "--warmup", (reelMode || tourMode) ? 90 : 0, 0, 3600);
    if (warmupFrames > 0) {
        controls.lastAdjustment = "warmup cli";
    }
    if (const auto seedArg = seedValue(argc, argv, "--seed")) {
        seed = *seedArg;
        controls.lastAdjustment = "seed cli";
    }
    const int holdSeconds = boundedIntOption(argc, argv, "--hold", 0, 0, 120);
    const auto frameDelay = std::chrono::milliseconds(std::max(1, 1000 / fps));
    width = boundedIntOption(argc, argv, "--width", width, 40, 220);
    height = boundedIntOption(argc, argv, "--height", height, 16, 80);

    if (hasArg(argc, argv, "--describe")) {
        printLaunchDescription(controls, frameLimit, warmupFrames, fps, holdSeconds, width, height, showChrome, showHud, seed, tourMode, tourSeconds);
        return 0;
    }

    asciiscope::ConsoleRenderer renderer({ .width = width, .height = height, .maxAge = 31, .color = controls.color, .smoothColor = controls.smoothColor, .blackFloor = controls.blackFloor });
    asciiscope::DemoSignalInput signalInput(seed);
    asciiscope::AnimationScene scene(renderer);

#ifdef _WIN32
    const auto inputState = configureConsoleInput();
#endif

    std::optional<asciiscope::SignalStats> latestStats;
    double visualFrame = 0.0;
    int presentedFrames = 0;
    int activeTourCue = -1;

    auto updateTourCue = [&]() {
        if (!tourMode) {
            return;
        }

        const int cueFrames = std::max(1, fps * tourSeconds);
        const int cue = (presentedFrames / cueFrames) % 4;
        if (cue != activeTourCue) {
            applyTourCue(cue, controls);
            activeTourCue = cue;
        }
    };

    auto drawNextFrame = [&]() {
        const int frame = static_cast<int>(visualFrame);
        const auto sampleCount = static_cast<std::size_t>(std::clamp(5400.0 * controls.density, 1200.0, 10800.0));
        auto signalFrame = signalInput.nextFrame(static_cast<std::uint64_t>(frame), sampleCount);
        if (const auto* source = signalFrame.findSource("main")) {
            latestStats = source->stats;
        }
        asciiscope::SceneSettings settings{
            .mode = controls.mode,
            .density = controls.density,
            .zoom = controls.zoom,
            .centerX = controls.centerX,
            .centerY = controls.centerY,
            .circleFrequencyHz = controls.circleFrequencyHz,
            .brightness = controls.brightness,
            .fade = controls.fade
        };
        scene.draw(signalFrame, settings);
        visualFrame += controls.speed;
        return frame;
    };

    updateTourCue();
    renderer.setColor(controls.color);
    renderer.setSmoothColor(controls.smoothColor);
    renderer.setChrome(showChrome);
    renderer.setGlyphRamp(glyphRampForStyle(controls.glyphStyle));
    renderer.setPalette(controls.palette);
    renderer.setBlackFloor(controls.blackFloor);
    for (int i = 0; i < warmupFrames; ++i) {
        drawNextFrame();
    }

    while (controls.running && (frameLimit == 0 || presentedFrames < frameLimit)) {
        pollControls(controls,
                     renderer.width(),
                     renderer.height()
#ifdef _WIN32
                     ,
                     inputState
#endif
        );
        updateTourCue();
        renderer.setColor(controls.color);
        renderer.setSmoothColor(controls.smoothColor);
        renderer.setChrome(showChrome);
        renderer.setGlyphRamp(glyphRampForStyle(controls.glyphStyle));
        renderer.setPalette(controls.palette);
        renderer.setBlackFloor(controls.blackFloor);

        if (controls.clearRequested) {
            renderer.clear();
            controls.clearRequested = false;
        }

        int frame = static_cast<int>(visualFrame);

        if (!controls.paused) {
            frame = drawNextFrame();
        }

        if (showHud && showChrome) {
            if (controls.help) {
                writeControlHelp(renderer, controls, latestStats);
            } else {
                writeControlHint(renderer);
            }
        }

        const std::string footer = showHud && showChrome ? footerFor(controls, latestStats) : std::string{};
        renderer.present(title, scene.modeName(frame, controls.mode), footer, frame);
        std::this_thread::sleep_for(frameDelay);
        ++presentedFrames;
    }

    if (holdSeconds > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(holdSeconds));
    }

#ifdef _WIN32
    restoreConsoleInput(inputState);
#endif
    renderer.restoreTerminal();
    return 0;
}

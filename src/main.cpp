#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
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
    bool clearRequested{ false };
    std::string lastAdjustment{ "ready" };
    std::string inputStatus{ "input pending" };
};

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
        return "1/2/3/4 modes  0 auto  space pause  +/- speed  wheel/z/Z zoom  [/]/arrows density  </> trails  c color  r clear  q quit";
    }

    if (stats.has_value()) {
        std::snprintf(
          footer,
          sizeof(footer),
          "%s | %s | %.2fx den %.2fx zoom %.2fx trail %d | sig rms %.2f pk %.2f min %.2f max %.2f | %s | last %s | h help",
          controls.paused ? "PAUSED" : "LIVE",
          mode,
          controls.speed,
          controls.density,
          controls.zoom,
          controls.fade,
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
      "%s | mode %s | speed %.2fx | density %.2fx | zoom %.2fx | trail %d | color %s | %s | last %s | h help | q quit",
      controls.paused ? "PAUSED" : "LIVE",
      mode,
      controls.speed,
      controls.density,
      controls.zoom,
      controls.fade,
      controls.color ? "on" : "off",
      controls.inputStatus.c_str(),
      controls.lastAdjustment.c_str());
    return footer;
}

} // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    Controls controls;
    controls.color = !hasArg(argc, argv, "--no-color");
    if (const auto modeArg = argValue(argc, argv, "--mode")) {
        if (const auto mode = modeFromName(*modeArg)) {
            controls.mode = *mode;
            controls.lastAdjustment = "mode cli";
        }
    }
    int frameLimit = hasArg(argc, argv, "--once") ? 90 : 0;
    if (const auto frames = positiveIntValue(argc, argv, "--frames")) {
        frameLimit = *frames;
    }

    asciiscope::ConsoleRenderer renderer({ .width = 112, .height = 34, .maxAge = 13, .color = controls.color });
    asciiscope::DemoSignalInput signalInput;
    asciiscope::AnimationScene scene(renderer);

#ifdef _WIN32
    const auto inputState = configureConsoleInput();
#endif

    std::optional<asciiscope::SignalStats> latestStats;
    double visualFrame = 0.0;
    int presentedFrames = 0;

    while (controls.running && (frameLimit == 0 || presentedFrames < frameLimit)) {
        pollControls(controls
#ifdef _WIN32
                     ,
                     inputState
#endif
        );
        renderer.setColor(controls.color);

        if (controls.clearRequested) {
            renderer.clear();
            controls.clearRequested = false;
        }

        const int frame = static_cast<int>(visualFrame);
        const auto sampleCount = static_cast<std::size_t>(std::clamp(5400.0 * controls.density, 1200.0, 10800.0));

        if (!controls.paused) {
            auto signalFrame = signalInput.nextFrame(static_cast<std::uint64_t>(frame), sampleCount);
            if (const auto* source = signalFrame.findSource("main")) {
                latestStats = source->stats;
            }
            asciiscope::SceneSettings settings{ .mode = controls.mode, .density = controls.density, .zoom = controls.zoom, .fade = controls.fade };
            scene.draw(signalFrame, settings);
            visualFrame += controls.speed;
        }

        renderer.present("ASCIISCOPE / SOEMDSP", scene.modeName(frame, controls.mode), footerFor(controls, latestStats), frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        ++presentedFrames;
    }

#ifdef _WIN32
    restoreConsoleInput(inputState);
#endif
    renderer.restoreTerminal();
    return 0;
}

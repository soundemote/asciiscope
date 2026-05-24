#include <asciiscope/ConsoleRenderer.hpp>

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace asciiscope {

namespace {

#ifdef _WIN32
WORD attributeForAge(std::uint8_t age, int maxAge, bool color) {
    if (!color) {
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    if (age > maxAge * 3 / 4) {
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    if (age > maxAge / 2) {
        return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    if (age > maxAge / 4) {
        return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
}

void writeTextToBuffer(std::vector<CHAR_INFO>& buffer, int width, int x, int y, std::string_view text, WORD attribute) {
    if (y < 0) {
        return;
    }

    for (std::size_t i = 0; i < text.size(); ++i) {
        const int px = x + static_cast<int>(i);
        if (px < 0 || px >= width) {
            continue;
        }

        const auto offset = static_cast<std::size_t>(y * width + px);
        if (offset >= buffer.size()) {
            continue;
        }

        buffer[offset].Char.AsciiChar = text[i];
        buffer[offset].Attributes = attribute;
    }
}
#endif

} // namespace

ConsoleRenderer::ConsoleRenderer(Config config)
  : config_(config)
  , cells_(static_cast<std::size_t>(config.width * config.height), 0) {}

void ConsoleRenderer::beginFrame() {
    if (cells_.size() != static_cast<std::size_t>(config_.width * config_.height)) {
        cells_.assign(static_cast<std::size_t>(config_.width * config_.height), 0);
    }
}

void ConsoleRenderer::clear() {
    std::fill(cells_.begin(), cells_.end(), 0);
}

void ConsoleRenderer::fade(int amount) {
    for (auto& cell : cells_) {
        cell = static_cast<std::uint8_t>(std::max(0, static_cast<int>(cell) - amount));
    }
}

void ConsoleRenderer::plot(double x, double y, double intensity) {
    const auto px = static_cast<int>((x * 0.5 + 0.5) * static_cast<double>(config_.width - 1));
    const auto py = static_cast<int>((0.5 - y * 0.5) * static_cast<double>(config_.height - 1));

    if (px < 0 || px >= config_.width || py < 0 || py >= config_.height) {
        return;
    }

    const auto age = static_cast<std::uint8_t>(
      std::clamp(static_cast<int>(static_cast<double>(config_.maxAge) * intensity), 1, config_.maxAge));
    auto& cell = cells_[static_cast<std::size_t>(index(px, py))];
    cell = std::max(cell, age);
}

void ConsoleRenderer::writeText(int x, int y, std::string_view text, std::uint8_t age) {
    if (y < 0 || y >= config_.height) {
        return;
    }

    for (std::size_t i = 0; i < text.size(); ++i) {
        const int px = x + static_cast<int>(i);
        if (px >= 0 && px < config_.width && text[i] != ' ') {
            cells_[static_cast<std::size_t>(index(px, y))] = age;
        }
    }
}

std::string ConsoleRenderer::render(std::string_view title, std::string_view mode, std::string_view footer, int frame) const {
    std::ostringstream out;
    out << "\x1b[?25l\x1b[H";
    out << "\x1b[1;37m" << title << "\x1b[0m"
        << "  \x1b[36m" << mode << "\x1b[0m"
        << "  \x1b[90mframe " << std::setw(5) << frame << "\x1b[0m\n";

    for (int x = 0; x < config_.width + 2; ++x) {
        out << (x == 0 || x == config_.width + 1 ? '+' : '-');
    }
    out << '\n';

    for (int y = 0; y < config_.height; ++y) {
        out << '|';
        for (int x = 0; x < config_.width; ++x) {
            const auto age = cells_[static_cast<std::size_t>(index(x, y))];
            if (age == 0) {
                out << ' ';
            } else if (config_.color) {
                out << colorFor(age) << glyphFor(age) << "\x1b[0m";
            } else {
                out << glyphFor(age);
            }
        }
        out << "|\n";
    }

    for (int x = 0; x < config_.width + 2; ++x) {
        out << (x == 0 || x == config_.width + 1 ? '+' : '-');
    }
    out << "\n\x1b[90m" << footer << "\x1b[0m\n";
    return out.str();
}

void ConsoleRenderer::present(std::string_view title, std::string_view mode, std::string_view footer, int frame) const {
#ifdef _WIN32
    auto output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE) {
        std::cout << render(title, mode, footer, frame) << std::flush;
        return;
    }

    CONSOLE_CURSOR_INFO cursorInfo{};
    if (GetConsoleCursorInfo(output, &cursorInfo)) {
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(output, &cursorInfo);
    }

    const int outWidth = frameWidth();
    const int outHeight = frameHeight();
    std::vector<CHAR_INFO> buffer(static_cast<std::size_t>(outWidth * outHeight));

    for (auto& cell : buffer) {
        cell.Char.AsciiChar = ' ';
        cell.Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    char header[160]{};
    std::snprintf(header, sizeof(header), "%.*s  %.*s  frame %5d",
                  static_cast<int>(title.size()), title.data(),
                  static_cast<int>(mode.size()), mode.data(),
                  frame);
    writeTextToBuffer(buffer, outWidth, 0, 0, header, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    for (int x = 0; x < outWidth; ++x) {
        const bool corner = x == 0 || x == outWidth - 1;
        buffer[static_cast<std::size_t>(x + outWidth)].Char.AsciiChar = corner ? '+' : '-';
        buffer[static_cast<std::size_t>(x + outWidth)].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        const auto bottom = static_cast<std::size_t>((config_.height + 2) * outWidth + x);
        buffer[bottom].Char.AsciiChar = corner ? '+' : '-';
        buffer[bottom].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    for (int y = 0; y < config_.height; ++y) {
        const int outY = y + 2;
        buffer[static_cast<std::size_t>(outY * outWidth)].Char.AsciiChar = '|';
        buffer[static_cast<std::size_t>(outY * outWidth)].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        buffer[static_cast<std::size_t>(outY * outWidth + outWidth - 1)].Char.AsciiChar = '|';
        buffer[static_cast<std::size_t>(outY * outWidth + outWidth - 1)].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        for (int x = 0; x < config_.width; ++x) {
            const auto age = cells_[static_cast<std::size_t>(index(x, y))];
            const auto outIndex = static_cast<std::size_t>(outY * outWidth + x + 1);
            buffer[outIndex].Char.AsciiChar = age == 0 ? ' ' : glyphFor(age);
            buffer[outIndex].Attributes = attributeForAge(age, config_.maxAge, config_.color);
        }
    }

    writeTextToBuffer(buffer, outWidth, 0, config_.height + 3, footer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    COORD bufferSize{ static_cast<SHORT>(outWidth), static_cast<SHORT>(outHeight) };
    COORD bufferCoord{ 0, 0 };
    SMALL_RECT writeRegion{ 0, 0, static_cast<SHORT>(outWidth - 1), static_cast<SHORT>(outHeight - 1) };

    if (!WriteConsoleOutputA(output, buffer.data(), bufferSize, bufferCoord, &writeRegion)) {
        std::cout << render(title, mode, footer, frame) << std::flush;
    }
#else
    std::cout << render(title, mode, footer, frame) << std::flush;
#endif
}

void ConsoleRenderer::restoreTerminal() const {
#ifdef _WIN32
    auto output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cursorInfo{};
        if (GetConsoleCursorInfo(output, &cursorInfo)) {
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(output, &cursorInfo);
        }
    }
#endif
    std::cout << "\x1b[?25h\x1b[0m\n";
}

int ConsoleRenderer::index(int x, int y) const noexcept {
    return y * config_.width + x;
}

char ConsoleRenderer::glyphFor(std::uint8_t age) const noexcept {
    constexpr std::string_view ramp = " .:-=+*#%@";
    const auto scaled = std::clamp<int>((static_cast<int>(age) * static_cast<int>(ramp.size() - 1)) / config_.maxAge, 0, static_cast<int>(ramp.size() - 1));
    return ramp[static_cast<std::size_t>(scaled)];
}

std::string ConsoleRenderer::colorFor(std::uint8_t age) const {
    if (age > config_.maxAge * 3 / 4) {
        return "\x1b[1;97m";
    }
    if (age > config_.maxAge / 2) {
        return "\x1b[1;36m";
    }
    if (age > config_.maxAge / 4) {
        return "\x1b[0;35m";
    }
    return "\x1b[0;34m";
}

} // namespace asciiscope

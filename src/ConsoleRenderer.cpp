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
WORD attributeForAge(std::uint8_t age, int maxAge, bool color, int palette) {
    if (!color) {
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    const int band = age > maxAge * 3 / 4 ? 3 : (age > maxAge / 2 ? 2 : (age > maxAge / 4 ? 1 : 0));

    if (palette % 4 == 1) {
        constexpr WORD ember[] = {
            FOREGROUND_RED,
            FOREGROUND_RED | FOREGROUND_INTENSITY,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        };
        return ember[band];
    }

    if (palette % 4 == 2) {
        constexpr WORD acid[] = {
            FOREGROUND_GREEN,
            FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        };
        return acid[band];
    }

    if (palette % 4 == 3) {
        constexpr WORD ice[] = {
            FOREGROUND_BLUE,
            FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        };
        return ice[band];
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
  , cells_(static_cast<std::size_t>(config.width * config.height), 0)
  , textCells_(static_cast<std::size_t>(config.width * config.height), ' ')
  , textAges_(static_cast<std::size_t>(config.width * config.height), 0) {}

void ConsoleRenderer::beginFrame() {
    if (cells_.size() != static_cast<std::size_t>(config_.width * config_.height)) {
        cells_.assign(static_cast<std::size_t>(config_.width * config_.height), 0);
    }
    if (textCells_.size() != static_cast<std::size_t>(config_.width * config_.height)) {
        textCells_.assign(static_cast<std::size_t>(config_.width * config_.height), ' ');
        textAges_.assign(static_cast<std::size_t>(config_.width * config_.height), 0);
    }

    clearText();
}

void ConsoleRenderer::clear() {
    std::fill(cells_.begin(), cells_.end(), 0);
    clearText();
}

void ConsoleRenderer::clearText() {
    std::fill(textCells_.begin(), textCells_.end(), ' ');
    std::fill(textAges_.begin(), textAges_.end(), 0);
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
        if (px >= 0 && px < config_.width) {
            const auto cellIndex = static_cast<std::size_t>(index(px, y));
            textCells_[cellIndex] = text[i];
            textAges_[cellIndex] = age;
        }
    }
}

std::string ConsoleRenderer::render(std::string_view title, std::string_view mode, std::string_view footer, int frame) const {
    std::ostringstream out;
    out << "\x1b[?25l\x1b[H";

    if (config_.chrome) {
        out << "\x1b[1;37m" << title << "\x1b[0m"
            << "  \x1b[36m" << mode << "\x1b[0m"
            << "  \x1b[90mframe " << std::setw(5) << frame << "\x1b[0m\n";

        for (int x = 0; x < config_.width + 2; ++x) {
            out << (x == 0 || x == config_.width + 1 ? '+' : '-');
        }
        out << '\n';
    }

    for (int y = 0; y < config_.height; ++y) {
        if (config_.chrome) {
            out << '|';
        }
        for (int x = 0; x < config_.width; ++x) {
            const auto cellIndex = static_cast<std::size_t>(index(x, y));
            const auto text = textCells_[cellIndex];
            const auto age = text == ' ' ? cells_[cellIndex] : textAges_[cellIndex];
            if (text != ' ') {
                if (config_.color) {
                    out << colorFor(age) << text << "\x1b[0m";
                } else {
                    out << text;
                }
            } else if (age == 0) {
                out << ' ';
            } else if (config_.color) {
                out << colorFor(age) << glyphFor(age) << "\x1b[0m";
            } else {
                out << glyphFor(age);
            }
        }
        if (config_.chrome) {
            out << '|';
        }
        out << '\n';
    }

    if (config_.chrome) {
        for (int x = 0; x < config_.width + 2; ++x) {
            out << (x == 0 || x == config_.width + 1 ? '+' : '-');
        }
        out << "\n\x1b[90m" << footer << "\x1b[0m\n";
    }
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

    if (config_.chrome) {
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
    }

    for (int y = 0; y < config_.height; ++y) {
        const int outY = config_.chrome ? y + 2 : y;
        const int outX = config_.chrome ? 1 : 0;

        if (config_.chrome) {
            buffer[static_cast<std::size_t>(outY * outWidth)].Char.AsciiChar = '|';
            buffer[static_cast<std::size_t>(outY * outWidth)].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            buffer[static_cast<std::size_t>(outY * outWidth + outWidth - 1)].Char.AsciiChar = '|';
            buffer[static_cast<std::size_t>(outY * outWidth + outWidth - 1)].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }

        for (int x = 0; x < config_.width; ++x) {
            const auto cellIndex = static_cast<std::size_t>(index(x, y));
            const auto text = textCells_[cellIndex];
            const auto age = text == ' ' ? cells_[cellIndex] : textAges_[cellIndex];
            const auto outIndex = static_cast<std::size_t>(outY * outWidth + x + outX);
            buffer[outIndex].Char.AsciiChar = text != ' ' ? text : (age == 0 ? ' ' : glyphFor(age));
            buffer[outIndex].Attributes = attributeForAge(age, config_.maxAge, config_.color, config_.palette);
        }
    }

    if (config_.chrome) {
        writeTextToBuffer(buffer, outWidth, 0, config_.height + 3, footer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

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

void ConsoleRenderer::setGlyphRamp(std::string_view glyphRamp) {
    if (!glyphRamp.empty()) {
        config_.glyphRamp = std::string(glyphRamp);
    }
}

int ConsoleRenderer::index(int x, int y) const noexcept {
    return y * config_.width + x;
}

char ConsoleRenderer::glyphFor(std::uint8_t age) const noexcept {
    const auto& ramp = config_.glyphRamp;
    const auto scaled = std::clamp<int>((static_cast<int>(age) * static_cast<int>(ramp.size() - 1)) / config_.maxAge, 0, static_cast<int>(ramp.size() - 1));
    return ramp[static_cast<std::size_t>(scaled)];
}

std::string ConsoleRenderer::colorFor(std::uint8_t age) const {
    const int band = age > config_.maxAge * 3 / 4 ? 3 : (age > config_.maxAge / 2 ? 2 : (age > config_.maxAge / 4 ? 1 : 0));

    if (config_.palette % 4 == 1) {
        constexpr std::string_view ember[] = { "\x1b[0;31m", "\x1b[1;31m", "\x1b[1;33m", "\x1b[1;97m" };
        return std::string(ember[band]);
    }

    if (config_.palette % 4 == 2) {
        constexpr std::string_view acid[] = { "\x1b[0;32m", "\x1b[1;32m", "\x1b[1;33m", "\x1b[1;97m" };
        return std::string(acid[band]);
    }

    if (config_.palette % 4 == 3) {
        constexpr std::string_view ice[] = { "\x1b[0;34m", "\x1b[1;36m", "\x1b[1;37m", "\x1b[1;97m" };
        return std::string(ice[band]);
    }

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

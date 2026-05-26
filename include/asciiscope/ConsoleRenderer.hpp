#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace asciiscope {

class ConsoleRenderer {
  public:
    struct Config {
        int width{ 112 };
        int height{ 34 };
        int maxAge{ 13 };
        bool color{ true };
        bool smoothColor{ true };
        bool chrome{ true };
        std::string glyphRamp{ " .:-=+*#%@" };
        int palette{ 0 };
        int blackFloor{ 0 };
    };

    explicit ConsoleRenderer(Config config = {});

    void beginFrame();
    void clear();
    void clearText();
    void fade(int amount = 1);
    void plot(double x, double y, double intensity = 1.0);
    void writeText(int x, int y, std::string_view text, std::uint8_t age);
    [[nodiscard]] std::string render(std::string_view title, std::string_view mode, std::string_view footer, int frame) const;
    void present(std::string_view title, std::string_view mode, std::string_view footer, int frame) const;
    void restoreTerminal() const;
    void setColor(bool enabled) noexcept { config_.color = enabled; }
    void setSmoothColor(bool enabled);
    void setChrome(bool enabled) noexcept { config_.chrome = enabled; }
    void setGlyphRamp(std::string_view glyphRamp);
    void setMaxAge(int maxAge);
    void setPalette(int palette);
    void setBlackFloor(int blackFloor) noexcept { config_.blackFloor = blackFloor; }

    [[nodiscard]] int width() const noexcept { return config_.width; }
    [[nodiscard]] int height() const noexcept { return config_.height; }
    [[nodiscard]] bool color() const noexcept { return config_.color; }

  private:
    [[nodiscard]] int index(int x, int y) const noexcept;
    [[nodiscard]] char glyphFor(std::uint8_t age) const noexcept;
    [[nodiscard]] std::string_view colorFor(std::uint8_t age) const;
    void rebuildColorCache();
    [[nodiscard]] int frameWidth() const noexcept { return config_.chrome ? config_.width + 2 : config_.width; }
    [[nodiscard]] int frameHeight() const noexcept { return config_.chrome ? config_.height + 4 : config_.height; }

    Config config_;
    std::vector<std::uint8_t> cells_;
    std::vector<char> textCells_;
    std::vector<std::uint8_t> textAges_;
    std::vector<std::string> colorCache_;
};

} // namespace asciiscope

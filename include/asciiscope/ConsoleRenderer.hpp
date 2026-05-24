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
        std::string glyphRamp{ " .:-=+*#%@" };
        int palette{ 0 };
    };

    explicit ConsoleRenderer(Config config = {});

    void beginFrame();
    void clear();
    void fade(int amount = 1);
    void plot(double x, double y, double intensity = 1.0);
    void writeText(int x, int y, std::string_view text, std::uint8_t age);
    [[nodiscard]] std::string render(std::string_view title, std::string_view mode, std::string_view footer, int frame) const;
    void present(std::string_view title, std::string_view mode, std::string_view footer, int frame) const;
    void restoreTerminal() const;
    void setColor(bool enabled) noexcept { config_.color = enabled; }
    void setGlyphRamp(std::string_view glyphRamp);
    void setPalette(int palette) noexcept { config_.palette = palette; }

    [[nodiscard]] int width() const noexcept { return config_.width; }
    [[nodiscard]] int height() const noexcept { return config_.height; }
    [[nodiscard]] bool color() const noexcept { return config_.color; }

  private:
    [[nodiscard]] int index(int x, int y) const noexcept;
    [[nodiscard]] char glyphFor(std::uint8_t age) const noexcept;
    [[nodiscard]] std::string colorFor(std::uint8_t age) const;
    [[nodiscard]] int frameWidth() const noexcept { return config_.width + 2; }
    [[nodiscard]] int frameHeight() const noexcept { return config_.height + 4; }

    Config config_;
    std::vector<std::uint8_t> cells_;
};

} // namespace asciiscope

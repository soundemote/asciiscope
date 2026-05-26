#pragma once

#include <string_view>

#include <asciiscope/ConsoleRenderer.hpp>
#include <asciiscope/SignalInput.hpp>

namespace asciiscope {

struct SceneSettings {
    int mode{ -1 };
    double density{ 1.0 };
    double zoom{ 1.0 };
    double centerX{ 0.0 };
    double centerY{ 0.0 };
    double circleFrequencyHz{ 1.25 };
    double cellAspect{ 2.0 };
    double brightness{ 1.0 };
    int fade{ 2 };
};

class AnimationScene {
  public:
    explicit AnimationScene(ConsoleRenderer& renderer);

    void draw(const SignalFrame& frame, const SceneSettings& settings);
    [[nodiscard]] std::string_view modeName(int frame, int overrideMode = -1) const noexcept;

  private:
    void drawAttractorBloom(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings);
    void drawWaveTunnel(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings);
    void drawParticleField(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings);
    void drawSpectralRibbon(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings);
    void drawSinCosCircle(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings);
    [[nodiscard]] int activeMode(int frame, int overrideMode) const noexcept;

    ConsoleRenderer& renderer_;
};

} // namespace asciiscope

#include <asciiscope/AnimationScene.hpp>

#include <algorithm>
#include <cmath>

#include <soemdsp/semath.hpp>

namespace asciiscope {

namespace {

const SignalSample& sampleAt(const SignalSource& source, std::size_t index) {
    return source.samples[index % source.samples.size()];
}

} // namespace

AnimationScene::AnimationScene(ConsoleRenderer& renderer)
  : renderer_(renderer) {}

void AnimationScene::draw(const SignalFrame& frame, const SceneSettings& settings) {
    renderer_.beginFrame();
    renderer_.fade(settings.fade);

    const auto* source = frame.findSource("main");
    if (source == nullptr || source->samples.empty()) {
        return;
    }

    const auto frameIndex = static_cast<int>(frame.frameIndex);

    switch (activeMode(frameIndex, settings.mode)) {
    case 0:
        drawAttractorBloom(frame, *source, settings);
        break;
    case 1:
        drawWaveTunnel(frame, *source, settings);
        break;
    default:
        drawParticleField(frame, *source, settings);
        break;
    }
}

std::string_view AnimationScene::modeName(int frame, int overrideMode) const noexcept {
    switch (activeMode(frame, overrideMode)) {
    case 0:
        return "multi-sprott bloom";
    case 1:
        return "phasor wave tunnel";
    default:
        return "pluck/noise particle field";
    }
}

int AnimationScene::activeMode(int frame, int overrideMode) const noexcept {
    if (overrideMode >= 0) {
        return overrideMode % 3;
    }

    return (frame / 260) % 3;
}

void AnimationScene::drawAttractorBloom(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings) {
    const double spin = frame.timeSeconds * 0.18;
    const double cs = std::cos(spin);
    const double sn = std::sin(spin);

    for (int i = 0; i < 5200; ++i) {
        const auto& s = sampleAt(source, static_cast<std::size_t>(i));
        const double x = (s.x * cs + s.z * sn) * 0.28;
        const double y = (s.y + s.z * 0.11 * s.lfo) * 0.22;
        renderer_.plot(x * settings.zoom, y * settings.zoom, 0.36 + s.pulse * 0.95);
    }
}

void AnimationScene::drawWaveTunnel(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings) {
    const double framePhase = frame.timeSeconds * 0.36;

    for (int ring = 0; ring < 46; ++ring) {
        const double depth = static_cast<double>(ring) / 46.0;
        const double radius = 0.05 + depth * 0.92;
        const double twist = framePhase + depth * 0.48;

        for (int spoke = 0; spoke < 96; ++spoke) {
            const auto& s = sampleAt(source, static_cast<std::size_t>(ring * 96 + spoke));
            const double p = static_cast<double>(spoke) / 96.0;
            const double wave = std::sin((p * 5.0 + twist + s.pulse * 0.25) * soemdsp::constant::kTAU);
            const double angle = p * soemdsp::constant::kTAU + twist + wave * 0.22;
            const double squeeze = 1.0 / (1.0 + depth * 0.8);
            const double x = std::cos(angle) * radius * squeeze;
            const double y = std::sin(angle) * radius * 0.48 + wave * 0.08 + s.lfo * 0.04;
            renderer_.plot(x * settings.zoom, y * settings.zoom, 0.20 + (1.0 - depth) * 0.9 + s.pulse * 0.6);
        }
    }
}

void AnimationScene::drawParticleField(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings) {
    const double drift = frame.timeSeconds * 0.66;

    for (int i = 0; i < 4300; ++i) {
        const auto& s = sampleAt(source, static_cast<std::size_t>(i));
        const double id = static_cast<double>(i);
        const double lane = std::fmod(id * soemdsp::constant::k1zPHI + s.phase + s.noise * 0.008, 1.0);
        const double angle = lane * soemdsp::constant::kTAU + drift * (0.25 + s.pulse);
        const double radial = std::clamp(0.18 + std::abs(s.z) * 0.12 + s.pulse * 0.45, 0.0, 1.05);
        const double shear = std::sin((lane * 3.0 + drift) * soemdsp::constant::kTAU) * 0.26;
        const double x = std::cos(angle) * radial + shear * 0.35;
        const double y = std::sin(angle * 0.73 + s.lfo) * radial * 0.55 + s.noise * 0.035;
        renderer_.plot(x * settings.zoom, y * settings.zoom, 0.24 + s.pulse + std::abs(s.noise) * 0.35);
    }
}

} // namespace asciiscope

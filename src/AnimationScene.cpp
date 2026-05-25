#include <asciiscope/AnimationScene.hpp>

#include <algorithm>
#include <cmath>

#include <soemdsp/semath.hpp>

namespace asciiscope {

namespace {

const SignalSample& sampleAt(const SignalSource& source, std::size_t index) {
    return source.samples[index % source.samples.size()];
}

void plotView(ConsoleRenderer& renderer, double x, double y, double intensity, const SceneSettings& settings) {
    renderer.plot((x * settings.zoom) + settings.centerX, (y * settings.zoom) + settings.centerY, intensity);
}

int gridX(ConsoleRenderer& renderer, double x, const SceneSettings& settings) {
    const double px = (((x * settings.zoom) + settings.centerX) * 0.5 + 0.5) * static_cast<double>(renderer.width() - 1);
    return static_cast<int>(std::round(px));
}

int gridY(ConsoleRenderer& renderer, double y, const SceneSettings& settings) {
    const double py = (0.5 - (((y * settings.zoom) + settings.centerY) * 0.5)) * static_cast<double>(renderer.height() - 1);
    return static_cast<int>(std::round(py));
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
    case 2:
        drawParticleField(frame, *source, settings);
        break;
    case 3:
        drawSpectralRibbon(frame, *source, settings);
        break;
    default:
        drawSinCosCircle(frame, *source, settings);
        break;
    }
}

std::string_view AnimationScene::modeName(int frame, int overrideMode) const noexcept {
    switch (activeMode(frame, overrideMode)) {
    case 0:
        return "multi-sprott bloom";
    case 1:
        return "phasor wave tunnel";
    case 2:
        return "pluck/noise particle field";
    case 3:
        return "spectral ribbon";
    default:
        return "sincos ascii circle";
    }
}

int AnimationScene::activeMode(int frame, int overrideMode) const noexcept {
    if (overrideMode >= 0) {
        return overrideMode % 5;
    }

    return (frame / 240) % 5;
}

void AnimationScene::drawAttractorBloom(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings) {
    const double spin = frame.timeSeconds * 0.18;
    const double cs = std::cos(spin);
    const double sn = std::sin(spin);

    for (int i = 0; i < 5200; ++i) {
        const auto& s = sampleAt(source, static_cast<std::size_t>(i));
        const double x = (s.x * cs + s.z * sn) * 0.28;
        const double y = (s.y + s.z * 0.11 * s.lfo) * 0.22;
        plotView(renderer_, x, y, 0.36 + s.pulse * 0.95, settings);
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
            plotView(renderer_, x, y, 0.20 + (1.0 - depth) * 0.9 + s.pulse * 0.6, settings);
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
        plotView(renderer_, x, y, 0.24 + s.pulse + std::abs(s.noise) * 0.35, settings);
    }
}

void AnimationScene::drawSpectralRibbon(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings) {
    const double scan = frame.timeSeconds * 0.72;
    const double width = static_cast<double>(renderer_.width() - 1);
    const double height = static_cast<double>(renderer_.height() - 1);
    const int columns = renderer_.width();

    for (int x = 0; x < columns; ++x) {
        const double bin = static_cast<double>(x) / static_cast<double>(std::max(1, columns - 1));
        const auto& s = sampleAt(source, static_cast<std::size_t>(x * 47 + static_cast<int>(frame.frameIndex) * 13));
        const double folded = std::abs(std::sin((bin * 7.0 + s.phase + scan * 0.23) * soemdsp::constant::kTAU));
        const double motion = std::sin((bin * 2.0 + scan + s.lfo * 0.12) * soemdsp::constant::kTAU) * 0.18;
        const double energy = std::clamp((s.pulse * 0.72 + folded * 0.34 + std::abs(s.noise) * 0.18) * settings.zoom, 0.02, 1.0);
        const int bar = std::clamp(static_cast<int>(energy * height * 0.78), 1, renderer_.height());

        for (int y = 0; y < bar; ++y) {
            const double rise = static_cast<double>(y) / std::max(1.0, height);
            const double px = (static_cast<double>(x) / width) * 2.0 - 1.0;
            const double py = -0.88 + rise * 1.58 + motion * (1.0 - rise);
            plotView(renderer_, px / settings.zoom, py / settings.zoom, 0.20 + energy * 0.9 - rise * 0.18, settings);
        }
    }
}

void AnimationScene::drawSinCosCircle(const SignalFrame& frame, const SignalSource& source, const SceneSettings& settings) {
    constexpr std::string_view glyphs{ "@*oO+x=:. " };
    const double phase = frame.timeSeconds * 0.42;
    const double pulse = source.stats.peak * 0.16 + std::sin(frame.timeSeconds * soemdsp::constant::kTAU * 0.23) * 0.04;
    const double radius = std::clamp(0.46 + pulse, 0.25, 0.72);

    for (int ring = 0; ring < 4; ++ring) {
        const double ringRadius = radius + static_cast<double>(ring) * 0.035;
        const double ringPhase = phase * (1.0 + static_cast<double>(ring) * 0.18);
        const double intensity = 0.92 - static_cast<double>(ring) * 0.15;

        for (int i = 0; i < 144; ++i) {
            const auto& s = sampleAt(source, static_cast<std::size_t>(i * 31 + ring * 127));
            const double p = static_cast<double>(i) / 144.0;
            const double wobble = std::sin((p * 8.0 + s.lfo * 0.2 + frame.timeSeconds * 0.6) * soemdsp::constant::kTAU) * 0.012;
            const double angle = p * soemdsp::constant::kTAU + ringPhase + s.noise * 0.01;
            const double x = std::cos(angle) * (ringRadius + wobble);
            const double y = std::sin(angle) * (ringRadius + wobble);
            plotView(renderer_, x, y, intensity + s.pulse * 0.25, settings);

            if (i % 3 == 0) {
                const auto glyphIndex = static_cast<std::size_t>((i / 3 + ring + static_cast<int>(frame.frameIndex / 4)) % static_cast<int>(glyphs.size()));
                const char glyph[2]{ glyphs[glyphIndex], '\0' };
                renderer_.writeText(gridX(renderer_, x, settings), gridY(renderer_, y, settings), glyph, 8 + static_cast<std::uint8_t>(ring));
            }
        }
    }

    const double markerAngle = phase * soemdsp::constant::kTAU;
    const double mx = std::cos(markerAngle) * radius;
    const double my = std::sin(markerAngle) * radius;
    renderer_.writeText(gridX(renderer_, mx, settings), gridY(renderer_, my, settings), "SOEM", 13);
}

} // namespace asciiscope

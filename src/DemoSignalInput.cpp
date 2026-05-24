#include <asciiscope/DemoSignalInput.hpp>

#include <algorithm>
#include <cmath>

#include <soemdsp/SampleRate.hpp>

namespace asciiscope {

DemoSignalInput::DemoSignalInput() {
    soemdsp::SampleRate::Update(44100.0, 64.0);

    sprott_.frequency = 28.0;
    sprott_.frequencyChanged();
    thomas_.frequency = 18.0;
    thomas_.frequencyChanged();

    scan_.setSampleRate(soemdsp::SampleRate::freq_);
    scan_.setFrequency(0.075);
    wobble_.setSampleRate(soemdsp::SampleRate::freq_);
    wobble_.setFrequency(0.19);
    pulseClock_.setSampleRate(soemdsp::SampleRate::freq_);
    pulseClock_.setFrequency(1.8);

    noise_.setSeed(0xA5C115C0);

    envelope_.delayTime_ = 0.0;
    envelope_.attackTime_ = 0.003;
    envelope_.releaseTime_ = 0.22;
    envelope_.sustainOffOn_ = 0;
    envelope_.velocitySensitivity_ = 0.35;
    envelope_.sampleRateChanged();

    energy_.setState(0.0);
    energy_.setTimeInSamples(900.0);
}

SignalFrame DemoSignalInput::nextFrame(std::uint64_t frameIndex, std::size_t sampleCount) {
    SignalSource main;
    main.id = "main";
    main.name = "Generated soemdsp Signal";
    main.kind = SignalKind::Audio;
    main.channelCount = 6;
    main.samples.reserve(sampleCount);

    double sumSquares = 0.0;

    for (std::size_t i = 0; i < sampleCount; ++i) {
        auto sample = tick();
        main.stats.min = i == 0 ? sample.pulse : std::min(main.stats.min, sample.pulse);
        main.stats.max = i == 0 ? sample.pulse : std::max(main.stats.max, sample.pulse);
        main.stats.peak = std::max(main.stats.peak, std::abs(sample.pulse));
        sumSquares += sample.pulse * sample.pulse;
        main.samples.push_back(sample);
    }

    if (!main.samples.empty()) {
        main.stats.rms = std::sqrt(sumSquares / static_cast<double>(main.samples.size()));
    }

    SignalFrame frame;
    frame.frameIndex = frameIndex;
    frame.sampleRate = soemdsp::SampleRate::freq_;
    frame.deltaTime = 1.0 / 60.0;
    frame.timeSeconds = static_cast<double>(frameIndex) * frame.deltaTime;
    frame.sources.push_back(std::move(main));
    return frame;
}

SignalSample DemoSignalInput::tick() {
    triggerIfNeeded();

    sprott_.run();
    thomas_.run();

    const double phase = scan_.getSample();
    const double wobble = wobble_.getSample();
    const double env = envelope_.run();
    energy_.setTarget(std::clamp(env + 0.14 * noise_.runUnipolar(), 0.0, 1.0));
    const double pulse = energy_.run();

    return {
        (sprott_.x * 0.55) + (thomas_.x * 0.12),
        (sprott_.y * 0.38) + (thomas_.y * 0.10),
        (sprott_.z * 0.42) + (thomas_.z * 0.08),
        wobble * 2.0 - 1.0,
        pulse,
        noise_.runBipolar(),
        phase,
    };
}

void DemoSignalInput::triggerIfNeeded() {
    ++sampleCounter_;
    const double before = pulseClock_.getUnipolarValue();
    pulseClock_.increment();
    const double after = pulseClock_.getUnipolarValue();

    if (after < before || sampleCounter_ == 1) {
        envelope_.velocity_ = 0.65 + 0.35 * noise_.runUnipolar();
        envelope_.triggerAttack();
    }
}

} // namespace asciiscope

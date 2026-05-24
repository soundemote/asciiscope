#pragma once

#include <asciiscope/SignalInput.hpp>

#include <soemdsp/envelope/LinearEnvelope.hpp>
#include <soemdsp/filter/Smoother.hpp>
#include <soemdsp/modulator/Attractor.hpp>
#include <soemdsp/oscillator/Phasor.hpp>
#include <soemdsp/random/NoiseGenerator.hpp>

namespace asciiscope {

class DemoSignalInput final : public ISignalInput {
  public:
    explicit DemoSignalInput(std::uint32_t seed = 0xA5C115C0);

    SignalFrame nextFrame(std::uint64_t frameIndex, std::size_t sampleCount) override;

  private:
    SignalSample tick();
    void triggerIfNeeded();

    soemdsp::oscillator::MultiSprottC sprott_;
    soemdsp::oscillator::Thomas thomas_;
    soemdsp::oscillator::Phasor scan_;
    soemdsp::oscillator::Phasor wobble_;
    soemdsp::oscillator::Phasor pulseClock_;
    soemdsp::utility::NoiseGenerator noise_;
    soemdsp::modulator::LinearDASR envelope_;
    soemdsp::filter::LinearSmoother energy_;
    int sampleCounter_{};
};

} // namespace asciiscope

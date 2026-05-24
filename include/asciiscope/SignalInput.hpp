#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace asciiscope {

enum class SignalKind {
    Audio,
    Control,
    Trigger,
    Debug
};

struct SignalSample {
    double x{};
    double y{};
    double z{};
    double lfo{};
    double pulse{};
    double noise{};
    double phase{};
};

struct SignalStats {
    double min{};
    double max{};
    double rms{};
    double peak{};
};

struct SignalSource {
    std::string id;
    std::string name;
    SignalKind kind{ SignalKind::Audio };
    int channelCount{ 1 };
    std::vector<SignalSample> samples;
    SignalStats stats;
};

struct SignalFrame {
    std::uint64_t frameIndex{};
    double sampleRate{};
    double deltaTime{};
    double timeSeconds{};
    std::vector<SignalSource> sources;

    [[nodiscard]] const SignalSource* findSource(std::string_view id) const noexcept;
};

class ISignalInput {
  public:
    virtual ~ISignalInput() = default;
    virtual SignalFrame nextFrame(std::uint64_t frameIndex, std::size_t sampleCount) = 0;
};

} // namespace asciiscope

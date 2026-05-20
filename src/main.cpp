#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include <soemdsp/SampleRate.hpp>
#include <soemdsp/modulator/Attractor.hpp>
#include <soemdsp/semath.hpp>

#include "AsciiOscilloscope.hpp"

namespace runtime {
constexpr int refresh_ms = 10;
constexpr int steps_per_frame = 30000;
constexpr double rotSpeedX = 16.0 / 8.0 * 0.001;
constexpr double rotSpeedY = 1.0 / 8.0 * 0.01;
} // namespace runtime

int main() {
  std::ios::sync_with_stdio(false);

  // setup dsp
  soemdsp::SampleRate::freq_ = 44100.0;
  soemdsp::oscillator::MultiSprottC attractor;
  attractor.frequency = 20.0;
  attractor.frequencyChanged();

  // init
  AsciiOscilloscope::Config scopeConfig;
  scopeConfig.width = 120;
  scopeConfig.height = 40;
  scopeConfig.zoom = 8.0;
  AsciiOscilloscope scope(scopeConfig);

  double angleX = 0.0;
  double angleY = 0.0;

  std::cout << "Starting Clean-Slate Scope... (Ctrl+C to quit)" << std::endl;

  while (true) {
    angleX = std::fmod(angleX + runtime::rotSpeedX, soemdsp::constant::kTAU);
    angleY = std::fmod(angleY + runtime::rotSpeedY, soemdsp::constant::kTAU);

    double cosX = std::cos(angleX);
    double sinX = std::sin(angleX);
    double cosY = std::cos(angleY);
    double sinY = std::sin(angleY);

    // Process a block of audio/data variables and push to scope
    for (int i = 0; i < runtime::steps_per_frame; ++i) {
      attractor.run();

      // Perform 3D to 2D projection rotation transforms
      double x1 = attractor.x * cosY + attractor.z * sinY;
      double z1 = -attractor.x * sinY + attractor.z * cosY;
      double y1 = attractor.y;
      double z_final = y1 * sinX + z1 * cosX;

      // Push our custom transformed coordinates to the scope object
      scope.pushSamplePair(x1, z_final);
    }

    // Render out the frame string dynamically tracking label info
    std::string currentFrame = scope.renderToString(angleX / soemdsp::constant::kTAU, angleY / soemdsp::constant::kTAU);
    std::cout << currentFrame << std::flush;

    std::this_thread::sleep_for(std::chrono::milliseconds(runtime::refresh_ms));
  }
  return 0;
}
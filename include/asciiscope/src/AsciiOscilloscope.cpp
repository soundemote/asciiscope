#include "../AsciiOscilloscope.hpp"
#include <algorithm>
#include <iomanip>

AsciiOscilloscope::AsciiOscilloscope(const Config& config) {
    configure(config);
}

void AsciiOscilloscope::configure(const Config& config) {
    mConfig = config;
    
    // Resize age buffers
    mAgeBuffer.assign(mConfig.height, std::vector<int>(mConfig.width, 0));
    
    // Resize and rebuild the static border layout
    rebuildBorderCanvas();
}

void AsciiOscilloscope::clearGrid() {
    for (auto& row : mAgeBuffer) {
        std::fill(row.begin(), row.end(), 0);
    }
}

void AsciiOscilloscope::rebuildBorderCanvas() {
    mBorderCanvas.assign(mConfig.height, std::string(mConfig.width, ' '));
    
    if (mConfig.width < 2 || mConfig.height < 2) return;

    // Horizontals
    for (int i = 0; i < mConfig.width; ++i) {
        mBorderCanvas[0][i]                  = '-';
        mBorderCanvas[mConfig.height - 1][i] = '-';
    }
    // Verticals
    for (int i = 0; i < mConfig.height; ++i) {
        mBorderCanvas[i][0]                 = '|';
        mBorderCanvas[i][mConfig.width - 1] = '|';
    }
    // Corners
    mBorderCanvas[0][0] = mBorderCanvas[0][mConfig.width - 1] = '+';
    mBorderCanvas[mConfig.height - 1][0] = mBorderCanvas[mConfig.height - 1][mConfig.width - 1] = '+';
}

void AsciiOscilloscope::pushSamplePair(double x, double y) {
    // Coordinate mapping with standard 0.4 height squish aspect ratio fix for terminals
    int col = static_cast<int>((mConfig.width / 2.0) + (x * mConfig.zoom));
    int row = static_cast<int>((mConfig.height / 2.0) - (y * mConfig.zoom * 0.4));

    // Clamp inside borders safely (1 to dimension - 2)
    col = std::clamp(col, 1, mConfig.width - 2);
    row = std::clamp(row, 1, mConfig.height - 2);

    mAgeBuffer[row][col] = mConfig.maxSampleAge;
}

std::string AsciiOscilloscope::renderToString(double labelX, double labelY) {
    std::stringstream frame;
    
    // Clear screen and home cursor ANSI sequences
    frame << "\033[2J\033[H";

    // Draw the telemetry readout/label string
    frame << "\033[37mX: "
          << std::fixed << std::setprecision(3) << std::setw(6) << labelX
          << "   Y: "
          << std::fixed << std::setprecision(3) << std::setw(6) << labelY
          << "\033[0m\n";

    // Render Canvas Loop
    for (int r = 0; r < mConfig.height; ++r) {
        for (int c = 0; c < mConfig.width; ++c) {
            if (mBorderCanvas[r][c] != ' ') {
                frame << "\033[37m" << mBorderCanvas[r][c]; // White border
            } else if (mAgeBuffer[r][c] > 0) {
                // Select symbol based on remaining sample lifetime intensity
                char symbol = (mAgeBuffer[r][c] > mConfig.fadeThreshold) ? '*' : '.';
                frame << "\033[33m" << symbol;              // Orange/Yellow trace
                
                // Decay the pixel age for the next frame iteration
                mAgeBuffer[r][c]--;
            } else {
                frame << " ";
            }
        }
        frame << "\n";
    }
    frame << "\033[0m"; // Reset terminal styling colors

    return frame.str();
}
#ifndef ASCII_OSCILLOSCOPE_HPP
#define ASCII_OSCILLOSCOPE_HPP

#include <string>
#include <vector>
#include <sstream>

class AsciiOscilloscope {
public:
    struct Config {
        int width         = 120;
        int height        = 40;
        double zoom       = 8.0; // Overall scale
        double offsetX    = 0.0; // Shift trace horizontally (+ is right, - is left)
        double offsetY    = 0.0; // Shift trace vertically (+ is up, - is down)
        int maxSampleAge  = 10;
        int fadeThreshold = 5;
    };

    AsciiOscilloscope(const Config& config = Config());
    ~AsciiOscilloscope() = default;

    // Resizes internal buffers if dimensions change
    void configure(const Config& config);

    // Clears the vector age buffer back to 0 (excluding the static border)
    void clearGrid();

    // Safely maps a coordinate and inserts it into the rendering age buffer
    void pushSamplePair(double x, double y);

    // Generates the ANSI escape frame string ready to be flushed to std::cout
    std::string renderToString(double labelX = 0.0, double labelY = 0.0);

    // Getters for dimensions
    int getWidth() const  { return mConfig.width; }
    int getHeight() const { return mConfig.height; }

private:
    void rebuildBorderCanvas();

    Config mConfig;
    std::vector<std::string> mBorderCanvas;
    std::vector<std::vector<int>> mAgeBuffer;
};

#endif // ASCII_OSCILLOSCOPE_HPP
#include <asciiscope/SignalInput.hpp>

namespace asciiscope {

const SignalSource* SignalFrame::findSource(std::string_view id) const noexcept {
    for (const auto& source : sources) {
        if (source.id == id) {
            return &source;
        }
    }

    return nullptr;
}

} // namespace asciiscope

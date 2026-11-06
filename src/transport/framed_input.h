#ifndef SCPI_ESP32_FRAMED_INPUT_H
#define SCPI_ESP32_FRAMED_INPUT_H

#include <stdint.h>
#include "transport/message_framer.h"
#include "scpi/engine_interface.h"

namespace scpi {
namespace transport {

// Connects a byte-stream framer to the transport-independent SCPI engine.
// The transport calls feed() for received bytes and poll() after draining
// available input. Only complete, non-empty messages reach the engine.
class FramedInput {
public:
    FramedInput(EngineInterface& engine,
                const MessageFramer::Config& config = MessageFramer::Config())
        : engine_(engine), framer_(config) {}

    MessageFramer::Result feed(uint8_t byte, uint32_t now_ms) {
        return dispatchIfReady(framer_.feed(byte, now_ms));
    }

    MessageFramer::Result poll(uint32_t now_ms) {
        return dispatchIfReady(framer_.poll(now_ms));
    }

    void reset() {
        framer_.reset();
    }

private:
    MessageFramer::Result dispatchIfReady(const MessageFramer::Result& result) {
        if (result.type == MessageFramer::MESSAGE_READY) {
            engine_.processProgramMessage(result.message);
        }
        return result;
    }

    EngineInterface& engine_;
    MessageFramer framer_;
};

} // namespace transport
} // namespace scpi

#endif // SCPI_ESP32_FRAMED_INPUT_H

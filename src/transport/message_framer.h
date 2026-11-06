#ifndef SCPI_ESP32_MESSAGE_FRAMER_H
#define SCPI_ESP32_MESSAGE_FRAMER_H

#include <stddef.h>
#include <stdint.h>
#include <string>

namespace scpi {
namespace transport {

// Converts a byte stream into complete program messages. This class is
// transport-only: it does not parse SCPI headers, units, or parameters.
class MessageFramer {
public:
    struct Config {
        size_t max_message_bytes;
        uint32_t inactivity_timeout_ms; // 0 disables timeout framing
        bool accept_cr;                 // project compatibility option

        Config(size_t max_bytes = 256,
               uint32_t timeout_ms = 0,
               bool cr_enabled = true)
            : max_message_bytes(max_bytes),
              inactivity_timeout_ms(timeout_ms),
              accept_cr(cr_enabled) {}
    };

    enum EventType {
        NO_EVENT,
        MESSAGE_READY,
        MESSAGE_OVERFLOW
    };

    struct Result {
        EventType type;
        std::string message;

        Result(EventType event = NO_EVENT, const std::string& text = std::string())
            : type(event), message(text) {}
    };

    explicit MessageFramer(const Config& config = Config())
        : config_(config), last_byte_ms_(0), have_last_byte_time_(false),
          swallow_lf_(false), discarding_overflow_(false) {
        buffer_.reserve(config_.max_message_bytes);
    }

    // Supply each received byte and the monotonic millisecond timestamp at
    // which it was received. A CR completes immediately; a following LF is
    // consumed as the second half of CR-LF, including across calls.
    Result feed(uint8_t byte, uint32_t now_ms) {
        if (swallow_lf_) {
            swallow_lf_ = false;
            if (byte == '\n') {
                return Result();
            }
        }

        if (byte == '\n') {
            return finishMessage();
        }

        if (byte == '\r' && config_.accept_cr) {
            swallow_lf_ = true;
            return finishMessage();
        }

        if (discarding_overflow_) {
            last_byte_ms_ = now_ms;
            have_last_byte_time_ = true;
            return Result();
        }

        if (buffer_.size() >= config_.max_message_bytes) {
            buffer_.clear();
            discarding_overflow_ = true;
            last_byte_ms_ = now_ms;
            have_last_byte_time_ = true;
            return Result(MESSAGE_OVERFLOW);
        }

        buffer_.push_back(static_cast<char>(byte));
        last_byte_ms_ = now_ms;
        have_last_byte_time_ = true;
        return Result();
    }

    // Call regularly from the transport service loop. Uses unsigned elapsed
    // arithmetic, which remains correct across uint32_t millisecond rollover.
    Result poll(uint32_t now_ms) {
        if (config_.inactivity_timeout_ms == 0 || !have_last_byte_time_) {
            return Result();
        }
        if (static_cast<uint32_t>(now_ms - last_byte_ms_) <
            config_.inactivity_timeout_ms) {
            return Result();
        }
        have_last_byte_time_ = false;
        if (discarding_overflow_) {
            discarding_overflow_ = false;
            buffer_.clear();
            return Result();
        }
        return finishMessage();
    }

    void reset() {
        buffer_.clear();
        have_last_byte_time_ = false;
        swallow_lf_ = false;
        discarding_overflow_ = false;
    }

private:
    Result finishMessage() {
        if (discarding_overflow_) {
            discarding_overflow_ = false;
            buffer_.clear();
            have_last_byte_time_ = false;
            return Result();
        }
        if (buffer_.empty()) {
            have_last_byte_time_ = false;
            return Result(); // Empty messages are deliberately not dispatched.
        }
        std::string completed;
        completed.swap(buffer_);
        buffer_.reserve(config_.max_message_bytes);
        have_last_byte_time_ = false;
        return Result(MESSAGE_READY, completed);
    }

    Config config_;
    std::string buffer_;
    uint32_t last_byte_ms_;
    bool have_last_byte_time_;
    bool swallow_lf_;
    bool discarding_overflow_;
};

} // namespace transport
} // namespace scpi

#endif // SCPI_ESP32_MESSAGE_FRAMER_H

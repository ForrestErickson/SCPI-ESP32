#ifndef SCPI_ESP32_MESSAGE_FRAMER_H
#define SCPI_ESP32_MESSAGE_FRAMER_H

#include <stddef.h>
#include <stdint.h>
#include <string>

namespace scpi {
namespace transport {

// Converts a byte stream into complete program messages. This class is
// transport-only: it does not parse SCPI headers, units, or parameters.
// LF and CRLF terminate messages. A CR not followed by LF is message data.
class MessageFramer {
public:
    struct Config {
        size_t max_message_bytes;
        uint32_t inactivity_timeout_ms; // 0 disables timeout framing

        Config(size_t max_bytes = 256, uint32_t timeout_ms = 0)
            : max_message_bytes(max_bytes),
              inactivity_timeout_ms(timeout_ms) {}
    };

    enum EventType {
        NO_EVENT,
        MESSAGE_READY,
        MESSAGE_OVERFLOW
    };

    enum Terminator {
        NO_TERMINATOR,
        TERMINATED_BY_LF,
        TERMINATED_BY_CRLF,
        TERMINATED_BY_TIMEOUT
    };

    struct Result {
        EventType type;
        Terminator terminator;
        std::string message;

        Result(EventType event = NO_EVENT,
               const std::string& text = std::string(),
               Terminator end = NO_TERMINATOR)
            : type(event), terminator(end), message(text) {}
    };

    explicit MessageFramer(const Config& config = Config())
        : config_(config), last_byte_ms_(0), pending_cr_ms_(0),
          have_last_byte_time_(false), pending_cr_(false),
          discarding_overflow_(false) {
        buffer_.reserve(config_.max_message_bytes);
    }

    // Supply each received byte and its monotonic millisecond timestamp.
    // Hold CR briefly so CRLF can be recognized as one terminator. If the
    // next byte is not LF, the CR is retained as ordinary message data.
    Result feed(uint8_t byte, uint32_t now_ms) {
        if (pending_cr_) {
            pending_cr_ = false;
            if (byte == '\n') {
                return finishMessage(TERMINATED_BY_CRLF);
            }
            Result prior = appendDataByte('\r', pending_cr_ms_);
            Result current = processByte(byte, now_ms);
            return prior.type != NO_EVENT ? prior : current;
        }
        return processByte(byte, now_ms);
    }

    // Call after draining currently available input and regularly from the
    // transport loop. A pending CR becomes data, never a terminator.
    Result poll(uint32_t now_ms) {
        if (pending_cr_) {
            pending_cr_ = false;
            Result prior = appendDataByte('\r', pending_cr_ms_);
            if (prior.type != NO_EVENT) {
                return prior;
            }
        }
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
        return finishMessage(TERMINATED_BY_TIMEOUT);
    }

    void reset() {
        buffer_.clear();
        have_last_byte_time_ = false;
        pending_cr_ = false;
        discarding_overflow_ = false;
    }

private:
    Result processByte(uint8_t byte, uint32_t now_ms) {
        if (byte == '\n') {
            return finishMessage(TERMINATED_BY_LF);
        }
        if (byte == '\r') {
            pending_cr_ = true;
            pending_cr_ms_ = now_ms;
            last_byte_ms_ = now_ms;
            have_last_byte_time_ = true;
            return Result();
        }
        return appendDataByte(static_cast<char>(byte), now_ms);
    }

    Result appendDataByte(char byte, uint32_t now_ms) {
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
        buffer_.push_back(byte);
        last_byte_ms_ = now_ms;
        have_last_byte_time_ = true;
        return Result();
    }

    Result finishMessage(Terminator terminator) {
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
        return Result(MESSAGE_READY, completed, terminator);
    }

    Config config_;
    std::string buffer_;
    uint32_t last_byte_ms_;
    uint32_t pending_cr_ms_;
    bool have_last_byte_time_;
    bool pending_cr_;
    bool discarding_overflow_;
};

} // namespace transport
} // namespace scpi

#endif // SCPI_ESP32_MESSAGE_FRAMER_H

#ifndef SCPI_ESP32_SCPI_ERROR_QUEUE_H
#define SCPI_ESP32_SCPI_ERROR_QUEUE_H

#include <stddef.h>
#include <deque>
#include <string>
#include "scpi/error.h"

namespace scpi {

// Bounded FIFO for errors detected while processing SCPI messages.
// When full, existing errors are preserved and additional errors are
// represented by one -350 "Queue overflow" entry after the queued errors.
class ErrorQueue {
public:
    static const size_t DEFAULT_CAPACITY = 16;

    explicit ErrorQueue(size_t capacity = DEFAULT_CAPACITY)
        : capacity_(capacity == 0 ? 1 : capacity), overflow_pending_(false) {}

    // Returns true when the error was queued. If full, returns false and
    // records a single pending queue-overflow indication.
    bool push(int code, const std::string& message) {
        if (errors_.size() >= capacity_) {
            overflow_pending_ = true;
            return false;
        }
        errors_.push_back(ScpiError(code, message));
        return true;
    }

    // Removes the oldest error. Once ordinary entries are drained, a single
    // queue-overflow error is returned if any pushes were rejected.
    bool pop(ScpiError& error) {
        if (!errors_.empty()) {
            error = errors_.front();
            errors_.pop_front();
            return true;
        }
        if (overflow_pending_) {
            error = ScpiError(-350, "Queue overflow");
            overflow_pending_ = false;
            return true;
        }
        return false;
    }

    size_t size() const {
        return errors_.size() + (overflow_pending_ ? 1U : 0U);
    }

    bool empty() const {
        return errors_.empty() && !overflow_pending_;
    }

    void clear() {
        errors_.clear();
        overflow_pending_ = false;
    }

private:
    size_t capacity_;
    std::deque<ScpiError> errors_;
    bool overflow_pending_;
};

} // namespace scpi

#endif // SCPI_ESP32_SCPI_ERROR_QUEUE_H

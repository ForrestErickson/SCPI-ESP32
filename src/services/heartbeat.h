#ifndef SCPI_ESP32_SERVICES_HEARTBEAT_H
#define SCPI_ESP32_SERVICES_HEARTBEAT_H

#include <stdint.h>

namespace services {

// Hardware-independent logical LED output. A board adapter translates this
// logical state into GPIO voltage levels and accounts for LED polarity.
class LogicalLedOutput {
public:
    virtual ~LogicalLedOutput() {}
    virtual void setLedOn(bool on) = 0;
};

// Nonblocking heartbeat service. Call service() frequently from the main loop.
// Time is supplied by the application in monotonically advancing milliseconds;
// unsigned subtraction makes elapsed-time calculations safe across uint32_t wrap.
class Heartbeat {
public:
    static const uint32_t DEFAULT_PERIOD_MS = 2000;
    static const uint8_t DEFAULT_DUTY_PERCENT = 25;
    static const uint32_t MIN_PERIOD_MS = 1;
    static const uint32_t MAX_PERIOD_MS = 86400000; // 24 hours

    explicit Heartbeat(LogicalLedOutput& output)
        : output_(output),
          enabled_(false),
          configured_period_ms_(DEFAULT_PERIOD_MS),
          active_period_ms_(DEFAULT_PERIOD_MS),
          duty_percent_(DEFAULT_DUTY_PERCENT),
          cycle_started_ms_(0),
          led_on_(false) {
        output_.setLedOn(false);
    }

    bool enabled() const { return enabled_; }
    uint32_t periodMilliseconds() const { return configured_period_ms_; }
    uint8_t dutyCyclePercent() const { return duty_percent_; }
    bool ledOn() const { return led_on_; }

    // A configured period is adopted when the next fresh cycle begins.
    bool setPeriodMilliseconds(uint32_t period_ms) {
        if (period_ms < MIN_PERIOD_MS || period_ms > MAX_PERIOD_MS) {
            return false;
        }
        configured_period_ms_ = period_ms;
        return true;
    }

    // Changing duty while enabled immediately starts a fresh cycle using the
    // active period. The new cycle begins ON unless duty is zero.
    bool setDutyCyclePercent(uint8_t percent, uint32_t now_ms) {
        if (percent > 100) {
            return false;
        }
        duty_percent_ = percent;
        if (enabled_) {
            cycle_started_ms_ = now_ms;
            updateOutput(percent != 0);
        } else {
            updateOutput(false);
        }
        return true;
    }

    // Enabling always begins a fresh cycle using the latest configured period.
    // Disabling interrupts the current cycle and turns the logical LED off now.
    void setEnabled(bool enabled, uint32_t now_ms) {
        if (enabled) {
            enabled_ = true;
            active_period_ms_ = configured_period_ms_;
            cycle_started_ms_ = now_ms;
            updateOutput(duty_percent_ != 0);
        } else {
            enabled_ = false;
            updateOutput(false);
        }
    }

    // Recalculate the desired output and return promptly; never waits or delays.
    void service(uint32_t now_ms) {
        if (!enabled_) {
            updateOutput(false);
            return;
        }

        if (duty_percent_ == 0) {
            updateOutput(false);
            return;
        }
        if (duty_percent_ == 100) {
            updateOutput(true);
            return;
        }

        const uint32_t elapsed_ms =
            static_cast<uint32_t>(now_ms - cycle_started_ms_);
        const uint32_t on_time_ms = static_cast<uint32_t>(
            (static_cast<uint64_t>(active_period_ms_) * duty_percent_) / 100U);
        const uint32_t phase_ms = elapsed_ms % active_period_ms_;
        updateOutput(phase_ms < on_time_ms);
    }

private:
    void updateOutput(bool on) {
        if (led_on_ != on) {
            led_on_ = on;
            output_.setLedOn(on);
        }
    }

    LogicalLedOutput& output_;
    bool enabled_;
    uint32_t configured_period_ms_;
    uint32_t active_period_ms_;
    uint8_t duty_percent_;
    uint32_t cycle_started_ms_;
    bool led_on_;
};

} // namespace services

#endif // SCPI_ESP32_SERVICES_HEARTBEAT_H

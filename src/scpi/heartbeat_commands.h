#ifndef SCPI_ESP32_SCPI_HEARTBEAT_COMMANDS_H
#define SCPI_ESP32_SCPI_HEARTBEAT_COMMANDS_H

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "scpi/command_registry.h"
#include "services/heartbeat.h"

namespace scpi {

// Supplies the application's monotonic millisecond clock without introducing
// an Arduino or ESP32 dependency into this command module.
typedef uint32_t (*HeartbeatNowFunction)(void* user_context);

// Adapts HEARTbeat SCPI commands to the hardware-independent heartbeat
// service. This object must outlive dispatch through the command registry.
class HeartbeatCommands {
public:
    HeartbeatCommands(services::Heartbeat& heartbeat,
                      HeartbeatNowFunction now_function,
                      void* clock_context = 0)
        : heartbeat_(heartbeat),
          now_function_(now_function),
          clock_context_(clock_context) {}

    bool registerWith(CommandRegistry& registry) {
        if (now_function_ == 0) return false;
        if (!registry.registerCommand("HEARTbeat:STATe",
                                      &HeartbeatCommands::handleState, this)) {
            return false;
        }
        if (!registry.registerCommand("HEARTbeat:PERiod",
                                      &HeartbeatCommands::handlePeriod, this)) {
            return false;
        }
        if (!registry.registerCommand("HEARTbeat:DUTYcycle",
                                      &HeartbeatCommands::handleDuty, this)) {
            return false;
        }
        return true;
    }

private:
    static void parameterError(ErrorQueue& errors) {
        errors.push(-108, "Parameter not allowed");
    }

    static bool parseOnOff(const std::string& text, bool& value) {
        if (equalsIgnoreCase(text, "ON")) {
            value = true;
            return true;
        }
        if (equalsIgnoreCase(text, "OFF")) {
            value = false;
            return true;
        }
        return false;
    }

    static bool equalsIgnoreCase(const std::string& left,
                                 const std::string& right) {
        if (left.size() != right.size()) return false;
        for (std::string::size_type i = 0; i < left.size(); ++i) {
            const unsigned char a = static_cast<unsigned char>(left[i]);
            const unsigned char b = static_cast<unsigned char>(right[i]);
            if (std::tolower(a) != std::tolower(b)) return false;
        }
        return true;
    }

    static std::string trim(const std::string& value) {
        const std::string::size_type first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        const std::string::size_type last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    // Periods are expressed in seconds by SCPI and stored by the service as
    // integer milliseconds. Values finer than 1 ms are rejected, not rounded.
    static bool parsePeriodMilliseconds(const std::string& text,
                                        uint32_t& period_ms,
                                        bool& out_of_range) {
        out_of_range = false;
        const std::string value = trim(text);
        if (value.empty()) return false;

        errno = 0;
        char* end = 0;
        const double seconds = std::strtod(value.c_str(), &end);
        if (end == value.c_str() || *end != '\0' || errno == ERANGE ||
            !std::isfinite(seconds)) {
            return false;
        }

        if (seconds < 0.001 || seconds > 86400.0) {
            out_of_range = true;
            return false;
        }

        const double milliseconds = seconds * 1000.0;
        const double nearest = std::floor(milliseconds + 0.5);
        if (std::fabs(milliseconds - nearest) > 0.000001) return false;
        if (nearest < 1.0 || nearest > 86400000.0) {
            out_of_range = true;
            return false;
        }
        period_ms = static_cast<uint32_t>(nearest);
        return true;
    }

    static bool parseDutyPercent(const std::string& text,
                                 uint8_t& percent,
                                 bool& out_of_range) {
        out_of_range = false;
        const std::string value = trim(text);
        if (value.empty()) return false;

        errno = 0;
        char* end = 0;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        if (end == value.c_str() || *end != '\0' || errno == ERANGE) {
            return false;
        }
        if (parsed < 0 || parsed > 100) {
            out_of_range = true;
            return false;
        }
        percent = static_cast<uint8_t>(parsed);
        return true;
    }

    static std::string formatPeriodSeconds(uint32_t period_ms) {
        char buffer[32];
        const unsigned long seconds = period_ms / 1000U;
        const unsigned long milliseconds = period_ms % 1000U;
        std::snprintf(buffer, sizeof(buffer), "%lu.%03lu", seconds, milliseconds);
        std::string result(buffer);
        while (!result.empty() && result[result.size() - 1] == '0') {
            result.erase(result.size() - 1);
        }
        if (!result.empty() && result[result.size() - 1] == '.') {
            result.erase(result.size() - 1);
        }
        return result;
    }

    static void handleState(void* context,
                            const CommandRequest& request,
                            ErrorQueue& errors,
                            ResponseSink& responses) {
        HeartbeatCommands* self = static_cast<HeartbeatCommands*>(context);
        if (request.is_query) {
            if (!trim(request.parameter_text).empty()) {
                parameterError(errors);
                return;
            }
            responses.writeResponse(self->heartbeat_.enabled() ? "1" : "0");
            return;
        }

        const std::string parameter = trim(request.parameter_text);
        if (parameter.empty()) {
            errors.push(-109, "Missing parameter");
            return;
        }
        bool enabled = false;
        if (!parseOnOff(parameter, enabled)) {
            errors.push(-128, "Numeric data error");
            return;
        }
        self->heartbeat_.setEnabled(enabled, self->now_function_(self->clock_context_));
    }

    static void handlePeriod(void* context,
                             const CommandRequest& request,
                             ErrorQueue& errors,
                             ResponseSink& responses) {
        HeartbeatCommands* self = static_cast<HeartbeatCommands*>(context);
        if (request.is_query) {
            if (!trim(request.parameter_text).empty()) {
                parameterError(errors);
                return;
            }
            responses.writeResponse(
                formatPeriodSeconds(self->heartbeat_.periodMilliseconds()));
            return;
        }

        const std::string parameter = trim(request.parameter_text);
        if (parameter.empty()) {
            errors.push(-109, "Missing parameter");
            return;
        }
        uint32_t period_ms = 0;
        bool out_of_range = false;
        if (!parsePeriodMilliseconds(parameter, period_ms, out_of_range)) {
            errors.push(out_of_range ? -222 : -128,
                        out_of_range ? "Data out of range" : "Numeric data error");
            return;
        }
        self->heartbeat_.setPeriodMilliseconds(period_ms);
    }

    static void handleDuty(void* context,
                           const CommandRequest& request,
                           ErrorQueue& errors,
                           ResponseSink& responses) {
        HeartbeatCommands* self = static_cast<HeartbeatCommands*>(context);
        if (request.is_query) {
            if (!trim(request.parameter_text).empty()) {
                parameterError(errors);
                return;
            }
            char buffer[8];
            std::snprintf(buffer, sizeof(buffer), "%u",
                          static_cast<unsigned>(self->heartbeat_.dutyCyclePercent()));
            responses.writeResponse(buffer);
            return;
        }

        const std::string parameter = trim(request.parameter_text);
        if (parameter.empty()) {
            errors.push(-109, "Missing parameter");
            return;
        }
        uint8_t percent = 0;
        bool out_of_range = false;
        if (!parseDutyPercent(parameter, percent, out_of_range)) {
            errors.push(out_of_range ? -222 : -128,
                        out_of_range ? "Data out of range" : "Numeric data error");
            return;
        }
        self->heartbeat_.setDutyCyclePercent(
            percent, self->now_function_(self->clock_context_));
    }

    services::Heartbeat& heartbeat_;
    HeartbeatNowFunction now_function_;
    void* clock_context_;
};

} // namespace scpi

#endif // SCPI_ESP32_SCPI_HEARTBEAT_COMMANDS_H

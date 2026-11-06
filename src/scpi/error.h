#ifndef SCPI_ESP32_SCPI_ERROR_H
#define SCPI_ESP32_SCPI_ERROR_H

#include <string>

namespace scpi {

// A SCPI-visible error code and its explanatory message.
// The code is kept separate from formatting so transports and tests can
// inspect errors without parsing a response string.
struct ScpiError {
    int code;
    std::string message;

    ScpiError(int error_code = 0, const std::string& error_message = "")
        : code(error_code), message(error_message) {}
};

} // namespace scpi

#endif // SCPI_ESP32_SCPI_ERROR_H

#ifndef SCPI_ESP32_SCPI_RESPONSE_SINK_H
#define SCPI_ESP32_SCPI_RESPONSE_SINK_H

#include <string>

namespace scpi {

// Destination for a complete SCPI response message.
// The sink owns transport-specific delivery and message termination.
class ResponseSink {
public:
    virtual ~ResponseSink() {}
    virtual void writeResponse(const std::string& response) = 0;
};

} // namespace scpi

#endif // SCPI_ESP32_SCPI_RESPONSE_SINK_H

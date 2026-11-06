#ifndef SCPI_ESP32_ENGINE_INTERFACE_H
#define SCPI_ESP32_ENGINE_INTERFACE_H

#include <string>

namespace scpi {

// Transport-independent entry point implemented by the generic SCPI engine.
// The caller supplies exactly one complete program message per invocation.
class EngineInterface {
public:
    virtual ~EngineInterface() {}
    virtual void processProgramMessage(const std::string& message) = 0;
};

} // namespace scpi

#endif // SCPI_ESP32_ENGINE_INTERFACE_H

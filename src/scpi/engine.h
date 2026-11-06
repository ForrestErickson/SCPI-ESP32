#ifndef SCPI_ESP32_SCPI_ENGINE_H
#define SCPI_ESP32_SCPI_ENGINE_H

#include <string>

#include "scpi/command_registry.h"
#include "scpi/engine_interface.h"
#include "scpi/parser.h"

namespace scpi {

// Initial synchronous engine: parses one supported command unit and dispatches
// it to the registered handler. Dependencies are owned by the application.
class Engine : public EngineInterface {
public:
    Engine(CommandRegistry& registry, ErrorQueue& errors,
           ResponseSink& responses)
        : registry_(registry), errors_(errors), responses_(responses) {}

    void processScpiMessage(const std::string& message) {
        CommandRequest request;
        const ParseResult result = parser_.parse(message, request);

        if (result == ParseResult::EmptyMessage) return;

        if (result == ParseResult::MultipleCommandUnitsUnsupported) {
            errors_.push(-103, "Invalid separator");
            return;
        }

        if (result != ParseResult::Ok) {
            errors_.push(-102, "Syntax error");
            return;
        }

        if (!registry_.dispatch(request, errors_, responses_)) {
            errors_.push(-113, "Undefined header");
        }
    }

private:
    Parser parser_;
    CommandRegistry& registry_;
    ErrorQueue& errors_;
    ResponseSink& responses_;
};

} // namespace scpi

#endif // SCPI_ESP32_SCPI_ENGINE_H

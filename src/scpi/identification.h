#ifndef SCPI_ESP32_IDENTIFICATION_H
#define SCPI_ESP32_IDENTIFICATION_H

#include <string>

#include "scpi/command_registry.h"

namespace scpi {

// Application-supplied IEEE 488.2 identification fields. The four fields are
// emitted in manufacturer, model, serial number, firmware revision order.
// Fields must not contain commas in this initial implementation.
class Identification {
public:
    Identification(const std::string& manufacturer,
                   const std::string& model,
                   const std::string& serial_number,
                   const std::string& firmware_revision)
        : manufacturer_(manufacturer),
          model_(model),
          serial_number_(serial_number),
          firmware_revision_(firmware_revision) {}

    // The Identification object must outlive dispatch through this registry.
    bool registerWith(CommandRegistry& registry) {
        return registry.registerCommand("*IDN", &Identification::handle, this);
    }

private:
    static void handle(void* context,
                       const CommandRequest& request,
                       ErrorQueue& errors,
                       ResponseSink& responses) {
        Identification* identity = static_cast<Identification*>(context);
        if (!request.is_query || !request.parameter_text.empty()) {
            errors.push(-108, "Parameter not allowed");
            return;
        }

        responses.writeResponse(
            identity->manufacturer_ + "," +
            identity->model_ + "," +
            identity->serial_number_ + "," +
            identity->firmware_revision_);
    }

    std::string manufacturer_;
    std::string model_;
    std::string serial_number_;
    std::string firmware_revision_;
};

} // namespace scpi

#endif // SCPI_ESP32_IDENTIFICATION_H

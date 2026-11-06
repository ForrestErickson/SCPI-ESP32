#include <cassert>
#include <string>

#include "scpi/engine.h"
#include "scpi/identification.h"

class CaptureResponseSink : public scpi::ResponseSink {
public:
    std::string response;
    void writeResponse(const std::string& value) { response = value; }
};

int main() {
    scpi::CommandRegistry registry;
    scpi::ErrorQueue errors;
    CaptureResponseSink responses;
    scpi::Identification identity("! Amused Scientist", "SCPI-ESP32",
                                  "AA:BB:CC:DD:EE:FF", "0.1.0");
    assert(identity.registerWith(registry));

    scpi::Engine engine(registry, errors, responses);
    engine.processScpiMessage("*IDN?");
    assert(responses.response ==
           "! Amused Scientist,SCPI-ESP32,AA:BB:CC:DD:EE:FF,0.1.0");
    assert(errors.empty());

    engine.processScpiMessage("*IDN");
    scpi::ScpiError error;
    assert(errors.pop(error));
    assert(error.code == -108);

    engine.processScpiMessage("*IDN? extra");
    assert(errors.pop(error));
    assert(error.code == -108);

    return 0;
}

#include <cassert>
#include <string>

#include "scpi/engine.h"

class CaptureResponseSink : public scpi::ResponseSink {
public:
    std::string response;
    void writeResponse(const std::string& value) { response = value; }
};

struct InstrumentState {
    int set_calls;
    double value;
    InstrumentState() : set_calls(0), value(0.0) {}
};

static void voltageHandler(void* context,
                           const scpi::CommandRequest& request,
                           scpi::ErrorQueue& errors,
                           scpi::ResponseSink& responses) {
    InstrumentState* state = static_cast<InstrumentState*>(context);
    if (request.is_query) {
        responses.writeResponse("1.25");
        return;
    }
    if (request.parameter_text != "1.25") {
        errors.push(-128, "Numeric data error");
        return;
    }
    state->value = 1.25;
    ++state->set_calls;
}

int main() {
    scpi::CommandRegistry registry;
    scpi::ErrorQueue errors;
    CaptureResponseSink responses;
    InstrumentState state;

    assert(registry.registerCommand("SOURce:VOLTage", voltageHandler, &state));
    scpi::Engine engine(registry, errors, responses);

    engine.processScpiMessage("SOUR:VOLT 1.25");
    assert(state.set_calls == 1);
    assert(state.value == 1.25);
    assert(errors.empty());

    engine.processScpiMessage("SOUR:VOLT?");
    assert(responses.response == "1.25");
    assert(errors.empty());

    engine.processScpiMessage("NOPE?");
    scpi::ScpiError error;
    assert(errors.pop(error));
    assert(error.code == -113);

    engine.processScpiMessage("SOUR::VOLT?");
    assert(errors.pop(error));
    assert(error.code == -102);

    engine.processScpiMessage("SOUR:VOLT 1;SOUR:VOLT 2");
    assert(errors.pop(error));
    assert(error.code == -103);

    // Empty framed messages are ignored rather than entered in the error queue.
    engine.processScpiMessage("   ");
    assert(errors.empty());
    return 0;
}

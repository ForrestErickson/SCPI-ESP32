#include <cassert>
#include <string>

#include "scpi/command_registry.h"

class CaptureResponseSink : public scpi::ResponseSink {
public:
    std::string response;
    void writeResponse(const std::string& value) { response = value; }
};

struct TestState {
    int calls;
    std::string received_path;
    std::string received_parameters;
    bool received_query;
    TestState() : calls(0), received_query(false) {}
};

static void testHandler(void* context,
                        const scpi::CommandRequest& request,
                        scpi::ErrorQueue&,
                        scpi::ResponseSink& responses) {
    TestState* state = static_cast<TestState*>(context);
    ++state->calls;
    state->received_path = request.command_path;
    state->received_parameters = request.parameter_text;
    state->received_query = request.is_query;
    if (request.is_query) responses.writeResponse("42");
}

int main() {
    scpi::CommandRegistry registry;
    scpi::ErrorQueue errors;
    CaptureResponseSink responses;
    TestState state;

    assert(registry.size() == 0);
    assert(!registry.registerCommand("", testHandler, &state));
    assert(!registry.registerCommand("MEASure:VOLTage", 0, &state));
    assert(registry.registerCommand("MEASure:VOLTage", testHandler, &state));
    assert(registry.size() == 1);

    // Matching ignores case but does not yet accept abbreviated keywords.
    assert(registry.find("measure:voltage") != 0);
    assert(registry.find("MEASure:VOLT") == 0);
    assert(!registry.registerCommand("measure:voltage", testHandler, &state));

    scpi::CommandRequest set_request("MEASure:VOLTage", false, "12.5");
    assert(registry.dispatch(set_request, errors, responses));
    assert(state.calls == 1);
    assert(state.received_path == "MEASure:VOLTage");
    assert(state.received_parameters == "12.5");
    assert(!state.received_query);

    scpi::CommandRequest query_request("measure:voltage", true, "");
    assert(registry.dispatch(query_request, errors, responses));
    assert(state.calls == 2);
    assert(state.received_query);
    assert(responses.response == "42");

    scpi::CommandRequest unknown_request("SYSTem:VERSion", true, "");
    assert(!registry.dispatch(unknown_request, errors, responses));
    assert(state.calls == 2);
    return 0;
}

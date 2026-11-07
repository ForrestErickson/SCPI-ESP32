#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "scpi/engine.h"
#include "scpi/heartbeat_commands.h"

class FakeLed : public services::LogicalLedOutput {
public:
    FakeLed() : on(false) {}
    bool on;
    std::vector<bool> changes;
    void setLedOn(bool value) {
        on = value;
        changes.push_back(value);
    }
};

struct FakeClock {
    uint32_t now;
    FakeClock() : now(0) {}
};

static uint32_t readClock(void* context) {
    return static_cast<FakeClock*>(context)->now;
}

class CaptureResponseSink : public scpi::ResponseSink {
public:
    std::string response;
    void writeResponse(const std::string& value) { response = value; }
};

static void expectError(scpi::ErrorQueue& errors, int expected_code) {
    scpi::ScpiError error;
    assert(errors.pop(error));
    assert(error.code == expected_code);
    assert(errors.empty());
}

int main() {
    FakeLed led;
    services::Heartbeat heartbeat(led);
    FakeClock clock;
    scpi::CommandRegistry registry;
    scpi::ErrorQueue errors;
    CaptureResponseSink responses;
    scpi::HeartbeatCommands commands(heartbeat, readClock, &clock);

    assert(commands.registerWith(registry));
    assert(registry.size() == 3);
    scpi::Engine engine(registry, errors, responses);

    // Default values are visible through the SCPI query interface.
    engine.processScpiMessage("HEART:STAT?");
    assert(responses.response == "0");
    engine.processScpiMessage("HEART:PER?");
    assert(responses.response == "2");
    engine.processScpiMessage("HEART:DUTY?");
    assert(responses.response == "25");
    assert(errors.empty());

    // State accepts ON/OFF and reports the conventional numeric query value.
    clock.now = 1000;
    engine.processScpiMessage("HEART:STAT ON");
    assert(heartbeat.enabled());
    assert(led.on);
    engine.processScpiMessage("HEART:STAT?");
    assert(responses.response == "1");
    engine.processScpiMessage("HEART:STAT off");
    assert(!heartbeat.enabled());
    assert(!led.on);
    engine.processScpiMessage("HEART:STAT?");
    assert(responses.response == "0");

    // Period command uses seconds, with 1 ms resolution in the service.
    engine.processScpiMessage("HEART:PER 1.250");
    assert(heartbeat.periodMilliseconds() == 1250);
    engine.processScpiMessage("HEART:PER?");
    assert(responses.response == "1.25");
    engine.processScpiMessage("HEART:PER 0.001");
    assert(heartbeat.periodMilliseconds() == 1);
    engine.processScpiMessage("HEART:PER 86400");
    assert(heartbeat.periodMilliseconds() == 86400000UL);

    // A configured period does not change the currently active run.
    engine.processScpiMessage("HEART:PER 4");
    clock.now = 2000;
    engine.processScpiMessage("HEART:STAT ON");
    assert(heartbeat.enabled());
    assert(led.on);
    clock.now = 2500;
    heartbeat.service(clock.now); // active period is 4 s, adopted at enable
    assert(led.on);
    engine.processScpiMessage("HEART:PER 8");
    assert(heartbeat.periodMilliseconds() == 8000);
    heartbeat.service(2999); // 25% of the active 4 s period is still ON
    assert(led.on);
    heartbeat.service(3000); // exactly 1 s elapsed: the ON interval ends
    assert(!led.on);
    engine.processScpiMessage("HEART:STAT OFF");
    assert(!led.on);

    // Duty changes while enabled restart the current cycle immediately.
    clock.now = 4000;
    engine.processScpiMessage("HEART:STAT ON");
    assert(led.on);
    clock.now = 4100;
    engine.processScpiMessage("HEART:DUTY 50");
    assert(heartbeat.dutyCyclePercent() == 50);
    assert(led.on);
    heartbeat.service(6099);
    assert(led.on);
    heartbeat.service(6100);
    assert(!led.on);
    engine.processScpiMessage("HEART:DUTY?");
    assert(responses.response == "50");

    // 0% and 100% are valid boundary values.
    engine.processScpiMessage("HEART:DUTY 0");
    assert(!led.on);
    heartbeat.service(20000);
    assert(!led.on);
    engine.processScpiMessage("HEART:DUTY 100");
    assert(led.on);
    heartbeat.service(50000);
    assert(led.on);

    // Missing, malformed, out-of-range, and query-parameter errors are distinct.
    engine.processScpiMessage("HEART:PER");
    expectError(errors, -109);
    engine.processScpiMessage("HEART:PER abc");
    expectError(errors, -128);
    engine.processScpiMessage("HEART:PER 1.0001");
    expectError(errors, -128); // finer than the 1 ms service resolution
    engine.processScpiMessage("HEART:PER 86400.001");
    expectError(errors, -222);
    engine.processScpiMessage("HEART:DUTY 101");
    expectError(errors, -222);
    engine.processScpiMessage("HEART:DUTY 10.5");
    expectError(errors, -128);
    engine.processScpiMessage("HEART:STAT MAYBE");
    expectError(errors, -224);
    engine.processScpiMessage("HEART:STAT? extra");
    expectError(errors, -108);
    engine.processScpiMessage("HEART:PER? extra");
    expectError(errors, -108);
    engine.processScpiMessage("HEART:DUTY? extra");
    expectError(errors, -108);

    // Invalid commands do not alter the prior service configuration.
    assert(heartbeat.periodMilliseconds() == 8000);
    assert(heartbeat.dutyCyclePercent() == 100);
    assert(errors.empty());
    return 0;
}

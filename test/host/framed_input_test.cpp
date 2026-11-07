#include "transport/framed_input.h"
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

class RecordingEngine : public scpi::EngineInterface {
public:
    void processScpiMessage(const std::string& message) override {
        messages.push_back(message);
    }
    std::vector<std::string> messages;
};

int main() {
    RecordingEngine engine;
    scpi::transport::FramedInput input(
        engine, scpi::transport::MessageFramer::Config(64, 25));

    // CR terminates immediately; the LF in CRLF is an ignored empty message.
    input.feed('S', 1);
    input.feed('Y', 2);
    input.feed('S', 3);
    input.feed('\r', 4);
    assert(engine.messages.size() == 1);
    assert(engine.messages[0] == "SYS");
    input.feed('\n', 5);
    assert(engine.messages.size() == 1);

    // LF and CR each terminate independent messages.
    input.feed('A', 6);
    input.feed('\n', 7);
    input.feed('B', 8);
    input.feed('\r', 9);
    input.feed('\n', 10);
    assert(engine.messages.size() == 3);
    assert(engine.messages[1] == "A");
    assert(engine.messages[2] == "B");

    // A message without a terminator is delivered after inactivity timeout.
    input.feed('C', 11);
    input.feed('D', 12);
    assert(input.poll(36).type == scpi::transport::MessageFramer::NO_EVENT);
    assert(engine.messages.size() == 3);
    assert(input.poll(37).type == scpi::transport::MessageFramer::MESSAGE_READY);
    assert(engine.messages.size() == 4);
    assert(engine.messages[3] == "CD");

    // Empty terminators are deliberately not dispatched.
    input.feed('\n', 38);
    input.feed('\r', 39);
    input.feed('\n', 40);
    assert(engine.messages.size() == 4);

    std::cout << "FramedInput tests passed\n";
    return 0;
}

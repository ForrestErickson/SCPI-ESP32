#include "transport/message_framer.h"
#include <assert.h>
#include <stdint.h>
#include <iostream>

using scpi::transport::MessageFramer;

static void expectMessage(const MessageFramer::Result& r, const char* expected) {
    assert(r.type == MessageFramer::MESSAGE_READY);
    assert(r.message == expected);
}

int main() {
    // LF terminates; empty and repeated terminators do not dispatch.
    MessageFramer lf;
    assert(lf.feed('A', 1).type == MessageFramer::NO_EVENT);
    expectMessage(lf.feed('\n', 2), "A");
    assert(lf.feed('\n', 3).type == MessageFramer::NO_EVENT);

    // CR completes immediately; LF may arrive in a later receive call.
    MessageFramer crlf;
    crlf.feed('B', 10);
    expectMessage(crlf.feed('\r', 11), "B");
    assert(crlf.feed('\n', 12).type == MessageFramer::NO_EVENT);
    crlf.feed('C', 13);
    expectMessage(crlf.feed('\r', 14), "C");
    crlf.feed('D', 15); // A non-LF byte after CR begins the next message.
    expectMessage(crlf.feed('\n', 16), "D");

    // CR compatibility can be disabled; then CR is ordinary message data.
    MessageFramer no_cr(MessageFramer::Config(32, 0, false));
    no_cr.feed('E', 1);
    no_cr.feed('\r', 2);
    expectMessage(no_cr.feed('\n', 3), std::string("E\r").c_str());

    // Inactivity timeout, disabled timeout, and timer rollover.
    MessageFramer timed(MessageFramer::Config(32, 20, true));
    timed.feed('T', 100);
    assert(timed.poll(119).type == MessageFramer::NO_EVENT);
    expectMessage(timed.poll(120), "T");
    timed.feed('R', UINT32_MAX - 5);
    assert(timed.poll(10).type == MessageFramer::NO_EVENT); // 16 ms elapsed
    expectMessage(timed.poll(14), "R");                    // 20 ms elapsed

    // Timeout restarts on each byte.
    timed.feed('X', 200);
    timed.feed('Y', 215);
    assert(timed.poll(230).type == MessageFramer::NO_EVENT);
    expectMessage(timed.poll(235), "XY");

    // Overflow is reported once; bytes are discarded through the next boundary.
    MessageFramer bounded(MessageFramer::Config(3, 0, true));
    bounded.feed('1', 1); bounded.feed('2', 2); bounded.feed('3', 3);
    assert(bounded.feed('4', 4).type == MessageFramer::MESSAGE_OVERFLOW);
    assert(bounded.feed('5', 5).type == MessageFramer::NO_EVENT);
    assert(bounded.feed('\n', 6).type == MessageFramer::NO_EVENT);
    bounded.feed('O', 7);
    expectMessage(bounded.feed('\n', 8), "O");

    std::cout << "MessageFramer tests passed\n";
    return 0;
}

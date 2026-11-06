#include "scpi/error_queue.h"
#include <assert.h>
#include <iostream>

int main() {
    scpi::ErrorQueue queue(2);
    scpi::ScpiError error;

    assert(queue.empty());
    assert(!queue.pop(error));

    assert(queue.push(-100, "Command error"));
    assert(queue.push(-102, "Syntax error"));
    assert(!queue.push(-200, "Execution error"));
    assert(!queue.push(-224, "Illegal parameter"));
    assert(queue.size() == 3); // two errors plus one overflow indication

    assert(queue.pop(error));
    assert(error.code == -100);
    assert(error.message == "Command error");

    assert(queue.pop(error));
    assert(error.code == -102);
    assert(error.message == "Syntax error");

    assert(queue.pop(error));
    assert(error.code == -350);
    assert(error.message == "Queue overflow");
    assert(queue.empty());
    assert(!queue.pop(error));

    assert(queue.push(0, "test"));
    queue.clear();
    assert(queue.empty());

    std::cout << "ErrorQueue tests passed\n";
    return 0;
}

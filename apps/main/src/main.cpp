#include "workspace.h"
#include <iostream>

int main() {
    std::cout << std::boolalpha;
    try {
        workspace ws;
    } catch (const std::exception& e) {
        std::puts(e.what());
        return 1;
    } catch (...) {
        std::puts("caught exception in main");
        return 1;
    }
    return 0;
}

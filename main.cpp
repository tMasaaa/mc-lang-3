#include <iostream>

extern "C" {
    int fib(int);
}

int main() {
    // std::cout << "Call fib with 10: " << fib(10) << std::endl;

    std::cout << "Call fib with 45: " << fib(45) << std::endl;
    return 0;
}

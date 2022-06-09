# RingBuffer

A header-only lock-free ring buffer implemented with C++11.

This ring buffer is available for single-consumer multi-producer. DO NOT use for multi-consumer.

Please read comment of `Push()` and `Pop()` method carefully before using.

I only tested the correctness of this implementation. It is not guaranteed to run faster than mutex version.

## Test

Simply compile test.cpp with your compiler. The test code is written for clang/g++, you may need to change `asm volatile("" ::: "memory")` to `MemoryBarrier()` like this if you want to compile the test with MSVC:

```cpp
#include <winnt.h>

// other code ...

int main() {
    InitTestData();
    // asm volatile("" ::: "memory"); // Remove this
    MemoryBarrier(); // Add this
    std::thread consumer(ConsumerFunc);
    std::thread producer0(ProducerFunc, 0);
    std::thread producer1(ProducerFunc, 1);
    std::thread producer2(ProducerFunc, 2);
    std::thread producer3(ProducerFunc, 3);

    consumer.join();
    producer0.join();
    producer1.join();
    producer2.join();
    producer3.join();
    return 0;
}
```

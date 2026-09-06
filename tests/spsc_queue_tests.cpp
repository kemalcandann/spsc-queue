#include "spsc_queue.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

void test_basic_push_pop()
{
    SPSCQueue<std::uint64_t, 2> queue;

    assert(queue.empty());

    assert(queue.try_push(42));

    assert(!queue.empty());

    std::uint64_t value = 0;

    assert(queue.try_pop(value));

    assert(value == 42);
    assert(queue.empty());
}

void test_full_queue()
{
    SPSCQueue<std::uint64_t, 2> queue;

    assert(queue.try_push(1));
    assert(queue.try_push(2));

    assert(!queue.try_push(3));

    std::uint64_t value = 0;

    assert(queue.try_pop(value));
    assert(value == 1);

    assert(queue.try_push(3));
}

void test_wrap_around()
{
    SPSCQueue<std::uint64_t, 2> queue;

    for (size_t i = 0; i < 1'000'000; ++i) {
        while (!queue.try_push(i)) {
            std::this_thread::yield();
        }

        std::uint64_t value = 0;

        while (!queue.try_pop(value)) {
            std::this_thread::yield();
        }

        assert(value == i);
    }
}

void test_spsc_ordering()
{
    constexpr std::uint64_t Count = 10'000'000;

    SPSCQueue<std::uint64_t, 2> queue;

    std::thread producer([&] {
        for (size_t i = 0; i < Count; ++i) {
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&] {
        for (size_t expected = 0;
             expected < Count;
             ++expected)
        {
            std::uint64_t value = 0;

            while (!queue.try_pop(value)) {
                std::this_thread::yield();
            }

            assert(value == expected);
        }
    });

    producer.join();
    consumer.join();
}

} // namespace

int main()
{
    test_basic_push_pop();
    test_full_queue();
    test_wrap_around();
    test_spsc_ordering();

    std::cout
        << "All SPSC queue tests passed.\n";

    return 0;
}
#include "throughput_benchmark.h"
#include "cpu_affinity.h"
#include "spsc_queue.h"
#include "benchmark_config.h"

#include <barrier>
#include <chrono>
#include <cstdint>
#include <immintrin.h>
#include <memory>
#include <thread>

ThroughputResult run_throughput_once() {
    using Queue = SPSCQueue<
            std::uint64_t,
            benchmark::ThroughputQueueCapacity
        >;

    auto queue = std::make_unique<Queue>();

    std::barrier<> phase_barrier(2);

    std::chrono::steady_clock::time_point start_time;

    std::chrono::steady_clock::time_point end_time;


    // --------------------------------------------------------
    // Consumer
    // --------------------------------------------------------

    std::thread consumer([&] {

        pin_to_core_or_abort(benchmark::ConsumerCpu);

        std::uint64_t value = 0;

        // ---------------------------
        // Warm-up
        // ---------------------------

        for (size_t i = 0; i < benchmark::ThroughputWarmupOps; ++i) {
            while (!queue->try_pop(value));

            if (value != i)  {
                std::abort();
            }

        }

        // Synchronize with producer.
        phase_barrier.arrive_and_wait();

        // ---------------------------
        // Measurement
        // ---------------------------

        for (size_t i = 0; i < benchmark::ThroughputMeasuredOps; ++i){
            while (!queue->try_pop(value));

            if (value != i)  {
                std::abort();
            }            
        }

        // Consumer completion defines the end of the benchmark,
        // because this guarantees that all measured messages have
        // actually traversed the queue.
        end_time = std::chrono::steady_clock::now();
    });


    // --------------------------------------------------------
    // Producer
    // --------------------------------------------------------

    std::thread producer([&] {

        pin_to_core_or_abort(benchmark::ProducerCpu);

        // ---------------------------
        // Warm-up
        // ---------------------------

        for (size_t i = 0; i < benchmark::ThroughputWarmupOps; ++i) {
            while (!queue->try_push(i));
        }

        // Both threads completed warm-up.
        phase_barrier.arrive_and_wait();

        // ---------------------------
        // Measurement start
        // ---------------------------

        start_time = std::chrono::steady_clock::now();

        // ---------------------------
        // Measurement
        // ---------------------------

        for (size_t i = 0; i < benchmark::ThroughputMeasuredOps; ++i) {
            while (!queue->try_push(i));
        }
    });

    producer.join();
    consumer.join();

    const std::chrono::duration<double> elapsed = end_time - start_time;

    const double throughput =
        (
            static_cast<double>(
                benchmark::ThroughputMeasuredOps
            )
            /
            elapsed.count()
        )
        /
        1'000'000.0;

    return { throughput };
}

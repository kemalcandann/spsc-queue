#include "latency_benchmark.h"
#include "cpu_affinity.h"
#include "spsc_queue.h"
#include "tsc.h"
#include "benchmark_config.h"

#include <vector>
#include <algorithm>
#include <memory>
#include <barrier>
#include <thread>
#include <immintrin.h>


// percentile interpolation is intentionally avoided.
// We select the element at p * (N - 1).
std::uint64_t percentile(const std::vector<std::uint64_t>& samples, double p)
{
    const double position = p * static_cast<double>(samples.size() - 1);

    return samples[static_cast<std::size_t>(position)];
}


LatencyStats calculate_latency_stats(std::vector<std::uint64_t>& samples, std::uint64_t aux_mismatches)
{
    std::sort(samples.begin(), samples.end());

    long double sum = 0.0L;

    for (const auto sample : samples) {
        sum += static_cast<long double>(sample);
    }

    LatencyStats stats;

    stats.min = samples.front();

    stats.p50 = percentile(samples, 0.50);
    stats.p90 = percentile(samples, 0.90);
    stats.p99 = percentile(samples, 0.99);
    stats.p999 = percentile(samples, 0.999);
    stats.p9999 = percentile(samples, 0.9999);

    stats.max = samples.back();

    stats.mean = static_cast<double>(
                sum /
                static_cast<long double>(samples.size()));

    stats.aux_mismatches = aux_mismatches;

    return stats;
}


// ============================================================
// Latency benchmark
// ============================================================
//
// Producer CPU 2
//        |
//        | q_tx
//        v
// Consumer CPU 4
//        |
//        | q_rx
//        v
// Producer CPU 2
//
// What we measure:
//     Producer -> Consumer -> Producer
//
// This is ROUND-TRIP latency through TWO SPSC queues.
// It is NOT a direct measurement of one-way queue latency.
// ============================================================

LatencyStats run_latency_once() {
    using Queue = SPSCQueue<std::uint64_t, benchmark::LatencyQueueCapacity>;

    auto q_tx = std::make_unique<Queue>();

    auto q_rx = std::make_unique<Queue>();

    std::vector<std::uint64_t> samples;

    samples.reserve(benchmark::LatencyMeasuredSamples);

    // Reusable C++20 barrier.
    //
    // Both threads complete warm-up first and then
    // synchronize immediately before the measurement phase.
    std::barrier<> phase_barrier(2);

    std::uint64_t aux_mismatches = 0;

    // --------------------------------------------------------
    // Consumer
    // --------------------------------------------------------

    std::thread consumer([&] {

        pin_to_core_or_abort(benchmark::ConsumerCpu);

        std::uint64_t token = 0;

        // ---------------------------
        // Warm-up phase
        // ---------------------------

        for (size_t i = 0; i < benchmark::LatencyWarmupSamples; ++i) {
            while (!q_tx->try_pop(token)) {
                _mm_pause();
            }

            while (!q_rx->try_push(token)) {
                _mm_pause();
            }
        }

        // Wait until producer also completes warm-up.
        phase_barrier.arrive_and_wait();

        // ---------------------------
        // Measurement phase
        // ---------------------------

        for (size_t i = 0; i < benchmark::LatencyMeasuredSamples; ++i) {
            while (!q_tx->try_pop(token)) {
                _mm_pause();
            }

            while (!q_rx->try_push(token)) {
                _mm_pause();
            }
        }
    });


    // --------------------------------------------------------
    // Producer
    // --------------------------------------------------------

    pin_to_core_or_abort(benchmark::ProducerCpu);

    // ---------------------------
    // Warm-up phase
    // ---------------------------

    for (size_t i = 0; i < benchmark::LatencyWarmupSamples; ++i) {
        while (!q_tx->try_push(i)) {
            _mm_pause();
        }

        std::uint64_t dummy;

        while (!q_rx->try_pop(dummy)) {
            _mm_pause();
        }
    }

    // Both producer and consumer completed warm-up.
    phase_barrier.arrive_and_wait();

    // ---------------------------
    // Measurement phase
    // ---------------------------

    std::uint64_t dummy = 0;
    for (size_t i = 0; i < benchmark::LatencyMeasuredSamples; ++i) {
        const TscReading start = rdtscp_ordered();

        while (!q_tx->try_push(i)) {
            _mm_pause();
        }

        std::uint64_t dummy;

        while (!q_rx->try_pop(dummy)) {
            _mm_pause();
        }

        if (dummy != i) {
            std::abort();
        }        

        const TscReading end = rdtscp_ordered();

        // TSC_AUX is used as a migration detector.
        //
        // We do NOT assume aux == CPU number.
        // We only require it to stay unchanged.
        if (start.aux != end.aux) {
            ++aux_mismatches;
        }

        samples.push_back(end.tsc - start.tsc);
    }

    consumer.join();

    // One migration is enough to invalidate our strict latency
    // measurement assumptions.
    if (aux_mismatches != 0) {
        std::fprintf(
            stderr,
            "Latency benchmark invalid: "
            "%llu TSC_AUX changes detected.\n",
            static_cast<unsigned long long>(
                aux_mismatches
            )
        );

        std::abort();
    }

    return calculate_latency_stats(samples, aux_mismatches);
}
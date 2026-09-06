#pragma once

#include <cstddef>
#include <cstdint>

namespace benchmark {

    inline constexpr int ProducerCpu = 2;
    inline constexpr int ConsumerCpu = 4;

    inline constexpr int Repetitions = 5;

    // Latency
    inline constexpr std::size_t LatencyQueueCapacity = 2;
    inline constexpr std::uint64_t LatencyWarmupSamples = 1'000'000;
    inline constexpr std::uint64_t LatencyMeasuredSamples = 2'000'000;

    // Throughput
    inline constexpr std::size_t ThroughputQueueCapacity = 65'536;
    inline constexpr std::uint64_t ThroughputWarmupOps = 5'000'000;
    inline constexpr std::uint64_t ThroughputMeasuredOps = 50'000'000;

} // namespace benchmark
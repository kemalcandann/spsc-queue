#pragma once

#include <cstdint>

struct LatencyStats {
    std::uint64_t min;
    std::uint64_t p50;
    std::uint64_t p90;
    std::uint64_t p99;
    std::uint64_t p999;
    std::uint64_t p9999;
    std::uint64_t max;

    double mean;

    std::uint64_t aux_mismatches;
};

LatencyStats run_latency_once();


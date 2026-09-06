#pragma once

#include <cstdint>
#include <string>

struct TscReading {
    std::uint64_t tsc;
    unsigned aux;
};

struct TscCalibration {
    double ticks_per_second;
    double ns_per_tick;
};

TscReading rdtscp_ordered() noexcept;

TscCalibration calibrate_tsc();

double ticks_to_ns(std::uint64_t ticks, const TscCalibration& calibration) noexcept;

std::string format_latency(std::uint64_t ticks, const TscCalibration& calibration);
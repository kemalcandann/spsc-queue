#include "tsc.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <immintrin.h>
#include <stdexcept>
#include <thread>
#include <vector>
#include <cstdio>

TscReading rdtscp_ordered() noexcept
{
    unsigned aux = 0;

    const std::uint64_t tsc = __rdtscp(&aux);

    _mm_lfence();

    return { tsc, aux };
}

TscCalibration calibrate_tsc()
{
    using clock = std::chrono::steady_clock;

    constexpr int samples = 5;
    constexpr auto interval = std::chrono::milliseconds(100);

    std::vector<double> frequencies;
    frequencies.reserve(samples);

    for (size_t i = 0; i < samples; ++i) {
        const auto time_begin = clock::now();
        const auto tsc_begin = rdtscp_ordered();

        std::this_thread::sleep_for(interval);

        const auto tsc_end = rdtscp_ordered();
        const auto time_end = clock::now();

        if (tsc_begin.aux != tsc_end.aux) {
            // The calibration thread migrated between CPUs.
            // Retry this sample rather than using potentially inconsistent data.
            --i;
            std::this_thread::yield();
            continue;
        }

        const double seconds = std::chrono::duration<double>(time_end - time_begin).count();

        const double ticks = static_cast<double>(tsc_end.tsc - tsc_begin.tsc);
        frequencies.push_back(ticks / seconds);
    }

    std::sort(frequencies.begin(), frequencies.end());

    const double ticks_per_second = frequencies[frequencies.size() / 2];

    return { ticks_per_second, 1e9 / ticks_per_second };
}

double ticks_to_ns(std::uint64_t ticks, const TscCalibration& calibration) noexcept
{
    return static_cast<double>(ticks) * calibration.ns_per_tick;
}

std::string format_latency(std::uint64_t ticks, const TscCalibration& calibration)
{
    const double ns = ticks_to_ns(ticks, calibration);

    char buffer[64];

    if (ns < 1'000.0) {
        std::snprintf(buffer, sizeof(buffer),
                      "%llu TSC ticks (%.3f ns)",
                      static_cast<unsigned long long>(ticks), ns);
    } else if (ns < 1'000'000.0) {
        std::snprintf(buffer, sizeof(buffer),
                      "%llu TSC ticks (%.3f us)",
                      static_cast<unsigned long long>(ticks),
                      ns / 1'000.0);
    } else {
        std::snprintf(buffer, sizeof(buffer),
                      "%llu TSC ticks (%.3f ms)",
                      static_cast<unsigned long long>(ticks),
                      ns / 1'000'000.0);
    }

    return buffer;
}
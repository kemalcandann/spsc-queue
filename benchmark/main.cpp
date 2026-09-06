#include "latency_benchmark.h"
#include "throughput_benchmark.h"
#include "benchmark_config.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {
        
    double median(std::vector<double> values)
    {
        std::sort(values.begin(), values.end());

        return values[values.size() / 2];
    }

    std::uint64_t median_u64(std::vector<std::uint64_t> values)
    {
        std::sort(values.begin(), values.end());

        return values[values.size() / 2];
    }

} // namespace

int main() {

    std::cout
        << "=========================================================\n"
        << "           SPSC LOW-LATENCY BENCHMARK\n"
        << "=========================================================\n";

    std::cout
        << "Producer CPU          : "
        << benchmark::ProducerCpu << '\n';

    std::cout
        << "Consumer CPU          : "
        << benchmark::ConsumerCpu << '\n';

    std::cout
        << "Repetitions           : "
        << benchmark::Repetitions << '\n';

    std::cout
        << "Latency Queue Capacity: "
        << benchmark::LatencyQueueCapacity << '\n';

    std::cout
        << "Throughput Queue Size : "
        << benchmark::ThroughputQueueCapacity << '\n';

    std::cout << "=========================================================\n\n";


    // ========================================================
    // LATENCY
    // ========================================================

    std::cout << "[1] LATENCY / RTT\n";

    std::cout
        << "Warm-up samples       : "
        << benchmark::LatencyWarmupSamples << '\n';

    std::cout
        << "Measured samples      : "
        << benchmark::LatencyMeasuredSamples << '\n';

    std::cout << '\n';


    std::vector<LatencyStats> latency_runs;

    latency_runs.reserve(benchmark::Repetitions);


    for (size_t run = 1; run <= benchmark::Repetitions; ++run) {
        const LatencyStats stats = run_latency_once();

        latency_runs.push_back(stats);

        std::cout << "Run " << run << '\n';

        std::cout
            << "  Mean    : "
            << std::fixed
            << std::setprecision(1)
            << stats.mean
            << " TSC ticks\n";

        std::cout
            << "  P50     : "
            << stats.p50
            << '\n';

        std::cout
            << "  P90     : "
            << stats.p90
            << '\n';

        std::cout
            << "  P99     : "
            << stats.p99
            << '\n';

        std::cout
            << "  P99.9   : "
            << stats.p999
            << '\n';

        std::cout
            << "  P99.99  : "
            << stats.p9999
            << '\n';

        std::cout
            << "  Max     : "
            << stats.max
            << '\n';

        std::cout
            << '\n';
    }


    // --------------------------------------------------------
    // Latency summary
    //
    // IMPORTANT:
    // These are medians of the per-run statistics.
    // This is NOT a pooled global percentile distribution.
    // --------------------------------------------------------

    std::vector<double> means;

    std::vector<std::uint64_t> p50s;
    std::vector<std::uint64_t> p90s;
    std::vector<std::uint64_t> p99s;
    std::vector<std::uint64_t> p999s;
    std::vector<std::uint64_t> p9999s;
    std::vector<std::uint64_t> maxs;


    for (const auto& stats : latency_runs) {
        means.push_back(stats.mean);

        p50s.push_back(stats.p50);
        p90s.push_back(stats.p90);
        p99s.push_back(stats.p99);
        p999s.push_back(stats.p999);
        p9999s.push_back(stats.p9999);

        maxs.push_back(stats.max);
    }


    std::cout
        << "Latency summary "
        << "(median across runs)\n";

    std::cout
        << "  Mean    : "
        << median(means)
        << " TSC ticks\n";

    std::cout
        << "  P50     : "
        << median_u64(p50s)
        << '\n';

    std::cout
        << "  P90     : "
        << median_u64(p90s)
        << '\n';

    std::cout
        << "  P99     : "
        << median_u64(p99s)
        << '\n';

    std::cout
        << "  P99.9   : "
        << median_u64(p999s)
        << '\n';

    std::cout
        << "  P99.99  : "
        << median_u64(p9999s)
        << '\n';

    std::cout
        << "  Max     : "
        << median_u64(maxs)
        << '\n';

    std::cout
        << '\n';


    // ========================================================
    // THROUGHPUT
    // ========================================================

    std::cout
        << "[2] THROUGHPUT\n";

    std::cout
        << "Warm-up operations   : "
        << benchmark::ThroughputWarmupOps
        << '\n';

    std::cout
        << "Measured operations  : "
        << benchmark::ThroughputMeasuredOps
        << '\n';

    std::cout << '\n';


    std::vector<double> throughput_runs;

    throughput_runs.reserve(benchmark::Repetitions);


    for (size_t run = 1; run <= benchmark::Repetitions; ++run) {
        const ThroughputResult result = run_throughput_once();

        throughput_runs.push_back(result.million_messages_per_second);

        std::cout
            << "Run " << run << ": "
            << std::fixed
            << std::setprecision(2)
            << result.million_messages_per_second
            << " Mmsg/s\n";
    }


    std::cout
        << '\n'
        << "Throughput summary "
        << "(median across runs): "
        << std::fixed
        << std::setprecision(2)
        << median(throughput_runs)
        << " Mmsg/s\n";


    // ========================================================

    std::cout
        << "\n=========================================================\n"
        << "IMPORTANT:\n"
        << "Latency values are TSC ticks, not CPU core cycles.\n"
        << "TSC_AUX remained constant during each measured run.\n"
        << "=========================================================\n";

    return 0;
}
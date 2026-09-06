#pragma once

struct ThroughputResult {
    double million_messages_per_second;
};

ThroughputResult run_throughput_once();
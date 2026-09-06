# SPSC Lock-Free Queue in C++

A fixed-capacity, cache-aware Single-Producer / Single-Consumer (SPSC) queue implemented in modern C++20.

The project focuses on understanding the fundamentals behind low-latency concurrent data structures:
- `std::atomic`
- C++ memory ordering
- acquire/release synchronization
- ring-buffer indexing
- cache-line separation and false sharing
- manual object lifetime management
- CPU affinity
- TSC-based latency measurement
- percentile-based benchmark analysis

## SPSC Design

The queue is designed for exactly one producer thread and one consumer thread.

The producer exclusively owns `tail_`, while the consumer exclusively owns `head_`. Because there is only one writer for each index, the implementation does not need CAS-based coordination such as `compare_exchange`.

The queue uses monotonically increasing head/tail counters and maps them to ring-buffer slots with:
```text
index & (capacity - 1)
```

Therefore the capacity is required to be a power of two.

## Memory Ordering

The implementation intentionally uses different memory orders depending on ownership and synchronization requirements:
- `relaxed` for owner-local index accesses
- `release` when publishing a newly constructed element or a newly freed slot
- `acquire` when observing the remote index before using the corresponding memory

The important synchronization paths are:

```text
Producer                         Consumer

construct T
    |
    | tail.store(release)
    v
                            tail.load(acquire)
                                  |
                                  v
                               read T

and:

```text
Consumer                         Producer

destroy T
    |
    | head.store(release)
    v
                            head.load(acquire)
                                  |
                                  v
                              reuse slot
```

The queue also caches the remote index locally so that an atomic load of the remote cache line is only required when the cached state indicates that the queue may be full or empty.

## Benchmark

The benchmark contains separate tests for:
- Latency / RTT using two SPSC queues in a ping-pong configuration
- Throughput using one producer and one consumer in a streaming configuration

The benchmark includes:
- CPU affinity
- warm-up phases
- repeated runs
- ordered TSC reads
- TSC_AUX consistency checking
- P50 / P90 / P99 / P99.9 / P99.99 / Max latency statistics

Benchmark results are intentionally documented separately from the implementation details.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Run:
```bash
./build/spsc_benchmark
```

### Tests can be enabled with:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Project Structure

```text
.
├── CMakeLists.txt
├── include/
│   ├── spsc_queue.h
│   ├── cpu_affinity.h
│   └── tsc.h
├── src/
│   ├── cpu_affinity.cpp
│   └── tsc.cpp
├── benchmark/
│   ├── benchmark_config.h
│   ├── latency_benchmark.h
│   ├── latency_benchmark.cpp
│   ├── throughput_benchmark.h
│   ├── throughput_benchmark.cpp
│   └── main.cpp
└── tests/
    ├── CMakeLists.txt
    └── spsc_queue_tests.cpp
```

## Benchmark Results

The benchmark was run with the producer and consumer pinned to CPUs 2 and 4.

Latency / RTT

TSC frequency: 3.792896 GHz

P50     : 646 TSC ticks (170.324 ns)
P90     : 722 TSC ticks (190.362 ns)
P99     : 988 TSC ticks (260.496 ns)
P99.9   : 2850 TSC ticks (751.431 ns)
P99.99  : 10374 TSC ticks (2.735 us)
Max     : 156446 TSC ticks (41.249 us)

Latency is measured as round-trip time using two SPSC queues in a ping-pong configuration.

Throughput

Median across 5 runs: 153.6 Mmsg/s

Warm-up operations: 5,000,000

Measured operations: 50,000,000

These results are a baseline for the specific hardware, operating system, compiler, CPU affinity, and benchmark configuration. They should not be interpreted as universal or production-HFT performance claims.


## Notes

This project is primarily a learning and experimentation project focused on low-latency C++ and lock-free programming. The benchmark numbers should be interpreted together with their exact hardware, compiler, operating-system, and benchmark configuration.

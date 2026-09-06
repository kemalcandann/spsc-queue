#include "cpu_affinity.h"

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <sched.h>

void pin_to_core_or_abort(int cpu)
{
    if (cpu < 0 || cpu >= CPU_SETSIZE) {
        std::fprintf(
            stderr,
            "Invalid CPU id: %d\n",
            cpu
        );
        std::abort();
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    const int rc =
        pthread_setaffinity_np(
            pthread_self(),
            sizeof(cpuset),
            &cpuset
        );

    if (rc != 0) {
        std::fprintf(
            stderr,
            "pthread_setaffinity_np failed: cpu=%d rc=%d\n",
            cpu,
            rc
        );
        std::abort();
    }

    const int actual_cpu = sched_getcpu();

    if (actual_cpu != cpu) {
        std::fprintf(
            stderr,
            "Affinity verification failed: requested=%d actual=%d\n",
            cpu,
            actual_cpu
        );
        std::abort();
    }
}
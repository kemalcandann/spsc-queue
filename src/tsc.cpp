#include "tsc.h"

#include <immintrin.h>

TscReading rdtscp_ordered() noexcept
{
    unsigned aux = 0;

    const std::uint64_t tsc = __rdtscp(&aux);

    _mm_lfence();

    return {
        tsc,
        aux
    };
}
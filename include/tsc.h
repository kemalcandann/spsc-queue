#pragma once

#include <cstdint>

struct TscReading {
    std::uint64_t tsc;
    unsigned aux;
};

TscReading rdtscp_ordered() noexcept;
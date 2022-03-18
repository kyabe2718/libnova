#pragma once

#if __has_include(<coroutine>)
#include <coroutine>
namespace nova {
namespace coro = std;
}
#elif __has_include(<experimental/coroutine>)

#include <experimental/coroutine>

namespace nova {
namespace coro = std::experimental::coroutines_v1;
}
#endif

#if defined(__APPLE__)
#define NOVA_ATOMIC_WAIT_NOTIFY_AVAILABLE 0
#else
#define NOVA_ATOMIC_WAIT_NOTIFY_AVAILABLE 1
#endif

#if __has_include(<numa.h>)
#define NOVA_NUMA_AVAILABLE 1
#else
#define NOVA_NUMA_AVAILABLE 0
#endif
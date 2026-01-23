#ifndef PROVERAPI_HPP
#define PROVERAPI_HPP

#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <semaphore>
#include "singleprover.hpp"

using namespace Pistache;

// Maximum number of concurrent proof requests allowed.
//
// Rationale: proof generation is effectively serialized by a mutex in SingleProver::prove().
// With FE timing out around 15s and typical timings of ~0.8s witness + ~2s proof per request,
// we cap in-flight work to keep the queueing tail within the FE timeout under moderate variance.
constexpr int DEFAULT_MAX_CONCURRENT_REQUESTS = 6;

// RAII guard for releasing an acquired semaphore on scope exit
class SemaphoreReleaseGuard {
    std::counting_semaphore<DEFAULT_MAX_CONCURRENT_REQUESTS>& sem;

public:
    explicit SemaphoreReleaseGuard(std::counting_semaphore<DEFAULT_MAX_CONCURRENT_REQUESTS>& s)
        : sem(s) {}

    ~SemaphoreReleaseGuard() {
        sem.release();
    }

    // Prevent copying
    SemaphoreReleaseGuard(const SemaphoreReleaseGuard&) = delete;
    SemaphoreReleaseGuard& operator=(const SemaphoreReleaseGuard&) = delete;
};

class ProverAPI {
    SingleProver& prover;

    // Limit concurrent proof requests to prevent DoS attacks
    std::counting_semaphore<DEFAULT_MAX_CONCURRENT_REQUESTS> request_limit;

public:
    explicit ProverAPI(SingleProver& _prover,
                      int max_concurrent_requests = DEFAULT_MAX_CONCURRENT_REQUESTS)
        : prover(_prover),
          request_limit(max_concurrent_requests) {}

    void postInput(const Rest::Request& request, Http::ResponseWriter response);
};

#endif // PROVERAPI_HPP

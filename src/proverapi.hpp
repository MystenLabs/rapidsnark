#ifndef PROVERAPI_HPP
#define PROVERAPI_HPP

#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <semaphore>
#include "singleprover.hpp"

using namespace Pistache;

// Maximum number of concurrent proof requests allowed
constexpr int DEFAULT_MAX_CONCURRENT_REQUESTS = 10;

// RAII guard for semaphore - ensures release on scope exit
class SemaphoreGuard {
    std::counting_semaphore<DEFAULT_MAX_CONCURRENT_REQUESTS>& sem;

public:
    explicit SemaphoreGuard(std::counting_semaphore<DEFAULT_MAX_CONCURRENT_REQUESTS>& s) 
        : sem(s) {}
    
    ~SemaphoreGuard() { 
        sem.release(); 
    }
    
    // Prevent copying
    SemaphoreGuard(const SemaphoreGuard&) = delete;
    SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;
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

#include "proverapi.hpp"
#include "nlohmann/json.hpp"
#include "logger.hpp"
#include <chrono>
#include <sstream>

using namespace Pistache;
using json = nlohmann::json;

void ProverAPI::postInput(const Rest::Request& request, Http::ResponseWriter response) {
    // Try to acquire semaphore - reject immediately if at capacity
    if (!request_limit.try_acquire()) {
        LOG_INFO("Server at capacity, rejecting request");
        response.send(Http::Code::Service_Unavailable, 
                     "Server at capacity, please retry later",
                     MIME(Text, Plain));
        return;
    }
    
    // Start timing the request
    auto request_start = std::chrono::steady_clock::now();
    
    // RAII guard ensures semaphore is released even if exception is thrown
    SemaphoreGuard guard(request_limit);
    
    try {
        // Generate proof
        json j = prover.startProve(request.body());
        LOG_DEBUG(j.dump().c_str());
        
        // Log e2e timing
        auto request_end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            request_end - request_start).count();
        
        std::ostringstream oss;
        oss << "Request completed in " << duration << "ms";
        std::string log_msg = oss.str();
        LOG_INFO(log_msg);
        
        response.send(Http::Code::Ok, j.dump(), MIME(Application, Json));
        
    } catch (const std::exception& e) {
        // Log error and timing
        auto request_end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            request_end - request_start).count();
        
        auto errString = e.what();
        LOG_ERROR(errString);
        
        std::ostringstream oss;
        oss << "Request failed after " << duration << "ms";
        std::string log_msg = oss.str();
        LOG_INFO(log_msg);
        
        response.send(Http::Code::Bad_Request, errString);
    }
}

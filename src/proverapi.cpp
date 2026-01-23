#include "proverapi.hpp"
#include "nlohmann/json.hpp"
#include "logger.hpp"
#include <atomic>
#include <chrono>
#include <sstream>

using namespace Pistache;
using json = nlohmann::json;

void ProverAPI::postInput(const Rest::Request& request, Http::ResponseWriter response) {
    static std::atomic<uint64_t> next_request_id{0};
    static std::atomic<int> in_flight{0};

    const auto id = next_request_id.fetch_add(1, std::memory_order_relaxed) + 1;

    // Try to acquire semaphore - reject immediately if at capacity
    if (!request_limit.try_acquire()) {
        LOG_INFO("Server at capacity, rejecting request id=" + std::to_string(id));
        response.send(Http::Code::Service_Unavailable,
                     "Server at capacity, please retry later",
                     MIME(Text, Plain));
        return;
    }

    const int in_flight_now = in_flight.fetch_add(1, std::memory_order_relaxed) + 1;
    LOG_INFO("Accepted request id=" + std::to_string(id) + " in_flight=" + std::to_string(in_flight_now));

    // Start timing the request
    auto request_start = std::chrono::steady_clock::now();

    bool success = true;
    try {
        // Generate proof
        json j;
        {
            SemaphoreReleaseGuard guard(request_limit);
            j = prover.startProve(request.body());
        }
        LOG_DEBUG(j.dump().c_str());

        response.send(Http::Code::Ok, j.dump(), MIME(Application, Json));

    } catch (const std::exception& e) {
        success = false;
        auto errString = e.what();
        LOG_ERROR(errString);
        response.send(Http::Code::Bad_Request, errString);
    }

    // Log e2e timing
    auto request_end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        request_end - request_start).count();

    const int in_flight_after = in_flight.fetch_sub(1, std::memory_order_relaxed) - 1;

    std::ostringstream oss;
    oss << "Request id=" << id << " " << (success ? "completed" : "failed")
        << " in " << duration << "ms"
        << " in_flight=" << in_flight_after;
    std::string log_msg = oss.str();
    LOG_INFO(log_msg);
}

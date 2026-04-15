#include <cstdlib> // for getenv
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include "proverapi.hpp"
#include "singleprover.hpp"
#include "logger.hpp"

using namespace CPlusPlusLogging;
using namespace Pistache;
using namespace Pistache::Rest;

void printUsage() {
    std::cerr << "Invalid usage\n";
    std::cerr << "Either use command line args: ./proverServer <zkLogin.zkey> <path_to_graph.bin>\n";
    std::cerr << "Or set ZKEY and WITNESS_GRAPH env variables and call ./proverServer\n";
}

int main(int argc, char **argv) {
    Logger::getInstance()->enableConsoleLogging();
    Logger::getInstance()->updateLogLevel(LOG_LEVEL_INFO);
    LOG_INFO("Initializing server...");

    int port = 8080;

    // The path to the zkey file, e.g., "/app/zkLogin.zkey"
    std::string zkeyFilePath;
    // The path to the circom-witnesscalc graph binary, e.g., "/app/zkLogin.bin"
    std::string graphFilePath;

    if (argc == 3) {
        zkeyFilePath = argv[1];
        graphFilePath = argv[2];
    } else if (argc == 1) {
        LOG_INFO("Reading from environment variables ZKEY and WITNESS_GRAPH");
        if (const char* x = getenv("ZKEY")) {
            zkeyFilePath = x;
        } else {
            printUsage();
            return -1;
        }

        if (const char* x = getenv("WITNESS_GRAPH")) {
            graphFilePath = x;
        } else {
            printUsage();
            return -1;
        }
    } else {
        printUsage();
        return -1;
    }

    SingleProver prover(zkeyFilePath, graphFilePath);
    ProverAPI proverAPI(prover);
    Address addr(Ipv4::any(), Port(port));

    const auto processor_count = std::thread::hardware_concurrency();
    std::ostringstream oss;
    oss << processor_count << " concurrent threads are supported";
    std::string numThreadsMsg = oss.str();
    LOG_INFO(numThreadsMsg);

    auto opts = Http::Endpoint::options().threads(processor_count).maxRequestSize(128000000);
    Http::Endpoint server(addr);
    server.init(opts);
    Router router;
    Routes::Post(router, "/input", Routes::bind(&ProverAPI::postInput, &proverAPI));
    server.setHandler(router.handler());
    std::string serverReady("Server ready on port " + std::to_string(port) + "...");
    LOG_INFO(serverReady);
    server.serve();
}

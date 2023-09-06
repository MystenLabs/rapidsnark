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
    std::cerr << "Either use command line args: ./proverServer <zkLogin.zkey> <path_to_binaries>\n";
    std::cerr << "Or set ZKEY and WITNESS_BINARIES env variables and call ./proverServer\n";
}

int main(int argc, char **argv) {
    Logger::getInstance()->enableConsoleLogging();
    Logger::getInstance()->updateLogLevel(LOG_LEVEL_INFO);
    LOG_INFO("Initializing server...");

    int port = 8080;
    LOG_INFO("Starting at port 8080");

    std::string zkeyFileLoc;
    std::string binariesFolderLoc;

    if (argc == 3) {
        zkeyFileLoc = argv[1];
        binariesFolderLoc = argv[2];
    } else if (argc == 1) {
        LOG_INFO("Reading from environment variables ZKEY and WITNESS_BINARIES");
        if (const char* x = getenv("ZKEY")) {
            zkeyFileLoc = x;
        } else {
            printUsage();
            return -1;
        }

        if (const char* x = getenv("WITNESS_BINARIES")) {
            binariesFolderLoc = x;
        } else {
            printUsage();
            return -1;
        }
    } else {
        printUsage();
        return -1;
    }

    SingleProver prover(zkeyFileLoc, binariesFolderLoc);
    ProverAPI proverAPI(prover);
    Address addr(Ipv4::any(), Port(port));

    auto opts = Http::Endpoint::options().threads(1).maxRequestSize(128000000);
    Http::Endpoint server(addr);
    server.init(opts);
    Router router;
    Routes::Post(router, "/input", Routes::bind(&ProverAPI::postInput, &proverAPI));
    server.setHandler(router.handler());
    std::string serverReady("Server ready on port " + std::to_string(port) + "...");
    LOG_INFO(serverReady);
    server.serve();
}

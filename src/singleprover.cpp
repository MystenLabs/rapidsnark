#include <fstream>

#include <singleprover.hpp>
#include "fr.hpp"

#include "logger.hpp"
#include "wtns_utils.hpp"

extern "C" {
#include "graph_witness.h"
}

SingleProver::SingleProver(std::string zkeyFilePath, std::string graphFilePath)
    : graphHandle(nullptr) {
    LOG_INFO("SingleProver::SingleProver begin");
    auto t0 = std::chrono::steady_clock::now();

    std::ifstream graphFile(graphFilePath, std::ios::binary | std::ios::ate);
    if (! graphFile.good()) {
        throw std::invalid_argument("cannot find the witness graph file at " + graphFilePath);
    }
    std::streamsize graphSize = graphFile.tellg();
    graphFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> graphBytes(graphSize);
    if (! graphFile.read(reinterpret_cast<char*>(graphBytes.data()), graphSize)) {
        throw std::runtime_error("failed to read witness graph file at " + graphFilePath);
    }

    auto t_prep0 = std::chrono::steady_clock::now();
    gw_status_t prepStatus = {OK, nullptr};
    int rc = gw_prepare_graph(graphBytes.data(), graphBytes.size(), &graphHandle, &prepStatus);
    if (rc != 0) {
        std::string msg = "Failed to prepare witness graph";
        if (prepStatus.error_msg != nullptr) {
            msg += ": ";
            msg += prepStatus.error_msg;
        }
        gw_free_status(&prepStatus);
        throw std::invalid_argument(msg);
    }
    gw_free_status(&prepStatus);
    auto t_prep1 = std::chrono::steady_clock::now();
    auto prep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_prep1 - t_prep0).count();
    LOG_INFO("Graph parsed and cached in " + std::to_string(prep_ms) + "ms");

    std::ifstream file3(zkeyFilePath.c_str());
    if (! file3.good()) {
        throw std::invalid_argument("cannot find the zkey file at " + zkeyFilePath);
    }

    mpz_init(altBbn128r);
    mpz_set_str(altBbn128r, "21888242871839275222246405745257275088548364400416034343698204186575808495617", 10);

    zKey = BinFileUtils::openExisting(zkeyFilePath, "zkey", 1);
    zkHeader = ZKeyUtils::loadHeader(zKey.get());

    std::string proofStr;
    if (mpz_cmp(zkHeader->rPrime, altBbn128r) != 0) {
        throw std::invalid_argument("zkey curve not supported" );
    }

    prover = Groth16::makeProver<AltBn128::Engine>(
        zkHeader->nVars,
        zkHeader->nPublic,
        zkHeader->domainSize,
        zkHeader->nCoefs,
        zkHeader->vk_alpha1,
        zkHeader->vk_beta1,
        zkHeader->vk_beta2,
        zkHeader->vk_delta1,
        zkHeader->vk_delta2,
        zKey->getSectionData(4),    // Coefs
        zKey->getSectionData(5),    // pointsA
        zKey->getSectionData(6),    // pointsB1
        zKey->getSectionData(7),    // pointsB2
        zKey->getSectionData(8),    // pointsC
        zKey->getSectionData(9)     // pointsH1
    );

    auto t1 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::string output("SingleProver::SingleProver initialized from zkey and witness graph in " + std::to_string(duration) + "ms");
    LOG_INFO(output);
}

SingleProver::~SingleProver()
{
    if (graphHandle != nullptr) {
        gw_free_graph(graphHandle);
        graphHandle = nullptr;
    }
    mpz_clear(altBbn128r);
}

json SingleProver::startProve(std::string input)
{
    LOG_INFO("SingleProver::startProve begin");

    auto t0 = std::chrono::steady_clock::now();
    LOG_DEBUG(input);

    void *wtnsBuffer = nullptr;
    size_t wtnsLen = 0;
    gw_status_t status = {OK, nullptr};

    int rc = gw_calc_witness_prepared(
        graphHandle,
        input.c_str(),
        &wtnsBuffer, &wtnsLen,
        &status);

    // Note: v0.3.0 of circom-witnesscalc sets status.error_msg on both success
    // and failure (see lib.rs line 127), so we rely on rc and always free.
    if (rc != 0) {
        std::string msg = "Witness generation failed";
        if (status.error_msg != nullptr) {
            msg += ": ";
            msg += status.error_msg;
        }
        gw_free_status(&status);
        throw std::invalid_argument(msg);
    }
    gw_free_status(&status);

    auto wtns = BinFileUtils::openFromBuffer(wtnsBuffer, wtnsLen, "wtns", 2);
    free(wtnsBuffer);
    auto wtnsHeader = WtnsUtils::loadHeader(wtns.get());
    if (mpz_cmp(wtnsHeader->prime, altBbn128r) != 0) {
        throw std::invalid_argument("Different wtns curve");
    }

    AltBn128::FrElement *wtnsData = (AltBn128::FrElement *)wtns->getSectionData(2);
    auto t1 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::string output("Witness generation finished in " + std::to_string(duration) + "ms");
    LOG_INFO(output);

    return prove(wtnsData)->toJson();
}

std::unique_ptr<Groth16::Proof<AltBn128::Engine>> SingleProver::prove(AltBn128::FrElement *wtnsData) {
    // The mutex is set because the performance of prover->prove degrades significantly
    //  when multiple instances are run in parallel.
    auto t_lock0 = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> guard(mtx);
    auto t_lock1 = std::chrono::steady_clock::now();

    auto lock_wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_lock1 - t_lock0).count();
    std::string log_msg = "SingleProver::prove mutex wait " + std::to_string(lock_wait_ms) + "ms";
    LOG_INFO(log_msg);
    LOG_INFO("SingleProver::prove begin");

    auto t1 = std::chrono::steady_clock::now();
    auto proof = prover->prove(wtnsData);
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::string output = "Proof generation finished in " + std::to_string(duration) + "ms";
    LOG_INFO(output);
    return proof;
}

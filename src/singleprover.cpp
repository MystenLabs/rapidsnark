#include <unistd.h>

#include <singleprover.hpp>
#include "fr.hpp"

#include "logger.hpp"
#include "wtns_utils.hpp"

SingleProver::SingleProver(std::string zkeyFilePath, std::string binariesFolderPath) {
    LOG_INFO("SingleProver::SingleProver begin");
    auto t0 = std::chrono::steady_clock::now();

    witnessBinaryFilePath = binariesFolderPath + "/zkLogin";
    std::string witnessDatFilePath = binariesFolderPath + "/zkLogin.dat";

    // Check if all the files exist
    std::ifstream file1(witnessBinaryFilePath.c_str());
    if (! file1.good()) {
        throw std::invalid_argument("cannot find the file zkLogin at " + witnessBinaryFilePath);
    }

    std::ifstream file2(witnessDatFilePath.c_str());
    if (! file2.good()) {
        throw std::invalid_argument("cannot find the file zkLogin.dat at " + witnessDatFilePath);
    }

    std::ifstream file3(zkeyFilePath.c_str());
    if (! file3.good()) {
        throw std::invalid_argument("cannot find the file zkLogin.zkey at " + zkeyFilePath);
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
    std::string output("SingleProver::SingleProver initialized from zkey in " + std::to_string(duration) + "ms");
    LOG_INFO(output);
}

SingleProver::~SingleProver()
{
    mpz_clear(altBbn128r);
}

// Function to generate a unique temporary file name
std::string generateTempFileName() {
    char templateName[] = "/tmp/zklogin_XXXXXX";
    int fd;
    // TODO: See if C++ stdlib has a better alternative
    // Note: The main req for us is that it returns a unique file name
    if ((fd = mkstemp(templateName)) == -1) {
        throw std::runtime_error("Failed to create a unique temporary file name");
    }
    close(fd);
    return templateName;
}

json SingleProver::startProve(std::string input)
{
    LOG_INFO("SingleProver::startProve begin");

    auto t0 = std::chrono::steady_clock::now();
    // TODO: Do not log PII. prover->prove also logs the ZK proof, which we might not want to log.
    LOG_DEBUG(input);

    json j = json::parse(input);
    std::string inputFileName = generateTempFileName();
    std::ofstream file(inputFileName);
    file << j;
    file.close();

    std::string witnessFileName = generateTempFileName();
    std::string command(witnessBinaryFilePath + " " + inputFileName + " " + witnessFileName);
    LOG_INFO(command);
    std::array<char, 128> buffer;
    std::string result;

    int returnCode = std::system(command.c_str());

    // TODO: Early fail with an unexpected returnCode
    if (returnCode != 0) {
        LOG_INFO("Unexpected returnCode");
        auto str = std::to_string(returnCode);
        LOG_INFO(str);
    }

    auto wtns = BinFileUtils::openExisting(witnessFileName, "wtns", 2);
    auto wtnsHeader = WtnsUtils::loadHeader(wtns.get());
    if (mpz_cmp(wtnsHeader->prime, altBbn128r) != 0) {
        throw std::invalid_argument( "different wtns curve" );
    }

    AltBn128::FrElement *wtnsData = (AltBn128::FrElement *)wtns->getSectionData(2);
    auto t1 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::string output("Witness generation finished in " + std::to_string(duration) + "ms");
    LOG_INFO(output);

    unlink(witnessFileName.c_str());
    unlink(inputFileName.c_str());

    return prove(wtnsData)->toJson();
}

std::unique_ptr<Groth16::Proof<AltBn128::Engine>> SingleProver::prove(AltBn128::FrElement *wtnsData) {
    // The mutex is set because the performance of prover->prove degrades significantly
    //  when multiple instances are run in parallel.
    std::lock_guard<std::mutex> guard(mtx);
    LOG_INFO("SingleProver::prove begin");

    auto t1 = std::chrono::steady_clock::now();
    auto proof = prover->prove(wtnsData);
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::string output = "Proof generation finished in " + std::to_string(duration) + "ms";
    LOG_INFO(output);
    return proof;
}

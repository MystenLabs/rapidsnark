#ifndef SINGLEPROVER_H
#define SINGLEPROVER_H

#include <mutex>
#include <string>
#include <vector>

#include "alt_bn128.hpp"
#include "groth16.hpp"
#include "zkey_utils.hpp"

/**
 * One-shot prover server
 *
 * 1. At server initialization, it reads a zkey file and the circom-witnesscalc
 *    graph binary, and instantiates a prover.
 * 2. During its run, it receives a JSON input file
 *      2a) First it computes a witness via the circom-witnesscalc library
 *      2b) Next it generates the ZKP
 */
class SingleProver {

    std::vector<uint8_t> graphData;
    mpz_t altBbn128r;
    std::unique_ptr<Groth16::Prover<AltBn128::Engine> > prover;
    std::unique_ptr<ZKeyUtils::Header> zkHeader;
    std::unique_ptr<BinFileUtils::BinFile> zKey;
    std::mutex mtx;

    std::unique_ptr<Groth16::Proof<AltBn128::Engine>> prove(AltBn128::FrElement *wtnsData);

public:
    SingleProver(std::string zkeyFilePath, std::string graphFilePath);
    ~SingleProver();
    json startProve(std::string input);
};

#endif // SINGLEPROVER_H

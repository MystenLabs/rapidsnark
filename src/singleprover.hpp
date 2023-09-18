#ifndef SINGLEPROVER_H
#define SINGLEPROVER_H

#include <mutex>
#include <string>

#include "alt_bn128.hpp"
#include "groth16.hpp"
#include "zkey_utils.hpp"

/** 
 * One-shot prover server
 * 
 * 1. At server initialization, it reads a zkey file and instantiates a prover
 * 2. During its run, it receives a JSON input file 
 *      2a) First it computes a witness
 *      2b) Next it generates the ZKP
 */
class SingleProver {

    std::string binariesFolderPath; // where to find zkLogin and zkLogin.dat
    std::string witnessBinaryFilePath;
    mpz_t altBbn128r;
    std::unique_ptr<Groth16::Prover<AltBn128::Engine> > prover;
    std::unique_ptr<ZKeyUtils::Header> zkHeader;
    std::unique_ptr<BinFileUtils::BinFile> zKey;
    std::mutex mtx;

    json prove(AltBn128::FrElement *wtnsData);

public: 
    SingleProver(std::string zkeyFilePath, std::string binariesFolderPath);
    ~SingleProver();
    json startProve(std::string input);
};

#endif // SINGLEPROVER_H

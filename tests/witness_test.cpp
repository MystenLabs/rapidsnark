// Standalone test for the witness-calc integration.
//
// Exercises the exact new code path used by SingleProver (load .bin graph ->
// gw_calc_witness -> BinFileUtils::openFromBuffer) without requiring the full
// Pistache/rapidsnark server to build. Writes the resulting witness to disk so
// `snarkjs wtns check <r1cs> <wtns>` can verify it against the circuit.
//
// Usage: ./witness_test <graph.bin> <inputs.json> <witness.wtns>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "binfile_utils.hpp"

extern "C" {
#include "graph_witness.h"
}

static std::vector<uint8_t> readBinary(const std::string &path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.good()) throw std::runtime_error("cannot open " + path);
    std::streamsize n = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(n);
    if (!f.read(reinterpret_cast<char *>(buf.data()), n))
        throw std::runtime_error("failed to read " + path);
    return buf;
}

static std::string readText(const std::string &path) {
    std::ifstream f(path);
    if (!f.good()) throw std::runtime_error("cannot open " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void writeBinary(const std::string &path, const void *data, size_t len) {
    std::ofstream f(path, std::ios::binary);
    if (!f.good()) throw std::runtime_error("cannot open " + path);
    f.write(reinterpret_cast<const char *>(data), len);
    if (!f.good()) throw std::runtime_error("failed to write " + path);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        std::fprintf(stderr, "Usage: %s <graph.bin> <inputs.json> <witness.wtns>\n", argv[0]);
        return 1;
    }

    const std::string graphPath = argv[1];
    const std::string inputsPath = argv[2];
    const std::string outPath = argv[3];

    auto graph = readBinary(graphPath);
    auto inputs = readText(inputsPath);
    std::printf("loaded graph (%zu bytes), inputs (%zu bytes)\n", graph.size(), inputs.size());

    void *wtnsBuf = nullptr;
    size_t wtnsLen = 0;
    gw_status_t status = {OK, nullptr};

    int rc = gw_calc_witness(inputs.c_str(), graph.data(), graph.size(),
                             &wtnsBuf, &wtnsLen, &status);
    if (rc != 0) {
        std::fprintf(stderr, "gw_calc_witness failed: %s\n",
                     status.error_msg ? status.error_msg : "(no msg)");
        gw_free_status(&status);
        return 1;
    }
    gw_free_status(&status);
    std::printf("gw_calc_witness ok: %zu bytes\n", wtnsLen);

    // Sanity-check the in-memory BinFile parse (same path SingleProver uses).
    auto wtns = BinFileUtils::openFromBuffer(wtnsBuf, wtnsLen, "wtns", 2);
    if (wtns->getSectionSize(2) == 0) {
        std::fprintf(stderr, "witness section 2 is empty\n");
        return 1;
    }
    std::printf("parsed wtns sections: header=%llu data=%llu bytes\n",
                (unsigned long long)wtns->getSectionSize(1),
                (unsigned long long)wtns->getSectionSize(2));

    writeBinary(outPath, wtnsBuf, wtnsLen);
    free(wtnsBuf);
    std::printf("wrote %s\n", outPath.c_str());
    return 0;
}

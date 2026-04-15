// Byte-equality check: gw_calc_witness_prepared (legacy WTNS wrapper) must
// equal gw_calc_witness_raw_prepared + gw_wtns_from_raw (composed path).
//
// If this passes, the two FFIs compose losslessly, and the server can switch
// to the raw path with no behavioral change to WTNS-producing callers.
//
// Usage: test_raw_vs_wrapped <graph.bin> <inputs.json>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
#include "graph_witness.h"
}

static std::vector<uint8_t> readBinary(const std::string &p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    std::streamsize n = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(n);
    f.read(reinterpret_cast<char *>(buf.data()), n);
    return buf;
}

static std::string readText(const std::string &p) {
    std::ifstream f(p);
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <graph.bin> <inputs.json>\n", argv[0]);
        return 1;
    }
    auto graph = readBinary(argv[1]);
    auto inputs = readText(argv[2]);

    void *handle = nullptr;
    gw_status_t st = {OK, nullptr};
    if (gw_prepare_graph(graph.data(), graph.size(), &handle, &st) != 0) {
        std::fprintf(stderr, "prepare_graph: %s\n", st.error_msg ? st.error_msg : "?");
        return 1;
    }
    gw_free_status(&st);

    // Legacy path
    void *wtnsOld = nullptr; size_t oldLen = 0;
    st = {OK, nullptr};
    if (gw_calc_witness_prepared(handle, inputs.c_str(), &wtnsOld, &oldLen, &st) != 0) {
        std::fprintf(stderr, "calc_witness_prepared: %s\n", st.error_msg ? st.error_msg : "?");
        return 1;
    }
    gw_free_status(&st);
    std::printf("legacy WTNS:    %zu bytes\n", oldLen);

    // Composed path: raw then wrap
    void *fe = nullptr; size_t feN = 0;
    st = {OK, nullptr};
    if (gw_calc_witness_raw_prepared(handle, inputs.c_str(), &fe, &feN, &st) != 0) {
        std::fprintf(stderr, "calc_witness_raw_prepared: %s\n", st.error_msg ? st.error_msg : "?");
        return 1;
    }
    gw_free_status(&st);
    std::printf("raw FEs:        %zu elements\n", feN);

    void *wtnsNew = nullptr; size_t newLen = 0;
    st = {OK, nullptr};
    if (gw_wtns_from_raw(handle, fe, feN, &wtnsNew, &newLen, &st) != 0) {
        std::fprintf(stderr, "wtns_from_raw: %s\n", st.error_msg ? st.error_msg : "?");
        return 1;
    }
    gw_free_status(&st);
    std::printf("composed WTNS:  %zu bytes\n", newLen);

    int rc = 0;
    if (oldLen != newLen) {
        std::fprintf(stderr, "FAIL: length mismatch (%zu vs %zu)\n", oldLen, newLen);
        rc = 2;
    } else if (std::memcmp(wtnsOld, wtnsNew, oldLen) != 0) {
        // Find first diff
        const uint8_t *a = (const uint8_t *)wtnsOld;
        const uint8_t *b = (const uint8_t *)wtnsNew;
        for (size_t i = 0; i < oldLen; i++) {
            if (a[i] != b[i]) {
                std::fprintf(stderr, "FAIL: first diff at byte %zu: 0x%02x vs 0x%02x\n",
                             i, a[i], b[i]);
                break;
            }
        }
        rc = 3;
    } else {
        std::printf("PASS: byte-for-byte equal\n");
    }

    gw_free_witness(wtnsOld, oldLen);
    gw_free_witness(wtnsNew, newLen);
    gw_free_witness(fe, feN * 32);  // assumes bn254; fine for this test
    gw_free_graph(handle);
    return rc;
}

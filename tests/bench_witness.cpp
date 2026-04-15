// Micro-benchmark comparing gw_calc_witness (raw bytes, re-parses every call)
// against gw_prepare_graph + gw_calc_witness_prepared (parse once, reuse).
//
// Usage: bench_witness <graph.bin> <inputs.json> [num_runs]

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
#include "graph_witness.h"
}

static std::vector<uint8_t> readBinary(const std::string &path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    std::streamsize n = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(n);
    f.read(reinterpret_cast<char *>(buf.data()), n);
    return buf;
}

static std::string readText(const std::string &path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: %s <graph.bin> <inputs.json> [num_runs]\n", argv[0]);
        return 1;
    }
    int runs = argc >= 4 ? std::atoi(argv[3]) : 5;

    auto graph = readBinary(argv[1]);
    auto inputs = readText(argv[2]);

    std::printf("=== gw_calc_witness (re-parses graph each call) ===\n");
    for (int i = 0; i < runs; i++) {
        void *wtns = nullptr;
        size_t wlen = 0;
        gw_status_t st = {OK, nullptr};
        auto t0 = std::chrono::steady_clock::now();
        int rc = gw_calc_witness(inputs.c_str(), graph.data(), graph.size(),
                                 &wtns, &wlen, &st);
        auto t1 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        if (rc != 0) { std::fprintf(stderr, "fail: %s\n", st.error_msg ? st.error_msg : "?"); return 1; }
        gw_free_status(&st);
        free(wtns);
        std::printf("  run %d: %lld ms\n", i + 1, (long long)ms);
    }

    std::printf("\n=== gw_calc_witness_prepared (parse once, reuse) ===\n");
    auto tp0 = std::chrono::steady_clock::now();
    void *handle = nullptr;
    gw_status_t pst = {OK, nullptr};
    int prc = gw_prepare_graph(graph.data(), graph.size(), &handle, &pst);
    auto tp1 = std::chrono::steady_clock::now();
    if (prc != 0) { std::fprintf(stderr, "prepare fail: %s\n", pst.error_msg ? pst.error_msg : "?"); return 1; }
    gw_free_status(&pst);
    std::printf("  prepare: %lld ms (one-time)\n",
        (long long)std::chrono::duration_cast<std::chrono::milliseconds>(tp1 - tp0).count());

    for (int i = 0; i < runs; i++) {
        void *wtns = nullptr;
        size_t wlen = 0;
        gw_status_t st = {OK, nullptr};
        auto t0 = std::chrono::steady_clock::now();
        int rc = gw_calc_witness_prepared(handle, inputs.c_str(),
                                          &wtns, &wlen, &st);
        auto t1 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        if (rc != 0) { std::fprintf(stderr, "fail: %s\n", st.error_msg ? st.error_msg : "?"); return 1; }
        gw_free_status(&st);
        free(wtns);
        std::printf("  run %d: %lld ms\n", i + 1, (long long)ms);
    }

    gw_free_graph(handle);
    return 0;
}

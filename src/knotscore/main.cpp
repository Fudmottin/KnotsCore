// src/knotescore/main.cpp

#include <cstdlib>
#include <iostream>
#include <string>

int run(const std::string& cmd) {
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Command failed: " << cmd << "\n";
    }
    return ret;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <core-tag> <knots-tag>\n";
        return 1;
    }

    std::string coreTag  = argv[1];
    std::string knotsTag = argv[2];

    // Check out the requested tags in each submodule
    if (run("git -C src/bitcoin fetch --tags") != 0) return 1;
    if (run("git -C src/bitcoin checkout " + coreTag) != 0) return 1;

    if (run("git -C src/bitcoinknots fetch --tags") != 0) return 1;
    if (run("git -C src/bitcoinknots checkout " + knotsTag) != 0) return 1;

    // Perform a diff between the two directories
    std::string diffCmd =
        "git diff --no-index --color src/bitcoin src/bitcoinknots";

    return run(diffCmd);
}


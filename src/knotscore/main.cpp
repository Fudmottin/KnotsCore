#include "GitUtils.hpp"
#include "DiffPrinter.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: KnotesCore <core-tag> <knots-tag>\n";
        return 1;
    }

    std::string coreTag  = argv[1];
    std::string knotsTag = argv[2];

    std::string coreRepo  = "src/bitcoin";
    std::string knotsRepo = "src/bitcoinknots";

    try {
        git::init();
        auto files = git::listChangedFiles(coreRepo, coreTag, knotsRepo, knotsTag);
        std::cout << "Number of changed files: " << files.size() << "\n";
        diff::printFileDiffs(files, coreRepo, coreTag, knotsRepo, knotsTag);
        git::shutdown();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        git::shutdown();
        return 1;
    }
    return 0;
}


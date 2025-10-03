#include "GitUtils.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <coreTag> <knotsTag>\n";
        return 1;
    }

    std::string coreTag  = argv[1];
    std::string knotsTag = argv[2];

    git::init();

    const std::string coreRepoPath  = "./src/bitcoin/";
    const std::string knotsRepoPath = "./src/bitcoinknots";

    auto diffs = git::listChangedFiles(coreRepoPath, coreTag, knotsRepoPath, knotsTag);

    std::cout << "Number of changed files: " << diffs.size() << "\n";
    for (auto& fd : diffs) {
        std::string statusStr;
        switch (fd.status) {
            case git::FileDiff::Status::OnlyInCore:  statusStr = "Only in Core"; break;
            case git::FileDiff::Status::OnlyInKnots: statusStr = "Only in Knots"; break;
            case git::FileDiff::Status::Modified:    statusStr = "Modified"; break;
        }
        std::cout << "  " << fd.path << " -> " << statusStr << "\n";

        // print patch if available
        if (!fd.patch.empty()) {
            std::cout << fd.patch << "\n";
        }
    }

    git::shutdown();
    return 0;
}


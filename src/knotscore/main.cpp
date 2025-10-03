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

    try {
        gitutils::init();

        const std::string coreRepoPath  = "./src/bitcoin/";
        const std::string knotsRepoPath = "./src/bitcoinknots";

        auto diffs = gitutils::listChangedFiles(coreRepoPath, coreTag, knotsRepoPath, knotsTag);

        std::cout << "Number of changed files: " << diffs.size() << "\n";
        for (auto& fd : diffs) {
            std::string statusStr;
            switch (fd.status) {
                case FileDiff::Status::OnlyInCore:  statusStr = "Only in Core"; break;
                case FileDiff::Status::OnlyInKnots: statusStr = "Only in Knots"; break;
                case FileDiff::Status::Modified:    statusStr = "Modified"; break;
            }

            std::cout << "  " << fd.path << " -> " << statusStr << "\n";

            // print patch if available
            if (fd.status == FileDiff::Status::Modified && !fd.patch.empty()) {
                std::cout << fd.patch << "\n";
            }
        }

        gitutils::shutdown();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}


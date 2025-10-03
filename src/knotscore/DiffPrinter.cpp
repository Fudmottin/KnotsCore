#include "DiffPrinter.hpp"
#include <iostream>

namespace diff {

static std::string statusToString(git::FileDiff::Status status) {
    switch (status) {
        case git::FileDiff::Status::OnlyInCore:  return "Only in Core";
        case git::FileDiff::Status::OnlyInKnots: return "Only in Knots";
        case git::FileDiff::Status::Modified:    return "Modified";
        default:                                 return "Unknown";
    }
}

void printPatch(const std::string& path, const std::string& patch) {
    std::cout << "--- " << path << " ---\n" << patch << "\n";
}

void printFileDiffs(const std::vector<git::FileDiff>& files,
                    const std::string& coreRepo,
                    const std::string& coreTag,
                    const std::string& knotsRepo,
                    const std::string& knotsTag) {
    if (files.empty()) {
        std::cout << "No changes between " << coreRepo << "@" << coreTag
                  << " and " << knotsRepo << "@" << knotsTag << "\n";
        return;
    }

    std::cout << "Changes between " << coreRepo << "@" << coreTag
              << " and " << knotsRepo << "@" << knotsTag << ":\n";

    for (const auto& f : files) {
        std::cout << "  " << f.path << " -> " << statusToString(f.status) << "\n";
        if (!f.patch.empty()) printPatch(f.path, f.patch);
    }

    std::cout << "Number of changed files: " << files.size() << "\n";
}

} // namespace diff


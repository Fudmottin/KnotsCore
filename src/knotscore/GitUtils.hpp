#pragma once
#include <string>
#include <vector>

namespace git {

struct FileDiff {
    std::string path;
    enum class Status { OnlyInCore, OnlyInKnots, Modified } status;
    std::string patch; // optional diff output
};

void init();
void shutdown();
std::string resolveCommit(const std::string& repoPath, const std::string& tag);
std::vector<FileDiff> listChangedFiles(
    const std::string& coreRepo,
    const std::string& coreTag,
    const std::string& knotsRepo,
    const std::string& knotsTag);
std::string diffFile(
    const std::string& coreRepo,
    const std::string& coreTag,
    const std::string& knotsRepo,
    const std::string& knotsTag,
    const std::string& path);

} // namespace git


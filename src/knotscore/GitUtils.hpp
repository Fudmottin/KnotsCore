#pragma once

#include <string>
#include <vector>

#include <git2.h>

namespace git {

struct FileDiff {
    enum class Status { OnlyInCore, OnlyInKnots, Modified };
    std::string path;
    Status status;
    std::string patch;
};

// ------------------- initialization -------------------
void init();
void shutdown();

// ------------------- repository helpers -------------------
git_repository* openRepo(const std::string& path);
git_commit* resolveCommit(git_repository* repo, const std::string& tag);
std::vector<FileDiff> listChangedFiles(
    const std::string& coreRepoPath,
    const std::string& coreTag,
    const std::string& knotsRepoPath,
    const std::string& knotsTag);

// ------------------- diff helpers -------------------
// Compare a single file between two trees in two repositories
std::string diffFile(
    git_repository* coreRepo,
    git_tree* coreTree,
    git_repository* knotsRepo,
    git_tree* knotsTree,
    const std::string& path);

} // namespace git


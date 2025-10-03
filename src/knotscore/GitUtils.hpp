#pragma once

#include <git2.h>
#include <string>
#include <vector>

// Simple structure to represent a file diff
struct FileDiff {
    enum class Status {
        OnlyInCore,
        OnlyInKnots,
        Modified
    };

    std::string path;
    Status status;
    std::string patch; // unified diff text if Modified
};

namespace gitutils {

// lifecycle
void init();
void shutdown();

// open repository
::git_repository* openRepo(const std::string& path);

// resolve tag/commit to commit object
::git_commit* resolveCommit(::git_repository* repo, const std::string& tag);

// list changed files between two tags
std::vector<FileDiff> listChangedFiles(
    const std::string& coreRepoPath,
    const std::string& coreTag,
    const std::string& knotsRepoPath,
    const std::string& knotsTag);

} // namespace gitutils


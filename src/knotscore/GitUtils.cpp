#include "GitUtils.hpp"
#include <git2.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

namespace git {

// ------------------- initialization -------------------

void init() {
    if (git_libgit2_init() < 0)
        throw std::runtime_error("Failed to initialize libgit2");
}

void shutdown() {
    git_libgit2_shutdown();
}

// ------------------- helpers -------------------

static git_repository* openRepo(const std::string& path) {
    git_repository* repo = nullptr;
    if (git_repository_open(&repo, path.c_str()) < 0)
        throw std::runtime_error("Failed to open repository: " + path);
    return repo;
}

static git_commit* resolveCommit(git_repository* repo, const std::string& tag) {
    git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo, tag.c_str()) < 0)
        throw std::runtime_error("Failed to resolve tag/commit: " + tag);

    git_object* peeled = nullptr;
    if (git_object_peel(&peeled, obj, GIT_OBJECT_COMMIT) < 0) {
        git_object_free(obj);
        throw std::runtime_error("Failed to peel tag to commit: " + tag);
    }

    git_commit* commit = (git_commit*)peeled; // safe cast
    git_object_free(obj);
    return commit; // caller frees
}

static std::map<std::string, const git_tree_entry*> collectTreeEntries(git_tree* tree) {
    std::map<std::string, const git_tree_entry*> entries;
    size_t count = git_tree_entrycount(tree);
    for (size_t i = 0; i < count; ++i) {
        const git_tree_entry* e = git_tree_entry_byindex(tree, i);
        entries[git_tree_entry_name(e)] = e;
    }
    return entries;
}

// ------------------- diff -------------------

std::vector<FileDiff> listChangedFiles(
    const std::string& coreRepoPath,
    const std::string& coreTag,
    const std::string& knotsRepoPath,
    const std::string& knotsTag)
{
    git_repository* coreRepo = openRepo(coreRepoPath);
    git_repository* knotsRepo = openRepo(knotsRepoPath);

    git_commit* coreCommit  = resolveCommit(coreRepo, coreTag);
    git_commit* knotsCommit = resolveCommit(knotsRepo, knotsTag);

    git_tree* coreTree;
    git_tree* knotsTree;
    if (git_commit_tree(&coreTree, coreCommit) < 0)
        throw std::runtime_error("Failed to get core tree");
    if (git_commit_tree(&knotsTree, knotsCommit) < 0)
        throw std::runtime_error("Failed to get knots tree");

    auto coreEntries  = collectTreeEntries(coreTree);
    auto knotsEntries = collectTreeEntries(knotsTree);

    std::set<std::string> allNames;
    for (auto& e : coreEntries)  allNames.insert(e.first);
    for (auto& e : knotsEntries) allNames.insert(e.first);

    std::vector<FileDiff> diffs;

    for (auto& name : allNames) {
        auto itCore  = coreEntries.find(name);
        auto itKnots = knotsEntries.find(name);

        if (itCore != coreEntries.end() && itKnots == knotsEntries.end()) {
            diffs.push_back({name, FileDiff::Status::OnlyInCore, ""});
        } else if (itCore == coreEntries.end() && itKnots != knotsEntries.end()) {
            diffs.push_back({name, FileDiff::Status::OnlyInKnots, ""});
        } else {
            const git_tree_entry* coreEntry  = itCore->second;
            const git_tree_entry* knotsEntry = itKnots->second;

            if (git_oid_cmp(git_tree_entry_id(coreEntry), git_tree_entry_id(knotsEntry)) != 0) {
                // Generate patch text
                git_diff* diff = nullptr;
                if (git_diff_tree_to_tree(&diff, coreRepo, coreTree, knotsTree, nullptr) < 0) {
                    git_tree_free(coreTree);
                    git_tree_free(knotsTree);
                    git_commit_free(coreCommit);
                    git_commit_free(knotsCommit);
                    git_repository_free(coreRepo);
                    git_repository_free(knotsRepo);
                    throw std::runtime_error("Failed to compute diff");
                }

                struct PatchCollector {
                    std::string targetFile;
                    std::ostringstream patchText;

                    static int file_cb(const git_diff_delta*, float, void*) { return 0; }
                    static int binary_cb(const git_diff_delta*, const git_diff_binary*, void*) { return 0; }
                    static int hunk_cb(const git_diff_delta*, const git_diff_hunk*, void*) { return 0; }

                    static int line_cb(const git_diff_delta* delta,
                                       const git_diff_hunk*,
                                       const git_diff_line* line,
                                       void* payload)
                    {
                        auto* self = static_cast<PatchCollector*>(payload);
                        try {
                            if (self->targetFile == delta->new_file.path) {
                                self->patchText.write(line->content, line->content_len);
                            }
                        } catch (...) {
                            return 0; // ignore
                        }
                        return 0; // continue
                    }
                };

                PatchCollector collector;
                collector.targetFile = name;

                int foreachErr = git_diff_foreach(diff,
                                                  &PatchCollector::file_cb,
                                                  &PatchCollector::binary_cb,
                                                  &PatchCollector::hunk_cb,
                                                  &PatchCollector::line_cb,
                                                  &collector);
                if (foreachErr < 0) {
                    // fallback: record as modified with empty patch
                    diffs.push_back({name, FileDiff::Status::Modified, ""});
                } else {
                    diffs.push_back({name, FileDiff::Status::Modified, collector.patchText.str()});
                }

                git_diff_free(diff);
            }
        }
    }

    git_tree_free(coreTree);
    git_tree_free(knotsTree);
    git_commit_free(coreCommit);
    git_commit_free(knotsCommit);
    git_repository_free(coreRepo);
    git_repository_free(knotsRepo);

    return diffs;
}

// Optional per-file diff (unused)
std::string diffFile(
    const std::string& coreRepoPath,
    const std::string& coreTag,
    const std::string& knotsRepoPath,
    const std::string& knotsTag,
    const std::string& path)
{
    return "";
}

} // namespace git


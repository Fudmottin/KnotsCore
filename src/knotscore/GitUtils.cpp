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

git_repository* openRepo(const std::string& path) {
    git_repository* repo = nullptr;
    if (git_repository_open(&repo, path.c_str()) < 0)
        throw std::runtime_error("Failed to open repository: " + path);
    return repo;
}

git_commit* resolveCommit(git_repository* repo, const std::string& tag) {
    git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo, tag.c_str()) < 0)
        throw std::runtime_error("Failed to resolve tag/commit: " + tag);

    git_object* peeled = nullptr;
    if (git_object_peel(&peeled, obj, GIT_OBJECT_COMMIT) < 0) {
        git_object_free(obj);
        throw std::runtime_error("Failed to peel tag to commit: " + tag);
    }

    git_commit* commit = (git_commit*)peeled;
    git_object_free(obj);
    return commit;
}

std::map<std::string, const git_tree_entry*> collectTreeEntries(git_tree* tree) {
    std::map<std::string, const git_tree_entry*> entries;
    size_t count = git_tree_entrycount(tree);
    for (size_t i = 0; i < count; ++i) {
        const git_tree_entry* e = git_tree_entry_byindex(tree, i);
        entries[git_tree_entry_name(e)] = e;
    }
    return entries;
}

// ------------------- diff -------------------

// diff a single file given tree objects
std::string diffFile(
    git_repository* coreRepo,
    git_tree* coreTree,
    git_repository* knotsRepo,
    git_tree* knotsTree,
    const std::string& path)
{
    git_diff* diff = nullptr;
    if (git_diff_tree_to_tree(&diff, coreRepo, coreTree, knotsTree, nullptr) < 0)
        throw std::runtime_error("Failed to compute diff");

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
            if (self->targetFile == delta->new_file.path) {
                self->patchText.write(line->content, line->content_len);
            }
            return 0;
        }
    };

    PatchCollector collector;
    collector.targetFile = path;

    git_diff_foreach(diff,
                     &PatchCollector::file_cb,
                     &PatchCollector::binary_cb,
                     &PatchCollector::hunk_cb,
                     &PatchCollector::line_cb,
                     &collector);

    git_diff_free(diff);
    return collector.patchText.str();
}

// recursive tree diff
void collectDiffsRecursive(
    git_repository* coreRepo,
    git_repository* knotsRepo,
    git_tree* coreTree,
    git_tree* knotsTree,
    const std::string& prefix,
    std::vector<FileDiff>& diffs)
{
    auto coreEntries  = collectTreeEntries(coreTree);
    auto knotsEntries = collectTreeEntries(knotsTree);

    std::set<std::string> allNames;
    for (auto& e : coreEntries)  allNames.insert(e.first);
    for (auto& e : knotsEntries) allNames.insert(e.first);

    for (auto& name : allNames) {
        const git_tree_entry* coreEntry  = coreEntries.count(name)  ? coreEntries[name]  : nullptr;
        const git_tree_entry* knotsEntry = knotsEntries.count(name) ? knotsEntries[name] : nullptr;
        std::string fullPath = prefix.empty() ? name : prefix + "/" + name;

        if (coreEntry && !knotsEntry) {
            diffs.push_back({fullPath, FileDiff::Status::OnlyInCore, ""});
        } else if (!coreEntry && knotsEntry) {
            diffs.push_back({fullPath, FileDiff::Status::OnlyInKnots, ""});
        } else if (coreEntry && knotsEntry) {
            git_object_t coreType  = git_tree_entry_type(coreEntry);
            git_object_t knotsType = git_tree_entry_type(knotsEntry);

            if (coreType != knotsType) {
                diffs.push_back({fullPath, FileDiff::Status::Modified, ""});
            } else if (coreType == GIT_OBJ_BLOB &&
                       git_oid_cmp(git_tree_entry_id(coreEntry), git_tree_entry_id(knotsEntry)) != 0)
            {
                std::string patch = diffFile(coreRepo, coreTree, knotsRepo, knotsTree, fullPath);
                diffs.push_back({fullPath, FileDiff::Status::Modified, patch});
            } else if (coreType == GIT_OBJ_TREE) {
                git_tree* subCoreTree;
                git_tree* subKnotsTree;
                git_tree_lookup(&subCoreTree, coreRepo, git_tree_entry_id(coreEntry));
                git_tree_lookup(&subKnotsTree, knotsRepo, git_tree_entry_id(knotsEntry));
                collectDiffsRecursive(coreRepo, knotsRepo, subCoreTree, subKnotsTree, fullPath, diffs);
                git_tree_free(subCoreTree);
                git_tree_free(subKnotsTree);
            }
        }
    }
}

// listChangedFiles entry
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
    if (git_commit_tree(&coreTree, coreCommit) < 0 ||
        git_commit_tree(&knotsTree, knotsCommit) < 0)
        throw std::runtime_error("Failed to get tree for commit");

    std::vector<FileDiff> diffs;
    collectDiffsRecursive(coreRepo, knotsRepo, coreTree, knotsTree, "", diffs);

    git_tree_free(coreTree);
    git_tree_free(knotsTree);
    git_commit_free(coreCommit);
    git_commit_free(knotsCommit);
    git_repository_free(coreRepo);
    git_repository_free(knotsRepo);

    return diffs;
}

} // namespace git


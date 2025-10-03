#include "GitUtils.hpp"
#include <stdexcept>
#include <sstream>
#include <map>
#include <set>

namespace gitutils {

// ------------------- initialization -------------------

void init() {
    if (git_libgit2_init() < 0)
        throw std::runtime_error("Failed to initialize libgit2");
}

void shutdown() {
    git_libgit2_shutdown();
}

// ------------------- helpers -------------------

::git_repository* openRepo(const std::string& path) {
    ::git_repository* repo = nullptr;
    if (git_repository_open(&repo, path.c_str()) < 0)
        throw std::runtime_error("Failed to open repository: " + path);
    return repo;
}

::git_commit* resolveCommit(::git_repository* repo, const std::string& tag) {
    ::git_object* obj = nullptr;
    if (git_revparse_single(&obj, repo, tag.c_str()) < 0)
        throw std::runtime_error("Failed to resolve tag/commit: " + tag);

    ::git_object* peeled = nullptr;
    if (git_object_peel(&peeled, obj, GIT_OBJECT_COMMIT) < 0) {
        git_object_free(obj);
        throw std::runtime_error("Failed to peel tag to commit: " + tag);
    }

    auto* commit = reinterpret_cast<::git_commit*>(peeled);
    git_object_free(obj);
    return commit;
}

static std::map<std::string, const ::git_tree_entry*> collectTreeEntries(::git_tree* tree) {
    std::map<std::string, const ::git_tree_entry*> entries;
    size_t count = git_tree_entrycount(tree);
    for (size_t i = 0; i < count; ++i) {
        const ::git_tree_entry* e = git_tree_entry_byindex(tree, i);
        entries[git_tree_entry_name(e)] = e;
    }
    return entries;
}

// ------------------- diff -------------------

static std::string diffSingleFile(::git_repository* coreRepo, ::git_tree* coreTree,
                                  ::git_repository* knotsRepo, ::git_tree* knotsTree,
                                  const std::string& path)
{
    ::git_diff* diff = nullptr;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;

    const char* paths[] = { path.c_str() };
    opts.pathspec.strings = const_cast<char**>(paths);
    opts.pathspec.count   = 1;
    opts.flags = GIT_DIFF_INCLUDE_UNTRACKED | GIT_DIFF_RECURSE_UNTRACKED_DIRS;

    if (git_diff_tree_to_tree(&diff, coreRepo, coreTree, knotsTree, &opts) < 0)
        return "";

    std::ostringstream out;
    git_diff_print(diff, GIT_DIFF_FORMAT_PATCH,
        [](const git_diff_delta*, const git_diff_hunk*, const git_diff_line* line, void* payload) {
            auto* out = static_cast<std::ostringstream*>(payload);
            out->write(line->content, line->content_len);
            return 0;
        }, &out);

    git_diff_free(diff);
    return out.str();
}

static void collectDiffsRecursive(
    ::git_repository* coreRepo,
    ::git_repository* knotsRepo,
    ::git_tree* coreTree,
    ::git_tree* knotsTree,
    const std::string& prefix,
    std::vector<FileDiff>& diffs)
{
    auto coreEntries  = collectTreeEntries(coreTree);
    auto knotsEntries = collectTreeEntries(knotsTree);

    std::set<std::string> allNames;
    for (auto& e : coreEntries)  allNames.insert(e.first);
    for (auto& e : knotsEntries) allNames.insert(e.first);

    for (auto& name : allNames) {
        const ::git_tree_entry* coreEntry  = coreEntries.count(name)  ? coreEntries[name]  : nullptr;
        const ::git_tree_entry* knotsEntry = knotsEntries.count(name) ? knotsEntries[name] : nullptr;
        std::string fullPath = prefix.empty() ? name : prefix + "/" + name;

        if (coreEntry && !knotsEntry) {
            diffs.push_back({fullPath, FileDiff::Status::OnlyInCore, ""});
        } else if (!coreEntry && knotsEntry) {
            diffs.push_back({fullPath, FileDiff::Status::OnlyInKnots, ""});
        } else if (coreEntry && knotsEntry) {
            auto coreType  = git_tree_entry_type(coreEntry);
            auto knotsType = git_tree_entry_type(knotsEntry);

            if (coreType != knotsType) {
                diffs.push_back({fullPath, FileDiff::Status::Modified, ""});
            } else if (coreType == GIT_OBJ_BLOB &&
                       git_oid_cmp(git_tree_entry_id(coreEntry), git_tree_entry_id(knotsEntry)) != 0)
            {
                diffs.push_back({fullPath, FileDiff::Status::Modified, ""});
            } else if (coreType == GIT_OBJ_TREE) {
                ::git_tree* subCoreTree;
                ::git_tree* subKnotsTree;
                git_tree_lookup(&subCoreTree, coreRepo, git_tree_entry_id(coreEntry));
                git_tree_lookup(&subKnotsTree, knotsRepo, git_tree_entry_id(knotsEntry));
                collectDiffsRecursive(coreRepo, knotsRepo, subCoreTree, subKnotsTree, fullPath, diffs);
                git_tree_free(subCoreTree);
                git_tree_free(subKnotsTree);
            }
        }
    }
}

// ------------------- entry point -------------------

std::vector<FileDiff> listChangedFiles(
    const std::string& coreRepoPath,
    const std::string& coreTag,
    const std::string& knotsRepoPath,
    const std::string& knotsTag)
{
    ::git_repository* coreRepo = openRepo(coreRepoPath);
    ::git_repository* knotsRepo = openRepo(knotsRepoPath);

    ::git_commit* coreCommit  = resolveCommit(coreRepo, coreTag);
    ::git_commit* knotsCommit = resolveCommit(knotsRepo, knotsTag);

    ::git_tree* coreTree;
    ::git_tree* knotsTree;
    if (git_commit_tree(&coreTree, coreCommit) < 0 ||
        git_commit_tree(&knotsTree, knotsCommit) < 0)
        throw std::runtime_error("Failed to get tree for commit");

    // Pass 1: collect files
    std::vector<FileDiff> diffs;
    collectDiffsRecursive(coreRepo, knotsRepo, coreTree, knotsTree, "", diffs);

    // Pass 2: compute patches for modified files
    for (auto& fd : diffs) {
        if (fd.status == FileDiff::Status::Modified) {
            fd.patch = diffSingleFile(coreRepo, coreTree, knotsRepo, knotsTree, fd.path);
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

} // namespace gitutils


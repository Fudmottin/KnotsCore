#include "DiffPrinter.hpp"
#include <iostream>
#include <stdexcept>

namespace diff {

static int hunk_cb(const git_diff_delta* delta,
                   const git_diff_hunk* hunk,
                   void* payload)
{
    std::ostream& out = *reinterpret_cast<std::ostream*>(payload);
    out << "@@ " << hunk->header << " @@\n";
    return 0;
}

static int line_cb(const git_diff_delta* delta,
                   const git_diff_hunk* hunk,
                   const git_diff_line* line,
                   void* payload)
{
    std::ostream& out = *reinterpret_cast<std::ostream*>(payload);
    char prefix = ' ';
    switch (line->origin) {
        case GIT_DIFF_LINE_ADDITION: prefix = '+'; break;
        case GIT_DIFF_LINE_DELETION: prefix = '-'; break;
        case GIT_DIFF_LINE_CONTEXT:  prefix = ' '; break;
        default: prefix = '?'; break;
    }
    out << prefix
        << std::string(line->content, line->content_len);
    return 0;
}

void printPatch(git_patch* patch)
{
    if (!patch) return;

    std::ostream& out = std::cout;
    const git_diff_delta* delta = git_patch_get_delta(patch);
    if (delta) {
        out << "--- " << (delta->old_file.path ? delta->old_file.path : "/dev/null") << "\n";
        out << "+++ " << (delta->new_file.path ? delta->new_file.path : "/dev/null") << "\n";
    }

    if (git_patch_print(patch, line_cb, &out) < 0) {
        git_patch_free(patch);
        throw std::runtime_error("Failed to print patch");
    }
}

}


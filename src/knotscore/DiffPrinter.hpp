#pragma once
#include <git2.h>
#include <string>

namespace diff {

// Print a git_patch as unified diff text
void printPatch(git_patch* patch);

}


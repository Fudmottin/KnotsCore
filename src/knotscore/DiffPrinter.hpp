#pragma once
#include "GitUtils.hpp"
#include <string>
#include <vector>

namespace diff {

void printPatch(const std::string& path, const std::string& patch);
void printFileDiffs(const std::vector<git::FileDiff>& files,
                    const std::string& coreRepo,
                    const std::string& coreTag,
                    const std::string& knotsRepo,
                    const std::string& knotsTag);

} // namespace diff


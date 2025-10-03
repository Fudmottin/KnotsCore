#pragma once
#include <string>
#include <vector>

namespace fw {

struct FileEntry {
    std::string path;
    bool existsInCore;
    bool existsInKnots;
};

std::vector<FileEntry> listFiles(const std::string& coreRepo, const std::string& knotsRepo);

}


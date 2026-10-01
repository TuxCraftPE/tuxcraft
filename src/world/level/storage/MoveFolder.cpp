#include "MoveFolder.h"
#include <cstdio>

void moveFolder(const std::string& src, const std::string& dst) {
    std::rename(src.c_str(), dst.c_str());
}

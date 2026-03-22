#pragma once

#include <M5Unified.hpp>

namespace alphabet_graffiti {

class AlphabetApp {
public:
    void Run();

private:
    void InitBoard();
    void DrawSkeletonUi();
};

}  // namespace alphabet_graffiti

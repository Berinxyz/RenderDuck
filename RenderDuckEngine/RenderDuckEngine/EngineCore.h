#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

#include "types.h"

#define ASSERTMSG(expression, msg) ((void)(                                                       \
            (!!(expression)) ||                                                               \
            (_wassert(_CRT_WIDE(#expression << msg), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0)) \
        )

#define ASSERTFAILMSG(msg) \
    do { \
        std::cerr << "ASSERT: " << msg \
                  << "\nIn file: " << __FILE__ << ", line: " << __LINE__ << std::endl; \
        std::cerr << "Press Enter to continue..."; \
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); \
    } while (0)


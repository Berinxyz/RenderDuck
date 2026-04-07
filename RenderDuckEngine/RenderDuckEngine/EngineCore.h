#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

#include "types.h"

#include "assert.h"

#define ASSERTFAILMSG(msg) ((void)((!!(1==0)) || (_wassert(_CRT_WIDE(msg), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0)))
#define ASSERTMSG(expression, msg) ((void)((!!(expression)) || (_wassert(_CRT_WIDE("ASSERT: " #expression ": " msg), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0)))


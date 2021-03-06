#pragma once

#include "defines.h"

// C++
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 ) // Remove when we move LLVM to it's own project
#endif

#include <string>
#include <regex>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <stack>
#include <queue>
#include <set>
#include <chrono>
#include <memory>
#include <utility>
#include <type_traits>
#include <functional>

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#ifdef CULT_TRACE
#define SPDLOG_TRACE_ON
#endif
#ifdef CULT_DEBUG
#define SPDLOG_DEBUG_ON
#endif

// Vendor
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include "spdlog/sinks/stdout_color_sinks.h"

#ifdef CULT_TRACE
#undef SPDLOG_TRACE
#define SPDLOG_TRACE(logger, ...) logger->trace(__VA_ARGS__)
#endif

// Deps
#include "types/core.h"

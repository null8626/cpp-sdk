/**
 * @module topgg
 * @file models.h
 * @brief The community-maintained C++ library for Top.gg.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2026 Top.gg & null8626
 * @date 2026-03-10
 * @version 3.0.0
 */

#pragma once

#include <topgg/topgg.h>

#include <string>
#include <optional>
#include <string>
#include <vector>

#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE
#endif

#include <ctime>

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

namespace topgg {
}; // namespace topgg
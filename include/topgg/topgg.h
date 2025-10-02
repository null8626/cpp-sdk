/**
 * @module topgg
 * @file topgg.h
 * @brief A community-maintained C++ API Client for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2025 Top.gg & null8626
 * @date 2025-10-02
 * @version 2.1.0
 */

#pragma once

#define __TOPGG_API__
#include <topgg/export.h>
#undef __TOPGG_API__

#ifdef __TOPGG_TESTING__
#define TOPGG_AUTOPOSTER_MIN_INTERVAL 2
#else
#define TOPGG_AUTOPOSTER_MIN_INTERVAL 900
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wdeprecated-literal-operator"
#pragma clang diagnostic ignored "-Wdeprecated-copy-with-dtor"
#endif

#include <dpp/dpp.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#define TOPGG_BASE_URL "https://top.gg/api"

#include <topgg/result.h>
#include <topgg/models.h>
#include <topgg/client.h>
/**
 * @module topgg
 * @file topgg.h
 * @brief The official C++ wrapper for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2025 Top.gg & null8626
 * @date 2025-02-19
 * @version 2.0.1
 */

#pragma once

#ifdef _WIN32
#if defined(DPP_STATIC) && !defined(TOPGG_STATIC)
#define TOPGG_STATIC
#elif defined(TOPGG_STATIC) && !defined(DPP_STATIC)
#define DPP_STATIC
#endif
#endif

#if defined(_WIN32) && !defined(TOPGG_STATIC)
#ifdef __TOPGG_BUILDING_DLL__
#include <dpp/win32_safe_warnings.h>
#define TOPGG_EXPORT __declspec(dllexport)
#else
#define TOPGG_EXPORT __declspec(dllimport)
#endif
#else
#define TOPGG_EXPORT
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TOPGG_UNUSED __attribute__((unused))
#else
#define TOPGG_UNUSED
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

#include <topgg/result.h>
#include <topgg/models.h>
#include <topgg/client.h>
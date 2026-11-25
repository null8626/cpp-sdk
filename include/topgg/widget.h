#pragma once

#include <topgg/topgg.h>

namespace topgg::widget {
/**
 * @brief Generates a large widget URL.
 *
 * @param platform The project's platform.
 * @param type The project's type.
 * @param id The project's Top.gg ID.
 * @param hide_avatar Whether to also hide the project's avatar. Defaults to
 * false.
 * @return std::string The generated widget URL.
 * @since 2.0.0
 */
std::string large(const topgg::platform& platform,
                  const topgg::project_type& type, const uint64_t id,
                  const bool hide_avatar = false);

/**
 * @brief Generates a small widget URL for displaying votes.
 *
 * @param platform The project's platform.
 * @param type The project's type.
 * @param id The project's Top.gg ID.
 * @param hide_avatar Whether to also hide the project's avatar. Defaults to
 * false.
 * @return std::string The generated widget URL.
 * @since 2.0.0
 */
std::string votes(const topgg::platform& platform,
                  const topgg::project_type& type, const uint64_t id,
                  const bool hide_avatar = false);

/**
 * @brief Generates a small widget URL for displaying a project's owner.
 *
 * @param platform The project's platform.
 * @param type The project's type.
 * @param id The project's Top.gg ID.
 * @param hide_avatar Whether to also hide the project's avatar. Defaults to
 * false.
 * @return std::string The generated widget URL.
 * @since 2.0.0
 */
std::string owner(const topgg::platform& platform,
                  const topgg::project_type& type, const uint64_t id,
                  const bool hide_avatar = false);

/**
 * @brief Generates a small widget URL for displaying social stats.
 *
 * @param platform The project's platform.
 * @param type The project's type.
 * @param id The project's Top.gg ID.
 * @param hide_avatar Whether to also hide the project's avatar. Defaults to
 * false.
 * @return std::string The generated widget URL.
 * @since 2.0.0
 */
std::string social(const topgg::platform& platform,
                   const topgg::project_type& type, const uint64_t id,
                   const bool hide_avatar = false);
};  // namespace topgg::widget
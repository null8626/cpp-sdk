#include <topgg/topgg.h>

#define _TOPGG_GENERATE_WIDGET_URL(endpoint, platform, type, id, hide_avatar) \
  auto url{std::string{TOPGG_BASE_URL "/widgets" endpoint} +                  \
           topgg::_platform_to_string(platform) + "/" +                       \
           topgg::_project_type_to_string(type) + "/" + std::to_string(id)};  \
                                                                              \
  if (hide_avatar) {                                                          \
    url += "?hideAvatar=true";                                                \
  }                                                                           \
                                                                              \
  return url

std::string topgg::widget::large(const topgg::platform& platform,
                                 const topgg::project_type& type,
                                 const uint64_t id, const bool hide_avatar) {
  _TOPGG_GENERATE_WIDGET_URL("/large/", platform, type, id, hide_avatar);
}

std::string topgg::widget::votes(const topgg::platform& platform,
                                 const topgg::project_type& type,
                                 const uint64_t id, const bool hide_avatar) {
  _TOPGG_GENERATE_WIDGET_URL("/small/votes/", platform, type, id, hide_avatar);
}

std::string topgg::widget::owner(const topgg::platform& platform,
                                 const topgg::project_type& type,
                                 const uint64_t id, const bool hide_avatar) {
  _TOPGG_GENERATE_WIDGET_URL("/small/owner/", platform, type, id, hide_avatar);
}

std::string topgg::widget::social(const topgg::platform& platform,
                                  const topgg::project_type& type,
                                  const uint64_t id, const bool hide_avatar) {
  _TOPGG_GENERATE_WIDGET_URL("/small/social/", platform, type, id, hide_avatar);
}
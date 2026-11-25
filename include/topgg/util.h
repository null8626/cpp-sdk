#pragma once

#include <topgg/topgg.h>

namespace topgg {
platform _from_platform_string(const std::string& platform);

project_type _from_project_type_string(const std::string& type);

time_t _from_time_string(const std::string& timestamp);

std::string _to_time_string(const time_t timestamp);

std::string _url_encode(const std::string& text);

std::string _random_string();

const char* _platform_to_string(const platform& platform);

const char* _project_type_to_string(const project_type& type);
};  // namespace topgg
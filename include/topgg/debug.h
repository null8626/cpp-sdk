#pragma once

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#define TOPGG_LOG(text) topgg::_log(__FILE__, __LINE__, text)
#define TOPGG_LOGF(format, ...) topgg::_logf(__FILE__, __LINE__, format, __VA_ARGS__)
#define TOPGG_ELOG(text) topgg::_elog(__FILE__, __LINE__, text)
#define TOPGG_ELOGF(format, ...) topgg::_elogf(__FILE__, __LINE__, format, __VA_ARGS__)


namespace topgg {
  void _log(const char* file, const size_t line, const char* text);

  void _logf(const char* file, const size_t line, const char* format, ...);

  void _elog(const char* file, const size_t line, const char* text);

  void _elogf(const char* file, const size_t line, const char* format, ...);
};
#else
#define TOPGG_LOG(text)
#define TOPGG_LOGF(format, ...)
#define TOPGG_ELOG(text)
#define TOPGG_ELOGF(format, ...)
#endif
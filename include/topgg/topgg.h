#pragma once

#if defined(_WIN32) && !defined(_WINSOCKAPI_)
#define _WINSOCKAPI_
#endif

#include <topgg/debug.h>
#include <topgg/models.h>
#include <topgg/util.h>
#ifdef TOPGG_WEBHOOKS
#include <topgg/webhooks/models.h>
#include <topgg/webhooks/client.h>
#endif
#include <topgg/exception.h>
#include <topgg/result.h>
#include <topgg/http.h>
#include <topgg/client.h>
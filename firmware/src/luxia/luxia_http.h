#pragma once

#include "sky_http_server.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

const sky_http_route_t *luxia_http_get_routes(size_t *count);

#ifdef __cplusplus
}
#endif

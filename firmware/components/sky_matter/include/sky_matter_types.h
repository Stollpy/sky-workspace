#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SKY_MATTER_TRANSPORT_WIFI,
    SKY_MATTER_TRANSPORT_THREAD,
    SKY_MATTER_TRANSPORT_AUTO,
} sky_matter_transport_t;

typedef enum {
    SKY_MATTER_WC_STATUS_STOPPED,
    SKY_MATTER_WC_STATUS_OPENING,
    SKY_MATTER_WC_STATUS_CLOSING,
} sky_matter_wc_status_t;

#ifdef __cplusplus
}
#endif

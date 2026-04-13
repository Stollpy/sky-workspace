#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise all Luxia subsystems in the correct startup order:
 * core → power → wifi → stepper → matter → ota → sleep → http → cmd → events → start.
 *
 * Call once from app_main().
 */
esp_err_t luxia_init(void);

#ifdef __cplusplus
}
#endif

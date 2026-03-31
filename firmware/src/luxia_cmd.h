#pragma once

#include "esp_err.h"
#include "sky_stepper_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Command identifiers ────────────────────────────────────────────── */

typedef enum {
    /* Calibration  0x01 – 0x0F */
    LUXIA_CMD_CALIBRATE_START       = 0x01,
    LUXIA_CMD_CALIBRATE_MARK_OPEN   = 0x02,
    LUXIA_CMD_CALIBRATE_MARK_CLOSED = 0x03,
    LUXIA_CMD_CALIBRATE_CANCEL      = 0x04,
    LUXIA_CMD_CALIBRATE_STATUS      = 0x05,

    /* Presets  0x10 – 0x1F */
    LUXIA_CMD_PRESET_SAVE           = 0x10,
    LUXIA_CMD_PRESET_APPLY          = 0x11,
    LUXIA_CMD_PRESET_DELETE         = 0x12,
    LUXIA_CMD_PRESET_LIST           = 0x13,

    /* Sequences  0x20 – 0x2F */
    LUXIA_CMD_SEQUENCE_RECORD_START = 0x20,
    LUXIA_CMD_SEQUENCE_RECORD_STEP  = 0x21,
    LUXIA_CMD_SEQUENCE_RECORD_STOP  = 0x22,
    LUXIA_CMD_SEQUENCE_PLAY         = 0x23,
    LUXIA_CMD_SEQUENCE_STOP         = 0x24,
    LUXIA_CMD_SEQUENCE_LIST         = 0x25,
    LUXIA_CMD_SEQUENCE_DELETE       = 0x26,
    LUXIA_CMD_SEQUENCE_EXPORT       = 0x27,
    LUXIA_CMD_SEQUENCE_IMPORT       = 0x28,

    /* Diagnostics  0x40 – 0x4F */
    LUXIA_CMD_STATUS                = 0x40,
    LUXIA_CMD_REBOOT                = 0x41,
    LUXIA_CMD_FACTORY_RESET         = 0x42,
    LUXIA_CMD_FIRMWARE_VERSION      = 0x43,
    LUXIA_CMD_OTA_CHECK             = 0x44,
    LUXIA_CMD_OTA_UPDATE            = 0x45,
    LUXIA_CMD_SET_NAME              = 0x46,
} luxia_cmd_t;

/* ── Aggregate device status ────────────────────────────────────────── */

typedef struct {
    struct {
        sky_stepper_state_t state;
        int32_t  position;
        int8_t   percent;
        bool     calibrated;
        int32_t  total_steps;
    } motor;
    struct {
        float    voltage;
        uint8_t  percent;
        bool     charging;
        bool     low;
        bool     critical;
    } battery;
    struct {
        bool     connected;
        int8_t   rssi;
        char     ip[16];
    } wifi;
    struct {
        bool     commissioned;
    } matter;
    uint32_t heap_free;
} luxia_status_t;

/* ── Initialisation ─────────────────────────────────────────────────── */

esp_err_t luxia_cmd_init(sky_stepper_handle_t motor);

/* ── Calibration ────────────────────────────────────────────────────── */

esp_err_t luxia_cmd_calibrate_start(sky_direction_t dir);
esp_err_t luxia_cmd_calibrate_mark_open(int32_t *out_position);
esp_err_t luxia_cmd_calibrate_mark_closed(int32_t *out_position,
                                          int32_t *out_total);
esp_err_t luxia_cmd_calibrate_cancel(void);
esp_err_t luxia_cmd_calibrate_status(bool *calibrated, int32_t *total);

/* ── Presets ────────────────────────────────────────────────────────── */

esp_err_t luxia_cmd_preset_save(const char *name, uint8_t percent);
esp_err_t luxia_cmd_preset_apply(const char *name);
esp_err_t luxia_cmd_preset_delete(const char *name);
esp_err_t luxia_cmd_preset_list(sky_pattern_t *out, uint8_t *count,
                                uint8_t max);

/* ── Sequences ──────────────────────────────────────────────────────── */

esp_err_t luxia_cmd_sequence_record_start(const char *name);
esp_err_t luxia_cmd_sequence_record_step(uint8_t percent,
                                         uint32_t delay_ms);
esp_err_t luxia_cmd_sequence_record_stop(void);
esp_err_t luxia_cmd_sequence_play(const char *name);
esp_err_t luxia_cmd_sequence_stop(void);
esp_err_t luxia_cmd_sequence_list(sky_pattern_t *out, uint8_t *count,
                                  uint8_t max);
esp_err_t luxia_cmd_sequence_delete(const char *name);
esp_err_t luxia_cmd_sequence_export(const char *name, sky_pattern_t *out);
esp_err_t luxia_cmd_sequence_import(const sky_pattern_t *pattern);

/* ── Diagnostics ────────────────────────────────────────────────────── */

esp_err_t luxia_cmd_get_status(luxia_status_t *out);
esp_err_t luxia_cmd_reboot(void);
esp_err_t luxia_cmd_factory_reset(void);
esp_err_t luxia_cmd_get_firmware_version(char *buf, size_t buf_size);
esp_err_t luxia_cmd_ota_check(bool *available);
esp_err_t luxia_cmd_ota_update(void);
esp_err_t luxia_cmd_set_motor_name(const char *name);

#ifdef __cplusplus
}
#endif

#include "luxia_http.h"
#include "luxia_cmd.h"
#include "sky_stepper.h"
#include "esp_log.h"
#include "cJSON.h"

#include <string.h>
#include <stdlib.h>

static const char *TAG = "luxia_http";

#define MAX_BODY_SIZE 256

/* ═══════════════════════════════════════════════════════════════════════
 *  Helpers
 * ═══════════════════════════════════════════════════════════════════ */

static cJSON *read_json_body(httpd_req_t *req)
{
    int content_len = req->content_len;
    if (content_len <= 0 || content_len > MAX_BODY_SIZE) return NULL;

    char *buf = malloc(content_len + 1);
    if (!buf) return NULL;

    int received = httpd_req_recv(req, buf, content_len);
    if (received != content_len) {
        free(buf);
        return NULL;
    }
    buf[content_len] = '\0';

    cJSON *json = cJSON_Parse(buf);
    free(buf);
    return json;
}

static esp_err_t send_json(httpd_req_t *req, cJSON *json)
{
    char *str = cJSON_PrintUnformatted(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, str);
    free(str);
    return ESP_OK;
}

static esp_err_t send_ok(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

static esp_err_t send_error(httpd_req_t *req, int status, const char *msg)
{
    httpd_resp_set_status(req, status == 400 ? "400 Bad Request"
                             : status == 404 ? "404 Not Found"
                             : "500 Internal Server Error");
    httpd_resp_set_type(req, "application/json");

    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "ok", false);
    cJSON_AddStringToObject(json, "error", msg);
    char *str = cJSON_PrintUnformatted(json);
    httpd_resp_sendstr(req, str);
    free(str);
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t send_esp_err(httpd_req_t *req, esp_err_t err)
{
    return send_error(req, 500, esp_err_to_name(err));
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Diagnostics
 * ═══════════════════════════════════════════════════════════════════ */

static const char *stepper_state_str(sky_stepper_state_t s)
{
    switch (s) {
    case SKY_STEPPER_STATE_IDLE:        return "idle";
    case SKY_STEPPER_STATE_MOVING:      return "moving";
    case SKY_STEPPER_STATE_STOPPING:    return "stopping";
    case SKY_STEPPER_STATE_CALIBRATING: return "calibrating";
    case SKY_STEPPER_STATE_ERROR:       return "error";
    default:                            return "unknown";
    }
}

static esp_err_t handle_status(httpd_req_t *req)
{
    luxia_status_t st;
    esp_err_t err = luxia_cmd_get_status(&st);
    if (err != ESP_OK) return send_esp_err(req, err);

    cJSON *root = cJSON_CreateObject();

    cJSON *motor = cJSON_AddObjectToObject(root, "motor");
    cJSON_AddStringToObject(motor, "state", stepper_state_str(st.motor.state));
    cJSON_AddNumberToObject(motor, "position", st.motor.position);
    cJSON_AddNumberToObject(motor, "percent", st.motor.percent);
    cJSON_AddBoolToObject(motor, "calibrated", st.motor.calibrated);
    cJSON_AddNumberToObject(motor, "total_steps", st.motor.total_steps);

    cJSON *batt = cJSON_AddObjectToObject(root, "battery");
    cJSON_AddNumberToObject(batt, "voltage", st.battery.voltage);
    cJSON_AddNumberToObject(batt, "percent", st.battery.percent);
    cJSON_AddBoolToObject(batt, "charging", st.battery.charging);
    cJSON_AddBoolToObject(batt, "low", st.battery.low);
    cJSON_AddBoolToObject(batt, "critical", st.battery.critical);

    cJSON *wifi = cJSON_AddObjectToObject(root, "wifi");
    cJSON_AddBoolToObject(wifi, "connected", st.wifi.connected);
    cJSON_AddNumberToObject(wifi, "rssi", st.wifi.rssi);
    cJSON_AddStringToObject(wifi, "ip", st.wifi.ip);

    cJSON *matter = cJSON_AddObjectToObject(root, "matter");
    cJSON_AddBoolToObject(matter, "commissioned", st.matter.commissioned);

    cJSON_AddNumberToObject(root, "heap_free", st.heap_free);

    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t handle_version(httpd_req_t *req)
{
    char ver[32];
    luxia_cmd_get_firmware_version(ver, sizeof(ver));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "version", ver);
    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t handle_reboot(httpd_req_t *req)
{
    send_ok(req);
    luxia_cmd_reboot();
    return ESP_OK;
}

static esp_err_t handle_factory_reset(httpd_req_t *req)
{
    send_ok(req);
    luxia_cmd_factory_reset();
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Motor control
 * ═══════════════════════════════════════════════════════════════════ */

static esp_err_t handle_motor_move(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *pct = cJSON_GetObjectItem(body, "percent");
    if (!cJSON_IsNumber(pct) || pct->valueint < 0 || pct->valueint > 100) {
        cJSON_Delete(body);
        return send_error(req, 400, "percent must be 0-100");
    }

    sky_stepper_handle_t motor = sky_stepper_get("motor");
    if (!motor) {
        cJSON_Delete(body);
        return send_error(req, 500, "motor not found");
    }
    esp_err_t err = sky_stepper_set_height(motor, (uint8_t)pct->valueint);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_motor_stop(httpd_req_t *req)
{
    sky_stepper_handle_t motor = sky_stepper_get("motor");
    if (!motor) return send_error(req, 500, "motor not found");

    esp_err_t err = sky_stepper_stop(motor);
    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_motor_name(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *name = cJSON_GetObjectItem(body, "name");
    if (!cJSON_IsString(name) || !name->valuestring[0]) {
        cJSON_Delete(body);
        return send_error(req, 400, "name is required");
    }

    esp_err_t err = luxia_cmd_set_motor_name(name->valuestring);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Calibration
 * ═══════════════════════════════════════════════════════════════════ */

static esp_err_t handle_calibrate_start(httpd_req_t *req)
{
    sky_direction_t dir = SKY_DIR_CW;

    cJSON *body = read_json_body(req);
    if (body) {
        cJSON *d = cJSON_GetObjectItem(body, "direction");
        if (cJSON_IsString(d) && strcmp(d->valuestring, "ccw") == 0) {
            dir = SKY_DIR_CCW;
        }
        cJSON_Delete(body);
    }

    esp_err_t err = luxia_cmd_calibrate_start(dir);
    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_calibrate_mark_open(httpd_req_t *req)
{
    int32_t pos = 0;
    esp_err_t err = luxia_cmd_calibrate_mark_open(&pos);
    if (err != ESP_OK) return send_esp_err(req, err);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddNumberToObject(root, "position", pos);
    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t handle_calibrate_mark_closed(httpd_req_t *req)
{
    int32_t pos = 0, total = 0;
    esp_err_t err = luxia_cmd_calibrate_mark_closed(&pos, &total);
    if (err != ESP_OK) return send_esp_err(req, err);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddNumberToObject(root, "position", pos);
    cJSON_AddNumberToObject(root, "total_steps", total);
    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t handle_calibrate_cancel(httpd_req_t *req)
{
    esp_err_t err = luxia_cmd_calibrate_cancel();
    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_calibrate_status(httpd_req_t *req)
{
    bool calibrated = false;
    int32_t total = 0;
    luxia_cmd_calibrate_status(&calibrated, &total);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "calibrated", calibrated);
    cJSON_AddNumberToObject(root, "total_steps", total);
    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Presets
 * ═══════════════════════════════════════════════════════════════════ */

static esp_err_t handle_preset_list(httpd_req_t *req)
{
    sky_pattern_t presets[CONFIG_SKY_STEPPER_MAX_PATTERNS];
    uint8_t count = 0;
    esp_err_t err = luxia_cmd_preset_list(presets, &count,
                                          CONFIG_SKY_STEPPER_MAX_PATTERNS);
    if (err != ESP_OK) return send_esp_err(req, err);

    cJSON *root = cJSON_CreateArray();
    for (uint8_t i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", presets[i].name);
        cJSON_AddNumberToObject(item, "percent", presets[i].preset_percent);
        cJSON_AddItemToArray(root, item);
    }
    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t handle_preset_save(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *name = cJSON_GetObjectItem(body, "name");
    cJSON *pct  = cJSON_GetObjectItem(body, "percent");
    if (!cJSON_IsString(name) || !name->valuestring[0] ||
        !cJSON_IsNumber(pct) || pct->valueint < 0 || pct->valueint > 100) {
        cJSON_Delete(body);
        return send_error(req, 400, "name (string) and percent (0-100) required");
    }

    esp_err_t err = luxia_cmd_preset_save(name->valuestring,
                                          (uint8_t)pct->valueint);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_preset_apply(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *name = cJSON_GetObjectItem(body, "name");
    if (!cJSON_IsString(name) || !name->valuestring[0]) {
        cJSON_Delete(body);
        return send_error(req, 400, "name is required");
    }

    esp_err_t err = luxia_cmd_preset_apply(name->valuestring);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_preset_delete(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *name = cJSON_GetObjectItem(body, "name");
    if (!cJSON_IsString(name) || !name->valuestring[0]) {
        cJSON_Delete(body);
        return send_error(req, 400, "name is required");
    }

    esp_err_t err = luxia_cmd_preset_delete(name->valuestring);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Sequences
 * ═══════════════════════════════════════════════════════════════════ */

static esp_err_t handle_sequence_list(httpd_req_t *req)
{
    sky_pattern_t seqs[CONFIG_SKY_STEPPER_MAX_PATTERNS];
    uint8_t count = 0;
    esp_err_t err = luxia_cmd_sequence_list(seqs, &count,
                                            CONFIG_SKY_STEPPER_MAX_PATTERNS);
    if (err != ESP_OK) return send_esp_err(req, err);

    cJSON *root = cJSON_CreateArray();
    for (uint8_t i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", seqs[i].name);
        cJSON_AddNumberToObject(item, "step_count",
                                seqs[i].sequence.step_count);

        cJSON *steps = cJSON_AddArrayToObject(item, "steps");
        for (uint8_t j = 0; j < seqs[i].sequence.step_count; j++) {
            cJSON *step = cJSON_CreateObject();
            cJSON_AddNumberToObject(step, "percent",
                                    seqs[i].sequence.steps[j].target_percent);
            cJSON_AddNumberToObject(step, "delay_ms",
                                    seqs[i].sequence.steps[j].delay_after_ms);
            cJSON_AddItemToArray(steps, step);
        }
        cJSON_AddItemToArray(root, item);
    }
    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t handle_sequence_record_start(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *name = cJSON_GetObjectItem(body, "name");
    if (!cJSON_IsString(name) || !name->valuestring[0]) {
        cJSON_Delete(body);
        return send_error(req, 400, "name is required");
    }

    esp_err_t err = luxia_cmd_sequence_record_start(name->valuestring);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_sequence_record_step(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *pct   = cJSON_GetObjectItem(body, "percent");
    cJSON *delay = cJSON_GetObjectItem(body, "delay_ms");
    if (!cJSON_IsNumber(pct) || pct->valueint < 0 || pct->valueint > 100 ||
        !cJSON_IsNumber(delay) || delay->valueint < 0) {
        cJSON_Delete(body);
        return send_error(req, 400, "percent (0-100) and delay_ms required");
    }

    esp_err_t err = luxia_cmd_sequence_record_step((uint8_t)pct->valueint,
                                                   (uint32_t)delay->valueint);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_sequence_record_stop(httpd_req_t *req)
{
    esp_err_t err = luxia_cmd_sequence_record_stop();
    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_sequence_play(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *name = cJSON_GetObjectItem(body, "name");
    if (!cJSON_IsString(name) || !name->valuestring[0]) {
        cJSON_Delete(body);
        return send_error(req, 400, "name is required");
    }

    esp_err_t err = luxia_cmd_sequence_play(name->valuestring);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_sequence_stop(httpd_req_t *req)
{
    esp_err_t err = luxia_cmd_sequence_stop();
    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

static esp_err_t handle_sequence_delete(httpd_req_t *req)
{
    cJSON *body = read_json_body(req);
    if (!body) return send_error(req, 400, "invalid JSON body");

    cJSON *name = cJSON_GetObjectItem(body, "name");
    if (!cJSON_IsString(name) || !name->valuestring[0]) {
        cJSON_Delete(body);
        return send_error(req, 400, "name is required");
    }

    esp_err_t err = luxia_cmd_sequence_delete(name->valuestring);
    cJSON_Delete(body);

    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  OTA
 * ═══════════════════════════════════════════════════════════════════ */

static esp_err_t handle_ota_check(httpd_req_t *req)
{
    bool available = false;
    esp_err_t err = luxia_cmd_ota_check(&available);
    if (err != ESP_OK) return send_esp_err(req, err);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "available", available);
    send_json(req, root);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t handle_ota_update(httpd_req_t *req)
{
    esp_err_t err = luxia_cmd_ota_update();
    if (err != ESP_OK) return send_esp_err(req, err);
    return send_ok(req);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Route table
 * ═══════════════════════════════════════════════════════════════════ */

static const sky_http_route_t s_routes[] = {
    /* Diagnostics */
    { .uri = "/api/status",          .method = HTTP_GET,  .handler = handle_status },
    { .uri = "/api/version",         .method = HTTP_GET,  .handler = handle_version },
    { .uri = "/api/reboot",          .method = HTTP_POST, .handler = handle_reboot },
    { .uri = "/api/factory-reset",   .method = HTTP_POST, .handler = handle_factory_reset },

    /* Motor */
    { .uri = "/api/motor/move",      .method = HTTP_POST, .handler = handle_motor_move },
    { .uri = "/api/motor/stop",      .method = HTTP_POST, .handler = handle_motor_stop },
    { .uri = "/api/motor/name",      .method = HTTP_POST, .handler = handle_motor_name },

    /* Calibration */
    { .uri = "/api/calibrate/start",       .method = HTTP_POST, .handler = handle_calibrate_start },
    { .uri = "/api/calibrate/mark-open",   .method = HTTP_POST, .handler = handle_calibrate_mark_open },
    { .uri = "/api/calibrate/mark-closed", .method = HTTP_POST, .handler = handle_calibrate_mark_closed },
    { .uri = "/api/calibrate/cancel",      .method = HTTP_POST, .handler = handle_calibrate_cancel },
    { .uri = "/api/calibrate/status",      .method = HTTP_GET,  .handler = handle_calibrate_status },

    /* Presets */
    { .uri = "/api/presets",         .method = HTTP_GET,  .handler = handle_preset_list },
    { .uri = "/api/presets/save",    .method = HTTP_POST, .handler = handle_preset_save },
    { .uri = "/api/presets/apply",   .method = HTTP_POST, .handler = handle_preset_apply },
    { .uri = "/api/presets/delete",  .method = HTTP_POST, .handler = handle_preset_delete },

    /* Sequences */
    { .uri = "/api/sequences",              .method = HTTP_GET,  .handler = handle_sequence_list },
    { .uri = "/api/sequences/record/start", .method = HTTP_POST, .handler = handle_sequence_record_start },
    { .uri = "/api/sequences/record/step",  .method = HTTP_POST, .handler = handle_sequence_record_step },
    { .uri = "/api/sequences/record/stop",  .method = HTTP_POST, .handler = handle_sequence_record_stop },
    { .uri = "/api/sequences/play",         .method = HTTP_POST, .handler = handle_sequence_play },
    { .uri = "/api/sequences/stop",         .method = HTTP_POST, .handler = handle_sequence_stop },
    { .uri = "/api/sequences/delete",       .method = HTTP_POST, .handler = handle_sequence_delete },

    /* OTA */
    { .uri = "/api/ota/check",       .method = HTTP_POST, .handler = handle_ota_check },
    { .uri = "/api/ota/update",      .method = HTTP_POST, .handler = handle_ota_update },
};

const sky_http_route_t *luxia_http_get_routes(size_t *count)
{
    *count = sizeof(s_routes) / sizeof(s_routes[0]);
    ESP_LOGI(TAG, "registering %d routes", (int)*count);
    return s_routes;
}

#include "sky_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "sky_event";

typedef struct {
    sky_event_id_t      event_id;
    sky_event_handler_t handler;
    void               *ctx;
    bool                active;
} subscriber_t;

typedef struct {
    sky_event_id_t event_id;
    void          *data;
    size_t         data_size;
} queue_item_t;

static subscriber_t    *s_subscribers     = NULL;
static uint16_t         s_max_subscribers = 0;
static SemaphoreHandle_t s_mutex          = NULL;
static QueueHandle_t    s_queue           = NULL;
static TaskHandle_t     s_task            = NULL;
static bool             s_async           = false;
static bool             s_initialized     = false;

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void dispatch_event(const sky_event_t *event)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (uint16_t i = 0; i < s_max_subscribers; i++) {
        if (s_subscribers[i].active &&
            s_subscribers[i].event_id == event->event_id) {
            sky_event_handler_t handler = s_subscribers[i].handler;
            void *ctx = s_subscribers[i].ctx;
            xSemaphoreGive(s_mutex);
            handler(event, ctx);
            xSemaphoreTake(s_mutex, portMAX_DELAY);
        }
    }
    xSemaphoreGive(s_mutex);
}

static void event_bus_task(void *arg)
{
    queue_item_t item;
    while (1) {
        if (xQueueReceive(s_queue, &item, portMAX_DELAY) == pdTRUE) {
            sky_event_t event = {
                .event_id     = item.event_id,
                .data         = item.data,
                .data_size    = item.data_size,
                .timestamp_ms = now_ms(),
            };
            dispatch_event(&event);
            free(item.data);
        }
    }
}

esp_err_t sky_event_bus_init(const sky_event_bus_config_t *config)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t max_sub  = config ? config->max_subscribers : 32;
    uint16_t queue_sz = config ? config->queue_size : 16;
    s_async = config ? config->async : true;

    s_subscribers = calloc(max_sub, sizeof(subscriber_t));
    if (!s_subscribers) {
        return ESP_ERR_NO_MEM;
    }
    s_max_subscribers = max_sub;

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        free(s_subscribers);
        s_subscribers = NULL;
        return ESP_ERR_NO_MEM;
    }

    if (s_async) {
        s_queue = xQueueCreate(queue_sz, sizeof(queue_item_t));
        if (!s_queue) {
            vSemaphoreDelete(s_mutex);
            free(s_subscribers);
            s_subscribers = NULL;
            return ESP_ERR_NO_MEM;
        }
        BaseType_t ret = xTaskCreate(event_bus_task, "sky_evt", 4096,
                                     NULL, 5, &s_task);
        if (ret != pdPASS) {
            vQueueDelete(s_queue);
            vSemaphoreDelete(s_mutex);
            free(s_subscribers);
            s_subscribers = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    s_initialized = true;
    ESP_LOGI(TAG, "initialized (max_sub=%u queue=%u async=%d)",
             max_sub, queue_sz, s_async);
    return ESP_OK;
}

esp_err_t sky_event_subscribe(sky_event_id_t event_id,
                              sky_event_handler_t handler,
                              void *ctx)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (!handler)       return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (uint16_t i = 0; i < s_max_subscribers; i++) {
        if (!s_subscribers[i].active) {
            s_subscribers[i].event_id = event_id;
            s_subscribers[i].handler  = handler;
            s_subscribers[i].ctx      = ctx;
            s_subscribers[i].active   = true;
            xSemaphoreGive(s_mutex);
            return ESP_OK;
        }
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGW(TAG, "subscriber slots full (max=%u)", s_max_subscribers);
    return ESP_ERR_NO_MEM;
}

esp_err_t sky_event_unsubscribe(sky_event_id_t event_id,
                                sky_event_handler_t handler)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (uint16_t i = 0; i < s_max_subscribers; i++) {
        if (s_subscribers[i].active &&
            s_subscribers[i].event_id == event_id &&
            s_subscribers[i].handler  == handler) {
            s_subscribers[i].active = false;
            xSemaphoreGive(s_mutex);
            return ESP_OK;
        }
    }
    xSemaphoreGive(s_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sky_event_publish(sky_event_id_t event_id,
                            const void *data,
                            size_t data_size)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    if (s_async && s_queue) {
        void *data_copy = NULL;
        if (data && data_size > 0) {
            data_copy = malloc(data_size);
            if (!data_copy) return ESP_ERR_NO_MEM;
            memcpy(data_copy, data, data_size);
        }
        queue_item_t item = {
            .event_id  = event_id,
            .data      = data_copy,
            .data_size = data_size,
        };
        if (xQueueSend(s_queue, &item, pdMS_TO_TICKS(100)) != pdTRUE) {
            free(data_copy);
            ESP_LOGW(TAG, "event queue full, dropping 0x%04lx",
                     (unsigned long)event_id);
            return ESP_ERR_TIMEOUT;
        }
        return ESP_OK;
    }

    sky_event_t event = {
        .event_id     = event_id,
        .data         = (void *)data,
        .data_size    = data_size,
        .timestamp_ms = now_ms(),
    };
    dispatch_event(&event);
    return ESP_OK;
}

void sky_event_bus_deinit(void)
{
    if (!s_initialized) return;
    s_initialized = false;

    if (s_task) {
        vTaskDelete(s_task);
        s_task = NULL;
    }
    if (s_queue) {
        queue_item_t item;
        while (xQueueReceive(s_queue, &item, 0) == pdTRUE) {
            free(item.data);
        }
        vQueueDelete(s_queue);
        s_queue = NULL;
    }
    if (s_mutex) {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
    }
    free(s_subscribers);
    s_subscribers = NULL;
    s_max_subscribers = 0;
}

// Copyright (c) Imaination Garden. Inc. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "config.h"
#include "init_iothub_client.h"

#include "api.h"
#include "firmware.h"
#include "wifi_setting_server.h"

#define SEC 1000

static const char *TAG = "main";

void azure_iothub(void *pv_parameter) {
  for (;;) {
    iothub_do_work();
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
void check_messages(void *pv_parameter) {
  wait_connect_wifi();
  send_boot_message();
  while (1) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
void check_connection(void *pv_parameter) {
  wait_connect_wifi();
  int connection_fail_count = 0;
  api_set_device_connection_url(DEVICE_CONNECTION_URL);
  api_set_device_connection_device_id(DEVICE_ID);
  while (1) {
    bool connection = api_get_device_connection();
    ESP_LOGI(TAG, "connection : %s", connection ? "true" : "false");
    if (connection) {
      connection_fail_count = 0;
    } else {
      connection_fail_count++;
    }
    if (connection_fail_count > 3) {
      esp_restart();
    }
    vTaskDelay(10 * SEC / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
void update(void *pv_parameter) {
  wait_connect_wifi();
  while (1) {
    ESP_LOGI(TAG, "current version : %s", VERSION);
    if (!firmware_is_latest_version()) {
      firmware_update();
    }
    vTaskDelay(60 * SEC / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void app_main() {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize firmware updater
  firmware_init();
  esp_ota_img_states_t ota_state;
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_get_state_partition(running, &ota_state);
  ESP_LOGI(TAG, "ota state: %d", ota_state);

  // Initialize wifi setting server
  set_ap_ssid(DEVICE_ID);
  start_wifi_setting_server();

  // Initialize iothub client
  init_iothub_client();

  if (xTaskCreate(&azure_iothub, "azure iothub", 1024 * 6, NULL, 1, NULL) !=
      pdPASS) {
    printf("create azure iothub task failed\r\n");
  }
  if (xTaskCreate(&check_messages, "check messages", 1024 * 6, NULL, 2, NULL) !=
      pdPASS) {
    printf("create check messages task failed\r\n");
  }
  if (xTaskCreate(&check_connection, "check connection", 1024 * 6, NULL, 3,
                  NULL) != pdPASS) {
    printf("create check connection task failed\r\n");
  }
  if (xTaskCreate(&update, "update", 1024 * 6, NULL, 4, NULL) != pdPASS) {
    printf("create update task failed\r\n");
  }
}

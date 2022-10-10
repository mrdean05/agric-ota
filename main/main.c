#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi-connect.h"
#include <stdio.h>

#include "device_drivers.h"
#define TAG "main app"
#define SSID_1 "ota-agric"
#define PASS "ota-agric"

void wifi_sta_setup(void) {
  wifi_drivers();
  esp_err_t wifi_sta_callback = wifi_sta_connect(SSID_1, PASS);

  if (wifi_sta_callback == 0) {
    ESP_LOGI(TAG, "CONNECTED");
  }
}

void app_main(void) {

  gpio_init();
  esp_err_t nvs_results = nvs_flash_init();
  if (nvs_results == ESP_ERR_NVS_NO_FREE_PAGES ||
      ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_results = nvs_flash_erase();
    nvs_results |= nvs_flash_init();
  }

  if (nvs_results != ESP_OK) {
    ESP_LOGE(TAG, "NVS init error");
  }

  wifi_sta_setup();
  cloud_start();
}

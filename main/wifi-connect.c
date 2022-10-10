#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"

#define TAG "WIFI"

esp_netif_t *esp_netif;

static EventGroupHandle_t wifi_events;
const uint32_t gotIP = BIT0;
 

void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch(event_id)
    {
        case SYSTEM_EVENT_STA_START: 
        ESP_LOGI(TAG, "connecting...");
        esp_wifi_connect();
        break;

        case SYSTEM_EVENT_STA_CONNECTED: 
        ESP_LOGI(TAG, "CONNECTED");
        break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "DISCONNECTED");
        esp_wifi_connect();
        break;

        case SYSTEM_EVENT_STA_GOT_IP: 
        ESP_LOGI(TAG, "IP OBTAINED");
        xEventGroupSetBits(wifi_events, gotIP);  
        break;

        default: 
        break;
    }
}


void wifi_drivers(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP, event_handler, NULL));
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
}


esp_err_t wifi_sta_connect(const char* ssid, const char* password)
{
    wifi_events= xEventGroupCreate();
    esp_netif = esp_netif_create_default_wifi_sta(); 
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)- 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password)- 1);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

   EventBits_t event_results=  xEventGroupWaitBits(wifi_events, gotIP, pdFALSE, pdTRUE, 2000 / portTICK_PERIOD_MS);
    if (event_results == 0)
    {
        return ESP_OK;  
          
    }

    else {
         return ESP_FAIL;
    }
}
#pragma once 
#include "esp_err.h"

void wifi_drivers(void);
esp_err_t wifi_sta_connect(const char* ssid, const char* password);
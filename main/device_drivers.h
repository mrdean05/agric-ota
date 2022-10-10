#pragma once 

#include "cJSON.h"

void gpio_init(void);
void change_set(uint32_t set);
int cloud_start(void);
int sm_sensor(void);
int ldr_sensor(void);
int dht11_temperature(void);
int dht11_humidity(void);
int getMessage(char* mPayload, int len);
esp_err_t do_firmware_upgrade(const char *url);

enum dht11_status {
    DHT11_CRC_ERROR = -2,
    DHT11_TIMEOUT_ERROR,
    DHT11_OK
};

struct dht11_reading {
    int status;
    int temperature;
    int humidity;
};

void DHT11_init(gpio_num_t);

struct dht11_reading DHT11_read();
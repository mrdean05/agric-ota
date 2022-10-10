#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "sdkconfig.h"
#include "driver/adc.h"
#include "device_drivers.h"
#include "esp_gpio.h"

#define BUF_SIZE (1024)


int sm_val= 0;
int light_val= 0; 
char *ota_url;
static bool ota_update_done = false;


void gpio_init()
{

    // define gpio led
    gpio_config_t io_config = {
        .mode = GPIO_MODE_INPUT,
//        .pull_up_en = 1,
//        .pull_down_en = 0,
        .pin_bit_mask = ((uint64_t)1 << MOISTURE),
    };

    gpio_config(&io_config);

    
    // define dh11 pin
    DHT11_init(DHT_PIN);

    // define adc
    adc1_config_width(ADC_WIDTH_BIT_12);   
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

}

int sm_sensor(void)
{

    sm_val = gpio_get_level(MOISTURE);
    printf("The level of input pin: %d\n", sm_val);
    return sm_val;
}

int ldr_sensor(void)
{

    light_val = adc1_get_raw(ADC1_CHANNEL_7);

    printf("light intensity: %d\n", light_val);

    return light_val;
}

int dht11_temperature(void)
{

    printf("Temperature is %d \n", DHT11_read().temperature);
    printf("Status code is %d\n", DHT11_read().status);

    return (DHT11_read().temperature) ;
}

int dht11_humidity(void)
{
    printf("Humidity is %d\n", DHT11_read().humidity);
    return (DHT11_read().humidity);
}

int getMessage(char *mPayload, int len)
{

    cJSON *url = NULL;
    cJSON *json = cJSON_ParseWithLength(mPayload, len);
    int status = 0;

    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        status = -1;
        goto end;
    }
    url = cJSON_GetObjectItemCaseSensitive(json, "ota_url");
    if (!cJSON_IsString(url) && (url->valuestring == NULL))
    {
        status = -1;
        goto end;
    }

    ota_url = url->valuestring;
    printf("%s \n", ota_url);

    if (do_firmware_upgrade(ota_url) == ESP_OK)
    {
        // Firmware upgrade successful
        ota_update_done = true;
    }

    if (ota_update_done)
    {
        esp_restart();
    }

end:
    cJSON_Delete(json);
    return status;
}





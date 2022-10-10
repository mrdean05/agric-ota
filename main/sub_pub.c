#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver/sdmmc_host.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_version.h"

#include "cJSON.h"
#include "device_drivers.h"
#define TAG "subpub"

// Root Certificate
extern const uint8_t
    aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");

// ert and private key
extern const uint8_t
    certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t
    certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t
    private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

// AWS IoT Endpoint specific to account and region
extern const uint8_t endpoint_txt_start[] asm("_binary_endpoint_txt_start");
extern const uint8_t endpoint_txt_end[] asm("_binary_endpoint_txt_end");

// Define default AWS MQTT port
uint32_t port = AWS_IOT_MQTT_PORT;

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName,
                                    uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params,
                                    void *pData) {
  ESP_LOGI(TAG, "Subscribe callback");
  char *userData = (char *)params->payload;
  int len = (int)params->payloadLen;
  printf("User Data: %.*s \n", len, userData);
  ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int)params->payloadLen,
           (char *)params->payload);
  int feedback = getMessage((char *)params->payload, (int)params->payloadLen);
  printf("%d\n", feedback);
}

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
  ESP_LOGW(TAG, "MQTT Disconnect");
  IoT_Error_t rc = FAILURE;

  if (NULL == pClient) {
    return;
  }

  if (aws_iot_is_autoreconnect_enabled(pClient)) {
    ESP_LOGI(TAG,
             "Auto Reconnect is enabled, Reconnecting attempt will start now");
  } else {
    ESP_LOGW(TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
    rc = aws_iot_mqtt_attempt_reconnect(pClient);
    if (NETWORK_RECONNECTED == rc) {
      ESP_LOGW(TAG, "Manual Reconnect Successful");
    } else {
      ESP_LOGW(TAG, "Manual Reconnect Failed - %d", rc);
    }
  }
}

void aws_iot_task(void *param) {

  IoT_Error_t rc = FAILURE;

  // Initialize mqtt parameter
  AWS_IoT_Client client;
  IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
  IoT_Publish_Message_Params paramsQOS0;
  ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR,
           VERSION_PATCH, VERSION_TAG);
  mqttInitParams.enableAutoReconnect = false; // We enable this later below
  mqttInitParams.pHostURL = (char *)endpoint_txt_start;
  mqttInitParams.port = port;
  mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
  mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
  mqttInitParams.pDevicePrivateKeyLocation =
      (const char *)private_pem_key_start;
  mqttInitParams.mqttCommandTimeout_ms = 20000;
  mqttInitParams.tlsHandshakeTimeout_ms = 5000;
  mqttInitParams.isSSLHostnameVerify = true;
  mqttInitParams.disconnectHandler = disconnectCallbackHandler;
  mqttInitParams.disconnectHandlerData = NULL;

  // intialize mqtt
  rc = aws_iot_mqtt_init(&client, &mqttInitParams);
  if (SUCCESS != rc) {
    ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
    abort();
  }

  /* Wait for WiFI to show as connected */
  //   xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
  //                       false, true, portMAX_DELAY);

  // Initialize connect parameters
  const char *CONFIG_AWS_EXAMPLE_CLIENT_ID = "geo-fence-iot";
  IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;
  connectParams.keepAliveIntervalInSec = 10;
  connectParams.isCleanSession = true;
  connectParams.MQTTVersion = MQTT_3_1_1;
  /* Client ID is set in the menuconfig of the example */
  connectParams.pClientID = CONFIG_AWS_EXAMPLE_CLIENT_ID;
  connectParams.clientIDLen = (uint16_t)strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);
  connectParams.isWillMsgPresent = false;

  ESP_LOGI(TAG, "Connecting to AWS...");
  do {
    rc = aws_iot_mqtt_connect(&client, &connectParams);
    if (SUCCESS != rc) {
      ESP_LOGE(TAG, "Error(%d) connecting to %s:%d", rc,
               mqttInitParams.pHostURL, mqttInitParams.port);
      vTaskDelay(1000 / portTICK_RATE_MS);
    }
  } while (SUCCESS != rc);

  /*
     Enbable autoreconnect if a disconnection happens
   * Enable Auto Reconnect functionality. Minimum and Maximum time of
   Exponential backoff are set in aws_iot_config.h
   *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
   *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
   */

  rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
  if (SUCCESS != rc) {
    ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
    abort();
  }

  const char *TOPIC = "iotDevice/ota";
  const int TOPIC_LEN = strlen(TOPIC);

  const char *TOPIC_SENSOR = "iotDevice/sensorData";
  const int TOPIC_LEN_SENSOR = strlen(TOPIC_SENSOR);

  ESP_LOGI(TAG, "Subscribing...");
  rc = aws_iot_mqtt_subscribe(&client, TOPIC, TOPIC_LEN, QOS0,
                              iot_subscribe_callback_handler, NULL);
  if (SUCCESS != rc) {
    ESP_LOGE(TAG, "Error subscribing : %d ", rc);
    abort();
  }

  paramsQOS0.qos = QOS0;
  paramsQOS0.isRetained = 0;
  while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc ||
          SUCCESS == rc)) {

    // Max time the yield function will wait for read messages
    rc = aws_iot_mqtt_yield(&client, 100);
    if (NETWORK_ATTEMPTING_RECONNECT == rc) {
      // If the client is attempting to reconnect we will skip the rest of the
      // loop.
      continue;
    }

    // create json object
    char *json_str;
    cJSON *ldr_value = NULL;
    cJSON *sm_value = NULL;
    cJSON *temperature = NULL;
    cJSON *humidity = NULL;
    cJSON *iot_device = cJSON_CreateObject();
    ldr_value = cJSON_CreateNumber(ldr_sensor());
    sm_value = cJSON_CreateNumber(sm_sensor());
    temperature = cJSON_CreateNumber(dht11_temperature());
    humidity = cJSON_CreateNumber(dht11_humidity());
    cJSON_AddItemToObject(iot_device, "Soil_Moisture", sm_value);
    cJSON_AddItemToObject(iot_device, "LDR", ldr_value);
    cJSON_AddItemToObject(iot_device, "Temperature", temperature);
    cJSON_AddItemToObject(iot_device, "Humidity", humidity);
    json_str = cJSON_Print(iot_device);

    if (json_str != NULL) {

      ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes",
               pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));

      paramsQOS0.payload = json_str;
      paramsQOS0.payloadLen = strlen(json_str);

      rc = aws_iot_mqtt_publish(&client, TOPIC_SENSOR, TOPIC_LEN_SENSOR,
                                &paramsQOS0);
    }
    vTaskDelay(2000 / portTICK_RATE_MS);
  }
  ESP_LOGE(TAG, "An error occurred in the main loop.");
  abort();
}

int cloud_start(void) {
  printf("Starting cloud\n");

  BaseType_t cloud_begin =
      xTaskCreate(&aws_iot_task, "aws_iot_task", 9216, NULL, 5, NULL);
  if (cloud_begin != pdPASS) {
    ESP_LOGE(TAG, "Couldnt create a cloud task\n");
  }
  return ESP_OK;
}
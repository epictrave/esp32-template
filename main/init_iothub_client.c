// Copyright (c) Imaination Garden. Inc. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include <stdio.h>
#include <stdlib.h>

#include "init_iothub_client.h"

#include "config.h"
#include "iothub_message.h"

#include "firmware.h"
#include "wifi_setting_server.h"

#ifndef BIT0
#define BIT0 (0x1 << 0)
#endif

#ifndef BIT1
#define BIT1 (0x1 << 1)
#endif

#ifndef BIT2
#define BIT2 (0x1 << 2)
#endif

EventGroupHandle_t iothub_event_group = NULL;
const int IOTHUB_CONNECTED_BIT = BIT0;
const int IOTHUB_MESSAGE_BIT = BIT1;
const int IOTHUB_PROPERTY_BIT = BIT2;
int iothub_do_work_count = 0;

typedef struct EVENT_INSTANCE_TAG {
  IOTHUB_MESSAGE_HANDLE message_handle;
  size_t message_tracking_id; // For tracking the messages within the user
                              // callback.
} EVENT_INSTANCE;
static int callback_counter = 1;
int receive_context = 1;

EVENT_INSTANCE message;
IOTHUB_CLIENT_LL_HANDLE iothub_client_handle;

static const char *TAG = "iothub";

void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                void *user_context_callback) {
  (void)printf(
      "\n\nConnection Status result:%s, Connection Status reason: %s\n\n",
      ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result),
      ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
  if (reason == IOTHUB_CLIENT_CONNECTION_OK) {
    xEventGroupSetBits(iothub_event_group, IOTHUB_CONNECTED_BIT);
  } else {
    xEventGroupClearBits(iothub_event_group, IOTHUB_CONNECTED_BIT);
  }
  if (reason == IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED) {
    (void)printf("Restart device due to connection retry expired.\r\n");
    esp_restart();
  }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT
receive_message_callback(IOTHUB_MESSAGE_HANDLE message,
                         void *user_context_callback) {
  int *counter = (int *)user_context_callback;
  const char *buffer;
  size_t size;
  MAP_HANDLE map_properties;
  const char *message_id;
  const char *correlation_id;

  // Message properties
  if ((message_id = IoTHubMessage_GetMessageId(message)) == NULL) {
    message_id = "<null>";
  }

  if ((correlation_id = IoTHubMessage_GetCorrelationId(message)) == NULL) {
    correlation_id = "<null>";
  }

  // Message content
  if (IoTHubMessage_GetByteArray(message, (const unsigned char **)&buffer,
                                 &size) != IOTHUB_MESSAGE_OK) {
    (void)printf("unable to retrieve the message data\r\n");
  } else {
    (void)printf(
        "Received Message [%d]\r\n Message ID: %s\r\n Correlation ID: %s\r\n "
        "Data: %.*s\r\nSize=%d\r\n",
        *counter, message_id, correlation_id, (int)size, buffer, (int)size);
    // If we receive the work 'quit' then we stop running
  }
  // Retrieve properties from the message
  map_properties = IoTHubMessage_Properties(message);
  if (map_properties != NULL) {
    const char *const *keys;
    const char *const *values;
    size_t property_count = 0;
    if (Map_GetInternals(map_properties, &keys, &values, &property_count) ==
        MAP_OK) {
      if (property_count > 0) {
        size_t index;
        printf(" Message Properties:\r\n");
        for (index = 0; index < property_count; index++) {
          (void)printf("\tKey: %s Value: %s\r\n", keys[index], values[index]);
        }
        (void)printf("\r\n");
      }
    }
  }
  /* Some device specific action code goes here... */
  (*counter)++;
  return IOTHUBMESSAGE_ACCEPTED;
}
static void send_confirmation_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result,
                                       void *user_context_callback) {
  EVENT_INSTANCE *event_instance = (EVENT_INSTANCE *)user_context_callback;
  size_t id = event_instance->message_tracking_id;

  if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
    (void)printf(
        "Confirmation[%d] received for message tracking id = %d with result = "
        "%s\r\n",
        callback_counter, (int)id,
        ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
    callback_counter++;
  }
  event_instance->message_tracking_id++;
  IoTHubMessage_Destroy(event_instance->message_handle);
  xEventGroupSetBits(iothub_event_group, IOTHUB_MESSAGE_BIT);
}
void send_json_message_with_device_id(char *json_string) {
  JSON_Value *root_value = json_parse_string(json_string);
  JSON_Object *root_object = json_value_get_object(root_value);
  json_object_set_string(root_object, "deviceID", DEVICE_ID);
  json_object_set_number(root_object, "messageID", message.message_tracking_id);
  char *text = json_serialize_to_string(root_value);
  send_message(text);
  json_value_free(root_value);
  free(text);
}
void send_boot_message(void) {
  JSON_Value *root_value = json_value_init_object();
  JSON_Object *root_object = json_value_get_object(root_value);
  json_object_set_string(root_object, "deviceID", DEVICE_ID);
  json_object_set_boolean(root_object, "boot", true);
  json_object_set_number(root_object, "messageID", message.message_tracking_id);
  json_object_set_string(root_object, "version", VERSION);
  char *text = json_serialize_to_string(root_value);
  send_message(text);
  json_value_free(root_value);
  free(text);
}
void send_message(char *buffer) {
  wait_iothub_connection();
  ESP_LOGI(TAG, "message [%s]", buffer);
  if ((message.message_handle = IoTHubMessage_CreateFromByteArray(
           (const unsigned char *)buffer, strlen(buffer))) == NULL) {
    (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
  } else {
    /* Set application properties
    MAP_HANDLE prop_map = IoTHubMessage_Properties(message.message_handle);
    (void)Map_AddOrUpdate(prop_map, "$$ContentType", "JSON");
    static char prop_text[1024];
    (void)sprintf_s(prop_text, sizeof(prop_text),
                    temperature > 28 ? "true" : "false");
    if (Map_AddOrUpdate(prop_map, "temperatureAlert", prop_text) != MAP_OK) {
      (void)printf("ERROR: Map_AddOrUpdate Failed!\r\n");
    }
    */
    if (IoTHubClient_LL_SendEventAsync(
            iothub_client_handle, message.message_handle,
            send_confirmation_callback, &message) != IOTHUB_CLIENT_OK) {
      (void)printf("ERROR: "
                   "IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
    } else {
      (void)printf("IoTHubClient_LL_SendEventAsync accepted message [%d] for "
                   "transmission to IoT Hub.\r\n",
                   (int)message.message_tracking_id);
      xEventGroupClearBits(iothub_event_group, IOTHUB_MESSAGE_BIT);
    }
  }
}

static int device_method_callback(const char *method_name,
                                  const unsigned char *payload, size_t size,
                                  unsigned char **response,
                                  size_t *response_size,
                                  void *user_context_callback) {
  (void)user_context_callback;
  (void)payload;
  (void)size;

  printf("Method Name: %s\n", method_name);
  int result;
  if (strcmp("quit", method_name) == 0) {
    const char deviceMethodResponse[] = "{ \"sucess\": \"true\" }";
    *response_size = sizeof(deviceMethodResponse) - 1;
    *response = malloc(*response_size);
    if (*response != NULL) {
      (void)memcpy(*response, deviceMethodResponse, *response_size);
      result = 200;
    } else {
      result = -1;
    }
  } else {
    // All other entries are ignored.
    const char deviceMethodResponse[] = "{ }";
    *response_size = sizeof(deviceMethodResponse) - 1;
    *response = malloc(*response_size);
    if (*response != NULL) {
      (void)memcpy(*response, deviceMethodResponse, *response_size);
    }
    result = -1;
  }

  return result;
}

static void device_twin_callback(DEVICE_TWIN_UPDATE_STATE update_state,
                                 const unsigned char *payload, size_t size,
                                 void *user_context_callback) {
  xEventGroupClearBits(iothub_event_group, IOTHUB_PROPERTY_BIT);
  ESP_LOGI(TAG, "Update state: %d", update_state);
  ESP_LOGI(TAG, "Desired twin :%s\r\n", payload);
  (void)update_state;
  (void)size;
  xEventGroupSetBits(iothub_event_group, IOTHUB_PROPERTY_BIT);

  firmware_parse_from_json((const char *)payload, update_state);
}

static void reported_state_callback(int status_code,
                                    void *user_context_callback) {
  (void)user_context_callback;
  printf("Device Twin reported properties update completed with result: %d\r\n",
         status_code);
  xEventGroupSetBits(iothub_event_group, IOTHUB_PROPERTY_BIT);
}

void report_property(char *reported_state) {
  wait_iothub_connection();
  if (IoTHubClient_LL_SendReportedState(
          iothub_client_handle, (const unsigned char *)reported_state,
          strlen(reported_state), reported_state_callback,
          0) != IOTHUB_CLIENT_OK) {
    printf(
        "[iothubClient] reported properties를 설정하는데 실패함.\n '%s'.\r\n",
        reported_state);
  } else {
    printf("[iothubClient] reported properties를 설정함.\n %s \r\n\n",
           reported_state);
    xEventGroupClearBits(iothub_event_group, IOTHUB_PROPERTY_BIT);
  }
}

void init_iothub_client() {
  iothub_event_group = xEventGroupCreate();
  xEventGroupSetBits(iothub_event_group,
                     IOTHUB_MESSAGE_BIT | IOTHUB_PROPERTY_BIT);
  message.message_tracking_id = 1;
  if (platform_init() != 0) {
    (void)printf("Failed to initialize the platform.\r\n");
  } else {
    if ((iothub_client_handle = IoTHubClient_LL_CreateFromConnectionString(
             CONNECTION_STRING, MQTT_Protocol)) == NULL) {
      (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
    } else {
      bool traceOn = false;
      IoTHubClient_LL_SetOption(iothub_client_handle, OPTION_LOG_TRACE,
                                &traceOn);

      int ping_time = 10;                   // 10 sec
      int sas_token_time = 60 * 60;         // 1 hour
      size_t retry_policy_expire_time = 60; // 1 min

      if (IoTHubClient_LL_SetOption(iothub_client_handle, OPTION_KEEP_ALIVE,
                                    &ping_time) != IOTHUB_CLIENT_OK) {
        (void)printf("Failure to keep alive.\r\n");
      }
      if (IoTHubClient_LL_SetOption(iothub_client_handle,
                                    OPTION_SAS_TOKEN_LIFETIME,
                                    &sas_token_time) != IOTHUB_CLIENT_OK) {
        (void)printf("Failure to set sas token life time.\r\n");
      }
      if (IoTHubClient_LL_SetRetryPolicy(
              iothub_client_handle, IOTHUB_CLIENT_RETRY_INTERVAL,
              retry_policy_expire_time) != IOTHUB_CLIENT_OK) {
        (void)printf("Failure to set sas token life time.\r\n");
      } else {
        (void)printf("IoTHubClient_LL_SetRetryPolicy...successful!\r\n");
      };
      if (IoTHubClient_LL_SetConnectionStatusCallback(
              iothub_client_handle, connection_status_callback, NULL) !=
          IOTHUB_CLIENT_OK) {
        (void)printf(
            "ERROR: "
            "IoTHubClient_LL_SetConnectionStatusCallback..........FAILED!\r\n");
      } else {
        (void)printf(
            "IoTHubClient_LL_SetConnectionStatusCallback...successful!\r\n");
      };
      if (IoTHubClient_LL_SetMessageCallback(
              iothub_client_handle, receive_message_callback,
              &receive_context) != IOTHUB_CLIENT_OK) {
        (void)printf(
            "ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
      } else {
        (void)printf("IoTHubClient_LL_SetMessageCallback...successful.\r\n");
      }
      if (IoTHubDeviceClient_LL_SetDeviceMethodCallback(
              iothub_client_handle, device_method_callback, NULL) !=
          IOTHUB_CLIENT_OK) {
        (void)printf(
            "ERROR: "
            "IoTHubDeviceClient_LL_SetDeviceMethodCallback..........FAILED!"
            "\r\n");
      } else {
        (void)printf(
            "IoTHubDeviceClient_LL_SetDeviceMethodCallback...successful."
            "\r\n");
      }
      if (IoTHubDeviceClient_LL_SetDeviceTwinCallback(
              iothub_client_handle, device_twin_callback, NULL) !=
          IOTHUB_CLIENT_OK) {
        (void)printf(
            "ERROR: "
            "IoTHubDeviceClient_LL_SetDeviceTwinCallback..........FAILED!"
            "\r\n");
      } else {
        (void)printf(
            "IoTHubDeviceClient_LL_SetDeviceTwinCallback...successful.\r\n");
      }
    }
  }
}
void iothub_do_work(void) {
  wait_connect_wifi();
  if (iothub_do_work_count++ % 10 == 0) {
    ESP_LOGI(TAG, "do work free heap : %d", esp_get_free_heap_size());
  }
  IoTHubClient_LL_DoWork(iothub_client_handle);
}
void wait_iothub_connection(void) {
  if (iothub_event_group == NULL) {
    ESP_LOGE(TAG, "iothub event group is not initialized");
    return;
  }
  wait_connect_wifi();
  xEventGroupWaitBits(iothub_event_group,
                      IOTHUB_CONNECTED_BIT | IOTHUB_MESSAGE_BIT |
                          IOTHUB_PROPERTY_BIT,
                      false, true, portMAX_DELAY);
}
// Copyright (c) Imaination Garden. Inc. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef INIT_IOTHUB_CLIENT_H
#define INIT_IOTHUB_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/threadapi.h"

#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_device_client_ll.h"
#include "iothubtransportmqtt.h"

#include "parson.h"

extern EventGroupHandle_t wifi_event_group;
extern const int WIFI_CONNECTED_BIT;

void init_iothub_client(void);
void iothub_do_work(void);
void send_json_message_with_device_id(char *json_string);
void send_boot_message(void);
void send_message(char *buffer);
void report_property(char *reported_state);
void wait_iothub_connection(void);
#ifdef __cplusplus
}
#endif

#endif /* INIT_IOTHUB_CLIENT_H */

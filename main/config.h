// Copyright (c) Imaination Garden. Inc. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef CONFIG_H
#define CONFIG_H

#include "iothub_connection_string.h"

#ifndef VERSION
#define VERSION "0.0.1"
#endif

#ifdef ENV_PROD
#define DEVICE_CONNECTION_URL                                                  \
  "https://fn-iothub-centralkr-ig-prod01.azurewebsites.net/api/HttpTrigger1"
#elif defined(ENV_DEV)
#define DEVICE_CONNECTION_URL                                                  \
  "https://fn-iothub-centralkr-ig-dev01.azurewebsites.net/api/CheckConnection"
#endif

#endif /* CONFIG_H */

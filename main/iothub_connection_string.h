// Copyright (c) Imaination Garden. Inc. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef IOTHUBCONNECTIONSTRING_H_
#define IOTHUBCONNECTIONSTRING_H_

#ifdef DEVICE_1
#define DEVICE_ID
#define CONNECTION_STRING
#define ENV_PROD

#elif defined(DEVICE_2)
#define DEVICE_ID
#define CONNECTION_STRING
#define ENV_PROD

#else
#define DEVICE_ID "Test"
#define CONNECTION_STRING                                                      \
  "HostName=IoTHUB-CentralKR-IG-Dev1.azure-devices.net;DeviceId=Test;"         \
  "SharedAccessKey=itk247jyQhOAC5yoCio1jeI0whTGPEWpnYgTPs8i7Z8="
#define ENV_DEV
#endif

#endif /* IOTHUBCONNECTIONSTRING_H_ */
/*
 * Copyright (c) 2024 Koen Vervloesem
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/coap.h>
#include <openthread/thread.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>

LOG_MODULE_REGISTER(ot_coap_bme280, LOG_LEVEL_DBG);

#define SEND_TO_ADDR "ff03::1"

static void send_sensor_request(const struct device *dev) {
  char json_buf[100];
  otError error = OT_ERROR_NONE;
  otInstance *p_instance = openthread_get_default_instance();
  otMessage *p_message;
  otMessageInfo message_info;
  otExtAddress eui64;
  char eui64_id[25] = "\0";
  char buf[3];
  struct sensor_value temp, press, hum;

  otPlatRadioGetIeeeEui64(p_instance, eui64.m8);
  for (uint8_t i = 0; i < 8; i++) {
    snprintk(buf, sizeof(buf), "%02X", eui64.m8[i]);
    strcat(eui64_id, buf);
  }

  memset(&message_info, 0, sizeof(message_info));
  otIp6AddressFromString(SEND_TO_ADDR, &message_info.mPeerAddr);
  message_info.mPeerPort = OT_DEFAULT_COAP_PORT;

  sensor_sample_fetch(dev);
  sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
  sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
  sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum);

  // Convert sensor values to °C, hPa, and % with 2 decimal places
  snprintk(json_buf, sizeof(json_buf),
           "{\"id\":\"%s\",\"temp\":%.2f,\"press\":%.2f,\"hum\":%.2f}",
           eui64_id, temp.val1 + temp.val2 / 1000000.0,
           press.val1 * 10.0 + press.val2 / 100000.0,
           hum.val1 + hum.val2 / 1000000.0);
  LOG_INF("JSON message: %s", json_buf);

  p_message = otCoapNewMessage(p_instance, NULL);
  if (p_message == NULL) {
    LOG_ERR("Failed to create message for CoAP Request");
    return;
  }

  otCoapMessageInit(p_message, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_PUT);

  error = otCoapMessageAppendUriPathOptions(p_message, "sensor");
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to append Uri-Path option for CoAP Request: %s",
            otThreadErrorToString(error));
    otMessageFree(p_message);
    return;
  }

  error = otCoapMessageAppendContentFormatOption(
      p_message, OT_COAP_OPTION_CONTENT_FORMAT_JSON);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to append content format option for CoAP Request: %s",
            otThreadErrorToString(error));
    otMessageFree(p_message);
    return;
  }

  error = otCoapMessageSetPayloadMarker(p_message);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to set payload marker for CoAP Request: %s",
            otThreadErrorToString(error));
    otMessageFree(p_message);
    return;
  }

  error = otMessageAppend(p_message, json_buf, strlen(json_buf));
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to append to CoAP Request message: %s",
            otThreadErrorToString(error));
    otMessageFree(p_message);
    return;
  }

  error = otCoapSendRequest(p_instance, p_message, &message_info, NULL, NULL);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to send CoAP Request: %s", otThreadErrorToString(error));
    otMessageFree(p_message);
    return;
  }

  LOG_INF("CoAP data sent");
}

void init_coap(void) {
  otInstance *p_instance = openthread_get_default_instance();
  otError error = otCoapStart(p_instance, OT_DEFAULT_COAP_PORT);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Cannot initialize CoAP: %s", otThreadErrorToString(error));
  }
}

/*
 * Get a device structure from a devicetree node with compatible
 * "bosch,bme280". (If there are multiple, just pick one.)
 */
static const struct device *get_bme280_device(void) {
  const struct device *const dev = DEVICE_DT_GET_ANY(bosch_bme280);

  if (dev == NULL) {
    /* No such node, or the node does not have status "okay". */
    LOG_ERR("Error: no device found.");
    return NULL;
  }

  if (!device_is_ready(dev)) {
    LOG_ERR("Error: Device \"%s\" is not ready; "
            "check the driver initialization logs for errors.",
            dev->name);
    return NULL;
  }

  LOG_INF("Found device \"%s\", getting sensor data", dev->name);
  return dev;
}

int main(void) {
  init_coap();

  const struct device *dev = get_bme280_device();

  if (dev == NULL) {
    LOG_ERR("Cannot initialize BME280 sensor");
    return 0;
  }

  while (1) {
    k_sleep(K_MSEC(5000));
    send_sensor_request(dev);
  }
  return 0;
}

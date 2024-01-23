/*
 * Copyright (c) 2024 Koen Vervloesem
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/coap.h>
#include <openthread/thread.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>

LOG_MODULE_REGISTER(ot_coap_led, LOG_LEVEL_DBG);

#define LED0_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int led_state = 0;

static void led_requested(void *p_context, otMessage *p_message,
                          const otMessageInfo *p_message_info);
static void led_send_response(otMessage *p_request_message,
                              const otMessageInfo *p_message_info);

static otCoapResource led_resource = {.mUriPath = "led",
                                      .mHandler = led_requested,
                                      .mContext = NULL,
                                      .mNext = NULL};

static void led_requested(void *p_context, otMessage *p_message,
                          const otMessageInfo *p_message_info) {
  otCoapCode method_code = otCoapMessageGetCode(p_message);
  otCoapType message_type = otCoapMessageGetType(p_message);
  char buf[1];
  int ret;

  if (message_type == OT_COAP_TYPE_CONFIRMABLE ||
      message_type == OT_COAP_TYPE_NON_CONFIRMABLE) {
    if (method_code == OT_COAP_CODE_PUT) {
      otMessageRead(p_message, otMessageGetOffset(p_message), buf, 1);
      LOG_INF("Received: %c", buf[0]);
      if (buf[0] == '0') {
        ret = gpio_pin_set_dt(&led, 0);
        led_state = 0;
      } else if (buf[0] == '1') {
        ret = gpio_pin_set_dt(&led, 1);
        led_state = 1;
      } else if (buf[0] == '2') {
        ret = gpio_pin_toggle_dt(&led);
        led_state = 1 - led_state;
      } else {
        LOG_ERR("Received unsupported payload: %c", buf[0]);
      }

      if (message_type == OT_COAP_TYPE_CONFIRMABLE) {
        led_send_response(p_message, p_message_info);
      }
    } else if (method_code == OT_COAP_CODE_GET) {
      led_send_response(p_message, p_message_info);
    }
  }
}

static void led_send_response(otMessage *p_request_message,
                              const otMessageInfo *p_message_info) {
  otError error;
  otMessage *p_response;
  otCoapCode response_code;
  otCoapType message_type;
  otInstance *p_instance = openthread_get_default_instance();
  char buf[1];

  p_response = otCoapNewMessage(p_instance, NULL);
  if (p_response == NULL) {
    LOG_ERR("Failed to create message for CoAP Response");
    return;
  }

  if (otCoapMessageGetType(p_request_message) == OT_COAP_TYPE_CONFIRMABLE) {
    message_type = OT_COAP_TYPE_ACKNOWLEDGMENT;
  } else if (otCoapMessageGetType(p_request_message) ==
             OT_COAP_TYPE_NON_CONFIRMABLE) {
    message_type = OT_COAP_TYPE_NON_CONFIRMABLE;
  } else {
    LOG_ERR("Unsupported message type in CoAP Request message: %d",
            otCoapMessageGetType(p_request_message));
    otMessageFree(p_response);
    return;
  }

  if (otCoapMessageGetCode(p_request_message) == OT_COAP_CODE_GET) {
    response_code = OT_COAP_CODE_CONTENT;
  } else if (otCoapMessageGetCode(p_request_message) == OT_COAP_CODE_PUT) {
    response_code = OT_COAP_CODE_CHANGED;
  } else {
    char *method_code_string = otCoapMessageCodeToString(p_request_message);
    LOG_ERR("Unsupported method code in CoAP Request message: %s",
            method_code_string);
    otMessageFree(p_response);
    return;
  }

  error = otCoapMessageInitResponse(p_response, p_request_message, message_type,
                                    response_code);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to initialize message for CoAP Response: %s",
            otThreadErrorToString(error));
    otMessageFree(p_response);
    return;
  }

  error = otCoapMessageSetPayloadMarker(p_response);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to set payload marker for CoAP Response: %s",
            otThreadErrorToString(error));
    otMessageFree(p_response);
    return;
  }

  buf[0] = led_state + 0x30;
  LOG_INF("LED state: %c", buf[0]);
  error = otMessageAppend(p_response, buf, 1);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to append to CoAP Response message: %s",
            otThreadErrorToString(error));
    otMessageFree(p_response);
    return;
  }

  error = otCoapSendResponse(p_instance, p_response, p_message_info);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to send CoAP Response: %s", otThreadErrorToString(error));
    otMessageFree(p_response);
  }
}

void init_coap(void) {
  otError error;
  otInstance *p_instance = openthread_get_default_instance();

  error = otCoapStart(p_instance, OT_DEFAULT_COAP_PORT);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Cannot initialize CoAP: %s", otThreadErrorToString(error));
    return;
  }
  LOG_INF("CoAP service started");
  otCoapAddResource(p_instance, &led_resource);
  LOG_INF("CoAP led resource started");
}

int init_led(void) {
  int ret;

  if (!gpio_is_ready_dt(&led)) {
    return 0;
  }

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return 0;
  }

  return 0;
}

int main(void) {
  int ret;

  init_led();
  init_coap();
  ret = gpio_pin_set_dt(&led, 0);

  return 0;
}

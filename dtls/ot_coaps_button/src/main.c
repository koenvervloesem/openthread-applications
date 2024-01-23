/*
 * Copyright (c) 2024 Koen Vervloesem
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/coap_secure.h>
#include <openthread/thread.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>

LOG_MODULE_REGISTER(ot_coaps_button, LOG_LEVEL_DBG);

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button =
    GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

const uint8_t *psk = "1234";
const uint8_t *psk_id = "my-id";
const char *server_address = "fd3a:3a7a:3ffe:406f:d732:851f:52af:fd79";

void client_connected(bool aConnected, void *aContext) {
  if (aConnected) {
    LOG_INF("DTLS client connected");
  } else {
    LOG_INF("DTLS client disconnected");
  }
}

static void send_led_request(void);
static void led_response_cb(void *p_context, otMessage *p_message,
                            const otMessageInfo *p_message_info,
                            otError result);

static void led_response_cb(void *p_context, otMessage *p_message,
                            const otMessageInfo *p_message_info,
                            otError result) {
  if (result == OT_ERROR_NONE) {
    LOG_INF("Delivery confirmed");
  } else {
    LOG_ERR("Delivery not confirmed: %s", otThreadErrorToString(result));
  }
}

static void send_led_request(void) {
  const char buf[1] = "2";
  otError error;
  otInstance *p_instance = openthread_get_default_instance();
  otMessage *p_message;

  p_message = otCoapNewMessage(p_instance, NULL);
  if (p_message == NULL) {
    LOG_ERR("Failed to create message for CoAP Request");
    return;
  }

  otCoapMessageInit(p_message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_PUT);

  error = otCoapMessageAppendUriPathOptions(p_message, "led");
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to append Uri-Path option for CoAP Request: %s",
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

  error = otMessageAppend(p_message, buf, 1);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to append to CoAP Request message: %s",
            otThreadErrorToString(error));
    otMessageFree(p_message);
    return;
  }

  error = otCoapSecureSendRequest(p_instance, p_message, led_response_cb, NULL);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to send CoAP Request: %s", otThreadErrorToString(error));
    otMessageFree(p_message);
    return;
  }

  LOG_INF("CoAP data sent");
}

void button_pressed(const struct device *dev, struct gpio_callback *cb,
                    uint32_t pins) {
  LOG_INF("Button pressed");
  send_led_request();
}

void init_coap(void) {
  otError error;
  otInstance *p_instance = openthread_get_default_instance();

  otCoapSecureSetPsk(p_instance, psk, strlen(psk), psk_id, strlen(psk_id));
  LOG_INF("PSK: %s", psk);
  LOG_INF("PSK id: %s", psk_id);

  error = otCoapSecureStart(p_instance, OT_DEFAULT_COAP_SECURE_PORT);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Cannot initialize CoAPS: %s", otThreadErrorToString(error));
    return;
  }
  LOG_INF("CoAP Secure service started");

  otSockAddr sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));
  error = otIp6AddressFromString(server_address, &sock_addr.mAddress);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Error %s: Cannot parse IPv6 address\n",
            otThreadErrorToString(error));
    return;
  }
  sock_addr.mPort = OT_DEFAULT_COAP_SECURE_PORT;

  error = otCoapSecureConnect(p_instance, &sock_addr, client_connected, NULL);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Cannot initialize DTLS session: %s", otThreadErrorToString(error));
    return;
  }
  LOG_INF("DTLS session initialized");
}

int init_button(void) {

  int ret;

  if (!gpio_is_ready_dt(&button)) {
    LOG_ERR("Error: button device %s is not ready", button.port->name);
    return 0;
  }

  ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure %s pin %d", ret, button.port->name,
            button.pin);
    return 0;
  }

  ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret,
            button.port->name, button.pin);
    return 0;
  }

  gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
  gpio_add_callback(button.port, &button_cb_data);
  LOG_INF("Set up button at %s pin %d", button.port->name, button.pin);

  return 0;
}

int main(void) {
  init_button();
  init_coap();
  return 0;
}

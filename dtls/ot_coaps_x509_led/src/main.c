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

LOG_MODULE_REGISTER(ot_coaps_x509_led, LOG_LEVEL_DBG);

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

#define DTLS_PRIVKEY                                                           \
  "-----BEGIN EC PRIVATE KEY-----\r\n"                                         \
  "MHcCAQEEIA0qy87WiS6egPu3YpBc/TRAkaZKs//b1OfUoW+sAOOjoAoGCCqGSM49\r\n"       \
  "AwEHoUQDQgAEOYqjUFM2JhpsMWEzmlRYal+XDfNnnCH3YGjOBo9SYlknvb+2K0Nb\r\n"       \
  "vq5u8EO3yDjO5SOC0rvLxrFxZc1w6kmquw==\r\n"                                   \
  "-----END EC PRIVATE KEY-----\r\n"

#define DTLS_X509_CERT                                                         \
  "-----BEGIN CERTIFICATE-----\r\n"                                            \
  "MIIB0TCCAXYCAQEwCgYIKoZIzj0EAwIwcjELMAkGA1UEBhMCQkUxFzAVBgNVBAgM\r\n"       \
  "DlZsYWFtcy1CcmFiYW50MRAwDgYDVQQHDAdHZWxyb2RlMRgwFgYDVQQKDA9Lb2Vu\r\n"       \
  "LVZlcnZsb2VzZW0xHjAcBgNVBAMMFWNhLmtvZW4udmVydmxvZXNlbS5ldTAeFw0y\r\n"       \
  "MzEyMDMxNDE1MTVaFw0yNDEyMDIxNDE1MTVaMHYxCzAJBgNVBAYTAkJFMRcwFQYD\r\n"       \
  "VQQIDA5WbGFhbXMtQnJhYmFudDEQMA4GA1UEBwwHR2Vscm9kZTEYMBYGA1UECgwP\r\n"       \
  "S29lbi1WZXJ2bG9lc2VtMSIwIAYDVQQDDBlzZXJ2ZXIua29lbi52ZXJ2bG9lc2Vt\r\n"       \
  "LmV1MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEOYqjUFM2JhpsMWEzmlRYal+X\r\n"       \
  "DfNnnCH3YGjOBo9SYlknvb+2K0Nbvq5u8EO3yDjO5SOC0rvLxrFxZc1w6kmquzAK\r\n"       \
  "BggqhkjOPQQDAgNJADBGAiEAxOM2UdS325QoKTl5v2wVNsbH2jboeuRzoRNCajgK\r\n"       \
  "zXYCIQDU+0L7R73VAAFZVEsWV0vX4YR9uqv+BK9qK6YRDtq1yQ==\r\n"                   \
  "-----END CERTIFICATE-----\r\n"

#define DTLS_CA_CERT                                                           \
  "-----BEGIN CERTIFICATE-----\r\n"                                            \
  "MIICOTCCAd+gAwIBAgIUHqT20DBUAHHT904VeSygpNjhRbAwCgYIKoZIzj0EAwIw\r\n"       \
  "cjELMAkGA1UEBhMCQkUxFzAVBgNVBAgMDlZsYWFtcy1CcmFiYW50MRAwDgYDVQQH\r\n"       \
  "DAdHZWxyb2RlMRgwFgYDVQQKDA9Lb2VuLVZlcnZsb2VzZW0xHjAcBgNVBAMMFWNh\r\n"       \
  "LmtvZW4udmVydmxvZXNlbS5ldTAeFw0yMzEyMDMxNDEzMTFaFw0yNDEyMDIxNDEz\r\n"       \
  "MTFaMHIxCzAJBgNVBAYTAkJFMRcwFQYDVQQIDA5WbGFhbXMtQnJhYmFudDEQMA4G\r\n"       \
  "A1UEBwwHR2Vscm9kZTEYMBYGA1UECgwPS29lbi1WZXJ2bG9lc2VtMR4wHAYDVQQD\r\n"       \
  "DBVjYS5rb2VuLnZlcnZsb2VzZW0uZXUwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC\r\n"       \
  "AAT7TMwcbPf748E6IYAbiVMf/oFO+rJr24HNbd/4RMgiDPJVegkgrQyF9TYYFTNz\r\n"       \
  "jK8l9r+826mdYapJiAv3UcKvo1MwUTAdBgNVHQ4EFgQUAb6z6lBPYE1ooUoS/DkS\r\n"       \
  "wdx+CHowHwYDVR0jBBgwFoAUAb6z6lBPYE1ooUoS/DkSwdx+CHowDwYDVR0TAQH/\r\n"       \
  "BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiB2WYi+zjPa86u4tFLwIX11NfDpWotr\r\n"       \
  "oTAypdBmJVcPEwIhAMpkY0s7hg6xp0gWHUh61mymvOiTiTLAZtrzKNECif9u\r\n"           \
  "-----END CERTIFICATE-----\r\n"

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

  error = otCoapSecureSendResponse(p_instance, p_response, p_message_info);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Failed to send CoAP Response: %s", otThreadErrorToString(error));
    otMessageFree(p_response);
  }
}

void init_coap(void) {
  otError error;
  otInstance *p_instance = openthread_get_default_instance();

  otCoapSecureSetCertificate(
      p_instance, (const uint8_t *)DTLS_X509_CERT, sizeof(DTLS_X509_CERT),
      (const uint8_t *)DTLS_PRIVKEY, sizeof(DTLS_PRIVKEY));
  otCoapSecureSetCaCertificateChain(p_instance, (const uint8_t *)DTLS_CA_CERT,
                                    sizeof(DTLS_CA_CERT));
  otCoapSecureSetSslAuthMode(p_instance, true);

  error = otCoapSecureStart(p_instance, OT_DEFAULT_COAP_SECURE_PORT);
  if (error != OT_ERROR_NONE) {
    LOG_ERR("Cannot initialize CoAPS: %s", otThreadErrorToString(error));
    return;
  }
  LOG_INF("CoAP Secure service started");
  otCoapSecureAddResource(p_instance, &led_resource);
  LOG_INF("CoAP Secure led resource started");
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

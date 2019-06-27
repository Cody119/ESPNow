
#ifndef ESP_NOW_H
#define ESP_NOW_H

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "esp_log.h"

#define WESP_NOW_TAG "EspNow Wrapper"
#define WESP_NOW_WIFI_CHANNEL 1
#define WESP_NOW_EVENT_QUEUE_SIZE 50

#define PRIMARY_KEY "pmk1234567890123"
#define LOCAL_KEY "lmk1234567890123"
#define BROAD_CAST_ADDRESS { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

#define WESP_NOW_STA

#ifdef WESP_NOW_STA

#define WESP_NOW_THIS_MODE WIFI_MODE_STA
#define WESP_NOW_PEER_MODE ESP_IF_WIFI_STA

#else

//Needs to be changed
#define WESP_NOW_THIS_MODE WIFI_MODE_STA
#define WESP_NOW_PEER_MODE ESP_IF_WIFI_STA

#endif

enum espNowEventType_t {
    SEND, RECV
};

#define HEADER_SIZE (sizeof(uint16_t))
#define PAYLOAD_SIZE (250 - HEADER_SIZE)

typedef void (*wespNowRecvCb)(uint8_t sender_mac[ESP_NOW_ETH_ALEN], uint16_t seq_num, uint8_t *data, uint16_t len);
typedef void (*wespNowSendCb)(const uint8_t *mac_addr, esp_now_send_status_t status, void *usr);

typedef struct {
    uint16_t seq_num;                     //Sequence number of ESPNOW data.
    uint8_t payload[PAYLOAD_SIZE];        //Real payload of ESPNOW data.
} __attribute__((packed)) espNowPacket_t;

typedef struct {
    enum espNowEventType_t id;

    union {

        struct {
            uint8_t mac[ESP_NOW_ETH_ALEN];
            uint8_t data[PAYLOAD_SIZE];
            uint16_t len;
            uint16_t seq;
            wespNowSendCb send_cb;
            void *usr;
        } sendData;

        struct {
            uint8_t mac[ESP_NOW_ETH_ALEN];
            espNowPacket_t data;
            uint16_t len;
        } recvData;
    } eventData;
} espNowEvent_t;

typedef struct {
    xQueueHandle eventQueue;
    TaskHandle_t taskHandle;
    wespNowRecvCb recvEvent;
} espNowHandle_t;

espNowHandle_t *espNowWrapper(uint8_t (*peer_macs)[ESP_NOW_ETH_ALEN], uint16_t length, wespNowRecvCb recvCb, void (**task)(void*));
void espNowLogMac();
void espNowSendFromISR(espNowHandle_t *handle, uint8_t mac[ESP_NOW_ETH_ALEN], uint16_t seq, void * data, uint8_t len, wespNowSendCb send_cb, void *usr);
void espNowSendAsync(espNowHandle_t *handle, uint8_t mac[ESP_NOW_ETH_ALEN], uint16_t seq, void * data, uint8_t len, wespNowSendCb send_cb, void *usr);

#endif
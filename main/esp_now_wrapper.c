#include "esp_now_wrapper.h"

espNowHandle_t wrapperHandle;
esp_now_send_status_t last_status;
static void espNowSendCb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    switch (status) {
        case ESP_NOW_SEND_SUCCESS:
            ESP_LOGI(WESP_NOW_TAG, "Packet sent to mac: "MACSTR, MAC2STR(mac_addr));
            break;
        case ESP_NOW_SEND_FAIL:
            if (mac_addr == NULL) {
                ESP_LOGE(WESP_NOW_TAG, "Failed to send, No mac adress or incorrect mac adress");
            } else {
                ESP_LOGE(WESP_NOW_TAG, "Failed to send to mac: "MACSTR, MAC2STR(mac_addr));
            }
            break;
        default:
            break;
    }
    last_status = status;
    xTaskNotifyGive(wrapperHandle.taskHandle);
}

static void espNowRecvCb(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (mac_addr == NULL) {
        ESP_LOGE(WESP_NOW_TAG, "Receive packet with no mac (indicates decode error)");
        return;
    }
    if (data == NULL) {
        ESP_LOGE(WESP_NOW_TAG, "Recived packet from "MACSTR" with no/corrupt data", MAC2STR(mac_addr));
        return;
    }
    if (len <= 0) {
        ESP_LOGI(WESP_NOW_TAG, "Recived empty packet from "MACSTR, MAC2STR(mac_addr));
        return;
    }
    ESP_LOGI(WESP_NOW_TAG, "Recived packet from "MACSTR, MAC2STR(mac_addr));    
    esp_log_buffer_hex(WESP_NOW_TAG, data, len);

    uint8_t *d = malloc(sizeof(uint8_t) * len);
    memcpy(d, data, len);
    espNowEvent_t event;
    event.id = RECV;
    event.eventData.sendData.data = d;
    event.eventData.recvData.len = len;
    BaseType_t ret = xQueueSend(wrapperHandle.eventQueue, &event, 0);
}

void cleanSendEvent(espNowEvent_t *event) {
    free(event->eventData.sendData.data);
}

void cleanRecvEvent(espNowEvent_t *event) {
    free(event->eventData.recvData.data);
}

static void espNowTask( void * pvParameters ) {
    espNowHandle_t *handle = (espNowHandle_t*)pvParameters;
    xQueueHandle eventQueue = handle->eventQueue;
    espNowEvent_t event;

    for (;;) {
        if (xQueueReceive(eventQueue, &event, portMAX_DELAY)) {
            switch (event.id) {
                case SEND: {
                    ESP_LOGI(WESP_NOW_TAG, "Sending to mac: "MACSTR, MAC2STR(event.eventData.sendData.mac));
                    
                    uint32_t psize = sizeof(espNowPacket_t)  + event.eventData.sendData.len;
                    espNowPacket_t *packet = malloc(psize);
                    packet->seq_num = 0;
                    memcpy(packet->payload, event.eventData.sendData.data, event.eventData.sendData.len);
                    //packet->crc = crc16_le(UINT16_MAX, (uint8_t const *)packet, sizeof(espNowPacket_t) + sizeof(uint8_t));
                    esp_log_buffer_hex(WESP_NOW_TAG, (uint8_t*)packet, psize);
                    ESP_ERROR_CHECK( esp_now_send(event.eventData.sendData.mac, (const uint8_t *)packet, psize) );

                    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                    if (ulNotificationValue == 1 && last_status == ESP_NOW_SEND_SUCCESS) {
                        ESP_LOGI(WESP_NOW_TAG, "Good Send");
                    } else {
                        ESP_LOGI(WESP_NOW_TAG, "Bad Send, Notify: %d, LastSend: %d", ulNotificationValue, last_status);
                    }

                    free(packet);
                    cleanSendEvent(&event);

                    break;
                }
                
                case RECV: {
                    if (handle->recvEvent)
                        (*handle->recvEvent)(
                            event.eventData.recvData.mac,
                            (espNowPacket_t*)event.eventData.recvData.data,
                            event.eventData.recvData.len - sizeof(espNowPacket_t)
                        );
                    cleanRecvEvent(&event);
                    break;
                }
                
                default:
                    ESP_LOGI(WESP_NOW_TAG, "Recived Unhandled event %d", event.id);
            }
        }
    }
}

void espNowLogMac() {
    uint8_t mac[ESP_NOW_ETH_ALEN];
    esp_efuse_mac_get_default(mac);
    ESP_LOGI(WESP_NOW_TAG, "MAC Adress: "MACSTR, MAC2STR(mac));
    wifi_mode_t x;
    esp_wifi_get_mode(&x);
    ESP_LOGI(WESP_NOW_TAG, "Mode: %d", x);
    esp_wifi_get_mac(x, mac);
    ESP_LOGI(WESP_NOW_TAG, "Mode MAC: "MACSTR, MAC2STR(mac));
    
}

void espNowInit() {
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espNowSendCb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espNowRecvCb) );
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)PRIMARY_KEY) );
}

// static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// uint8_t MAC12[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xA4, 0x36, 0x45, 0xF4};

void espNowAddPeer(uint8_t peer_mac[ESP_NOW_ETH_ALEN]) {
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = WESP_NOW_WIFI_CHANNEL;
    peer.ifidx = WESP_NOW_PEER_MODE;
    // peer.encrypt = false;
    peer.encrypt = true;
    // memcpy(peer.peer_addr, MAC12, ESP_NOW_ETH_ALEN);
    memcpy(peer.peer_addr, peer_mac, ESP_NOW_ETH_ALEN);
    memcpy(peer.lmk, LOCAL_KEY, ESP_NOW_KEY_LEN);
    ESP_ERROR_CHECK( esp_now_add_peer(&peer) );
}

espNowHandle_t *espNowWrapper(uint8_t (*peer_macs)[ESP_NOW_ETH_ALEN], uint16_t length) {//}, size_t length) {
    espNowInit();
    
    size_t i = 0;
    uint8_t (*next_mac)[ESP_NOW_ETH_ALEN] = peer_macs;
    for (; i < length; i++) {
        espNowAddPeer(*peer_macs);
        ESP_LOGI(WESP_NOW_TAG, "Added peer with mac: "MACSTR, MAC2STR(*peer_macs));
        next_mac++;
    }

    espNowHandle_t handle;// = malloc(sizeof(espNowHandle_t));
    handle.eventQueue = xQueueCreate(WESP_NOW_EVENT_QUEUE_SIZE, sizeof(espNowEvent_t));
    handle.recvEvent = NULL;
    wrapperHandle = handle;
    BaseType_t ret = xTaskCreate(espNowTask, WESP_NOW_TAG, 2048, &wrapperHandle, 4, &(wrapperHandle.taskHandle));
    

    return &wrapperHandle;
}
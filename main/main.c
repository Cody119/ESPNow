#include "main.h"

//-------------------------------------
// MAC Adresses for used devices
//-------------------------------------

//"Reworked" (COM12?) The recieve device
// 30 ae a4 36 45 f4 - Factory
// 30 ae a4 36 45 f4 - Station
uint8_t MAC1[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xA4, 0x36, 0x45, 0xF4};
//Other board (COM13?) the button device
// 30 ae a4 0d 70 a4 - Factory
// 30 ae a4 0d 70 a5 - Station
uint8_t MAC2[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xA4, 0x0D, 0x70, 0xA4};


// -------------------------------------------------------------------------------------------------------
// Modified RSSI logger (Source: https://gist.github.com/Staubgeborener/28d571ef812303cbf47915b81c158576)
// -------------------------------------------------------------------------------------------------------

// This struct was obtained from https://www.hackster.io/p99will/esp32-wifi-mac-scanner-sniffer-promiscuous-4c12f4
typedef struct { 
  int16_t fctl;
  int16_t duration;
  uint8_t da[6];
  uint8_t sa[6];
  uint8_t bssid[6];
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

#ifdef BUTTON
#define WHITE_LIST MAC1
#else
#define WHITE_LIST MAC2
#endif


void wifi_promiscuous(void* buffer, wifi_promiscuous_pkt_type_t type)
{
    //parsing buffer data
    wifi_promiscuous_pkt_t* p = (wifi_promiscuous_pkt_t*)buffer;

    //see: https://github.com/espressif/esp-idf/blob/8e4a8e17037c51ce2452e55b5b75bbfaecb25838/components/esp32/include/esp_wifi_types.h#L192
    int len = p->rx_ctrl.sig_len;
    WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
    if (len < 0){
        return;
    }
    if (memcmp(WHITE_LIST, wh->sa, 6) == 0) {
        ESP_LOGD(WESP_NOW_TAG, "Recived packet with RSSI: %02d from "MACSTR, p->rx_ctrl.rssi, MAC2STR(wh->sa));
    }
}

void logRSSI() {
    esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous); //set promiscuous callback
    esp_wifi_set_promiscuous(true); //as soon this flag is true, the callback will triggered for each packet
}



//------------------------------------------------------------------------------------------

/* WiFi should start before using ESPNOW */
static void wifi_init() {
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WESP_NOW_THIS_MODE) );

    ESP_ERROR_CHECK( esp_wifi_start());

    /* Set channel */
    ESP_ERROR_CHECK( esp_wifi_set_channel(WESP_NOW_WIFI_CHANNEL, 0) );

// #if CONFIG_ESPNOW_ENABLE_LONG_RANGE
//     ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
// #endif

    /* From here you could use the following for normal wifi, but for esp now its not used */
    // wifi_config_t sta_config = {
    //     .sta = {
    //         .ssid = CONFIG_ESP_WIFI_SSID,
    //         .password = CONFIG_ESP_WIFI_PASSWORD,
    //         .bssid_set = false
    //     }
    // };
    // ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    // ESP_ERROR_CHECK( esp_wifi_start() );
    // ESP_ERROR_CHECK( esp_wifi_connect() );

}

static xQueueHandle evt_queue = NULL;

#ifdef BUTTON

TickType_t prev = 0;
uint16_t count = 0;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    TickType_t t = xTaskGetTickCountFromISR();
    if (t - prev < (250/portTICK_PERIOD_MS)) { 
        return;
    }
    prev = t;

    int16_t *d = malloc(sizeof(int16_t));
    count++;
    *d = count;
    espNowEvent_t event;
    event.id = SEND;
    event.eventData.sendData.data = d;
    event.eventData.sendData.len = sizeof(int16_t);
    memcpy(event.eventData.sendData.mac, MAC1, ESP_NOW_ETH_ALEN);
    xQueueSendFromISR(evt_queue, &event, NULL);
}

static void recvEvent(uint8_t sender_mac[ESP_NOW_ETH_ALEN], espNowPacket_t *data, uint16_t len){
    // char *d = malloc(sizeof(len));
    // memcpy(d, data->payload, len);
    espNowEvent_t event;
    event.id = SEND;
    event.eventData.sendData.data = NULL;
    event.eventData.sendData.len = 0;
    event.eventData.sendData.seq = data->seq_num;
    memcpy(event.eventData.sendData.mac, sender_mac, ESP_NOW_ETH_ALEN);
    xQueueSend(evt_queue, &event, 0);
};

#else

static void recvEvent(uint8_t sender_mac[ESP_NOW_ETH_ALEN], espNowPacket_t *data, uint16_t len){
    if (len > 0) {
        ESP_LOGD(WESP_NOW_TAG, "Got marker return %d", *(uint16_t*)data->payload );
    } else {
        ESP_LOGD(WESP_NOW_TAG, "Got packet %d", data->seq_num );
    }
}

uint16_t seq = 0;
static void pingTask( void * pvParameters ) {
    espNowEvent_t event;
    event.id = SEND;
    event.eventData.sendData.data = NULL;
    event.eventData.sendData.len = 0;
    event.eventData.sendData.seq = seq;
    seq++;
    memcpy(event.eventData.sendData.mac, MAC2, ESP_NOW_ETH_ALEN);

    for (;;) {
        vTaskDelay(500/portTICK_PERIOD_MS);
        event.eventData.sendData.seq = seq;
        seq++;
        ESP_LOGD(WESP_NOW_TAG, "Sending packet %d", seq );
        xQueueSend(evt_queue, &event, 0);
    }
}

#endif



void app_main(void)
{
    nvs_flash_init();
    wifi_init();

#ifdef BUTTON
    espNowHandle_t *espNowQueue = espNowWrapper(&MAC1, 1);
    evt_queue = espNowQueue->eventQueue;
    espNowQueue->recvEvent = &recvEvent;
#else

    espNowHandle_t *espNowQueue = espNowWrapper(&MAC2, 1);
    evt_queue = espNowQueue->eventQueue;
    espNowQueue->recvEvent = &recvEvent;
#endif

    espNowLogMac();
    
#ifdef BUTTON
    gpio_config_t config = {
        .pin_bit_mask = GPIO_SEL_32,
        .mode = GPIO_MODE_DEF_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    ESP_ERROR_CHECK( gpio_config(&config) );

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_NUM_32, gpio_isr_handler, NULL);
#else

    gpio_pad_select_gpio(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
    BaseType_t ret = xTaskCreate(pingTask, "pingTask", 2048, NULL, 4, NULL);
#endif
    logRSSI();
}


#include "main.h"

//"Reworked" board 30 ae a4 36 45 f4
uint8_t MAC1[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xA4, 0x36, 0x45, 0xF4};
//Other board 30 ae a4 0d 70 a4
uint8_t MAC2[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xA4, 0x0D, 0x70, 0xA4};

/* WiFi should start before using ESPNOW */
static void wifi_init() {
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESP_NOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());

    /* Set channel */
    ESP_ERROR_CHECK( esp_wifi_set_channel(ESP_NOW_WIFI_CHANNEL, 0) );

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

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    //uint32_t x = gpio_get_level(GPIO_NUM_32);
    uint16_t *d = malloc(sizeof(uint16_t));
    *d = 77;
    espNowEvent_t event;
    event.id = SEND;
    event.eventData.sendData.data = d;
    memcpy(event.eventData.sendData.mac, MAC1, sizeof(MAC1));
    xQueueSendFromISR(evt_queue, &event, NULL);
}

void app_main(void)
{
    nvs_flash_init();
    wifi_init();
    espNowHandle_t *espNowQueue = espNowWrapper(&MAC1, 1);
    espNowLogMac();
    

    evt_queue = espNowQueue->eventQueue;
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
}


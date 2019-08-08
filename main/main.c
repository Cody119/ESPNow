#include "main.h"

// https://www.reddit.com/r/esp32/comments/9jmkf9/here_is_how_you_set_a_custom_mac_address_on_esp32/

//"Reworked" (COM12?)
// 30 ae a4 36 45 f4 - Factory
// 30 ae a4 36 45 f4 - Station
uint8_t MAC1[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xA4, 0x36, 0x45, 0xF4};
//Other board (COM13?)
// 30 ae a4 0d 70 a4 - Factory
// 30 ae a4 0d 70 a5 - Station
uint8_t MAC2[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xA4, 0x0D, 0x70, 0xA4};

uint8_t TEST_MAC[ESP_NOW_ETH_ALEN] = {0x30, 0xAE, 0xDE, 0xED, 0xFF, 0xFF};

/* WiFi should start before using ESPNOW */
static void wifi_init() {
    // esp_base_mac_addr_set(TEST_MAC);
    // esp_wifi_set_mac(ESP_IF_WIFI_STA, TEST_MAC);

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
}

#ifdef BUTTON

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint8_t d = 77;
    espNowSendFromISR((espNowHandle_t*)arg, MAC2, 0, &d, sizeof(uint8_t), NULL, NULL);
}
#else

uint32_t x = 0;
static void recvEvent(uint8_t sender_mac[ESP_NOW_ETH_ALEN], espNowPacket_t *data, uint16_t len){
    x = !x;
    ESP_ERROR_CHECK( gpio_set_level(GPIO_NUM_13, x) );
};

#endif



void app_main(void)
{
    nvs_flash_init();
    wifi_init();

#ifdef BUTTON
    espNowHandle_t *espNowQueue = espNowWrapper(&MAC2, 1, NULL, NULL);
#else
    espNowHandle_t *espNowQueue = espNowWrapper(&MAC1, 1, &recvEvent, NULL);
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
    gpio_isr_handler_add(GPIO_NUM_32, gpio_isr_handler, espNowQueue);
#else

    gpio_pad_select_gpio(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
#endif
}


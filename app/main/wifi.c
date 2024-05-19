#include "main.h"

#include "esp_system.h" //esp_init funtions esp_err_t 
#include "esp_wifi.h" //esp_wifi_init functions and wifi operations
#include "esp_event.h" //for wifi event
#include "nvs_flash.h" //non volatile storage
#include "lwip/err.h" //light weight ip packets error handling
#include "lwip/sys.h" //system applications for light weight ip apps


const char *WIFI_TAG = "Wifi";
const char *ssid = "Nokia 8.3 5G";
const char *pass = "UnPostoAlCaldo";
int retry_num=0;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){

    switch(event_id){
    case WIFI_EVENT_STA_START: ESP_LOGI(WIFI_TAG, "WIFI CONNECTING....\n"); break;
    case WIFI_EVENT_STA_CONNECTED: ESP_LOGI(WIFI_TAG, "WiFi CONNECTED\n"); break;
    case WIFI_EVENT_STA_DISCONNECTED: 
        ESP_LOGI(WIFI_TAG, "WiFi lost connection\n");
        if(retry_num<5){esp_wifi_connect();retry_num++;ESP_LOGI(WIFI_TAG, "Retrying to Connect...\n");}
        break;
    case IP_EVENT_STA_GOT_IP: ESP_LOGI(WIFI_TAG, "Wifi got IP...\n\n"); break;
    }
}

void wifi_connection()
{
    esp_netif_init();
    esp_event_loop_create_default();     // event loop                    
    esp_netif_create_default_wifi_sta(); // WiFi station                      
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //     
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
            
           }
    
        };
    strcpy((char*)wifi_configuration.sta.ssid, ssid);
    strcpy((char*)wifi_configuration.sta.password, pass);    
    //esp_log_write(ESP_LOG_INFO, "Kconfig", "SSID=%s, PASS=%s", ssid, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    
    esp_wifi_connect();
    ESP_LOGI(WIFI_TAG,  "wifi_init_softap finished"); //. SSID:%s  password:%s",ssid,pass
    
}
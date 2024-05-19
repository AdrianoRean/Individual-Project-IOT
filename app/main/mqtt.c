#include "main.h"
#include "mqtt_client.h" //provides important functions to connect with MQTT
#include "esp_event.h" //managing events of mqtt
#include "esp_tls.h"

// docker run -p 8080:8080 -p 1883:1883 hivemq/hivemq4

const char *MQTT_TAG = "MQTT";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){ 
    ESP_LOGI(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client; 
    switch ((esp_mqtt_event_id_t)event_id){
        case MQTT_EVENT_CONNECTED: 
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(client,"Voltage Measurements",0); //in mqtt we require a topic to subscribe and client is from event client and 0 is quality of service it can be 1 or 2
            ESP_LOGI(MQTT_TAG, "sent subscribe successful" );
            break;
        case MQTT_EVENT_DISCONNECTED: ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED"); break;
        case MQTT_EVENT_SUBSCRIBED: ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED"); break;
        case MQTT_EVENT_UNSUBSCRIBED: ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED"); break;
        case MQTT_EVENT_DATA: ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA"); break;
        case MQTT_EVENT_ERROR: ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");break;
        default:
            ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void mqtt_initialize(void){

    
    ESP_LOGI(MQTT_TAG, "Creating config");

    const esp_mqtt_client_config_t mqtt_cfg={
        .broker = {
            .address.uri = "mqtts://d069030b4c20449cb72dd2416d630586.s1.eu.hivemq.cloud",
        },
        .credentials = {
            .username= "Rean-esp",
            .authentication.password = "Sottoscriv0!"
        }
    };

    
    ESP_LOGI(MQTT_TAG, "Creating Client");
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg); 
    
    ESP_LOGI(MQTT_TAG, "Registering client");
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    ESP_LOGI(MQTT_TAG, "Starting client");
    esp_mqtt_client_start(client); 
}

static void mqtt_task(void){
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(MQTT_TAG, "Starting MQTT...");
    mqtt_initialize();
    ESP_LOGI(MQTT_TAG, "Everything setted!");
    while(1){
        vTaskDelay(pdMS_TO_TICKS(4000));
    }

}
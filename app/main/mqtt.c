#include "main.h"
#include "certificate.h"
#include "time.c"

#include "mqtt_client.h" //provides important functions to connect with MQTT
#include "esp_event.h" //managing events of mqtt
#include "esp_tls.h"

const char *MQTT_TAG = "MQTT";
#define URI_MQTT "mqtts://d069030b4c20449cb72dd2416d630586.s1.eu.hivemq.cloud"
#define USERNAME "Rean-esp"
#define PASSWORD "Sottoscriv0!"
#define TOPIC "voltage"

esp_mqtt_client_handle_t client;

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
            ESP_LOGI(MQTT_TAG, "Other event id:%d", (int)event_id);
            break;
    }
}

static void mqtt_initialize(void){

    
    ESP_LOGI(MQTT_TAG, "Creating config");

    const esp_mqtt_client_config_t mqtt_cfg={
        .broker = {
            .address.uri = URI_MQTT,
            .verification.certificate = (const char*) cert_pem
        },
        .credentials = {
            .username= USERNAME,
            .authentication.password = PASSWORD
        }
    };

    
    ESP_LOGI(MQTT_TAG, "Creating Client");
    client = esp_mqtt_client_init(&mqtt_cfg); 
    
    ESP_LOGI(MQTT_TAG, "Registering client");
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    ESP_LOGI(MQTT_TAG, "Starting client");
    esp_mqtt_client_start(client); 
}

static void mqtt_task(void* buff){
    struct BufferStructure buffer_to_aggregate = *(struct BufferStructure*)buff;
    uint32_t voltage_buff[buffer_to_aggregate.size_of_buffer];

    ESP_LOGI(MQTT_TAG, "Starting MQTT...");
    mqtt_initialize();
    ESP_LOGI(MQTT_TAG, "Everything setted!");
    while(1){

        ESP_LOGI(MQTT_TAG, "Waiting for data...");
        xStreamBufferReceive( buffer_to_aggregate.buffer,
                             voltage_buff,
                             buffer_to_aggregate.size_of_buffer*sizeof(uint32_t),
                              portMAX_DELAY );
        
        ESP_LOGI(MQTT_TAG, "Data received!");

        uint32_t sum = 0;
        for (int j = 0; j < buffer_to_aggregate.size_of_buffer; j++){
            sum += voltage_buff[j];
        }

        float average = ((float)sum)/buffer_to_aggregate.size_of_buffer;

        ESP_LOGI(MQTT_TAG, "Average calculated");

        int len = snprintf(NULL, 0, "%f", average);
        char *payload = malloc(len + 1);
        snprintf(payload, len + 1, "%f", average);
        
        ESP_LOGI(MQTT_TAG, "Sending message...");
        //printTime();
        esp_mqtt_client_publish(client, TOPIC, payload, len, 0, 0);
        
        // do stuff with result
        free(payload);
        ESP_LOGI(MQTT_TAG, "Message sent!");
    }

}
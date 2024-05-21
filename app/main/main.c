#include "main.h"

#include <freertos/task.h>

#include "measure.c"
#include "fft.c"
#include "wifi.c"
#include "mqtt.c"

TaskHandle_t myTaskHandle = NULL;
TaskHandle_t regularTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;

uint32_t measurements[init_sample_size];
uint32_t* measurements_final;
struct BufferStructure buffer_struct;
struct BufferStructure buffer_struct2;

const char *MAIN_TAG = "Main";
const char *REGULAR_TAG = "Regular";

void regular_task(void){
    
    buffer_struct.wait_time = ceil(1000/init_sample_size);
    buffer_struct.size_of_buffer = init_sample_size;
    buffer_struct.buffer = xStreamBufferCreate( buffer_struct.size_of_buffer*sizeof(uint32_t),
                                           buffer_struct.size_of_buffer*sizeof(uint32_t) );
    buffer_struct.loop = false;
    
    ESP_LOGI(REGULAR_TAG, "Created buffer");
    
    ESP_LOGI(REGULAR_TAG, "First measuring...");

    xTaskCreatePinnedToCore(measure_task, "measure_task", sizeof(uint32_t)*init_sample_size + 4096, &buffer_struct, 10, &myTaskHandle, 1);

    int n_b = xStreamBufferReceive( buffer_struct.buffer,
                             measurements,
                             sizeof(uint32_t)*init_sample_size,
                             1000*60 );

    ESP_LOGI(REGULAR_TAG, "Data needed: %d, Data received: %d", sizeof(uint32_t)*(int)init_sample_size, n_b);

    vStreamBufferDelete(buffer_struct.buffer);
    
    ESP_LOGI(REGULAR_TAG, "Done!");
    
    ESP_LOGI(REGULAR_TAG, "Doing FFT...");

    float max_frequency = compute_max_frequency(measurements, 1000);
    
    ESP_LOGI(REGULAR_TAG, "Done!");
    
    ESP_LOGI(REGULAR_TAG, "Creating new buffer...");

    //COmpute parameters and variables for usual work
    if(max_frequency == 0){
        ESP_LOGI(REGULAR_TAG, "Max frequency not correct!");
        vTaskDelete(NULL);
    }
    float wait_time = 1000/(max_frequency*2);
    buffer_struct2.wait_time = floor(wait_time);
    buffer_struct2.size_of_buffer = ceil(2000/wait_time);
    buffer_struct2.buffer = xStreamBufferCreate( buffer_struct2.size_of_buffer*sizeof(uint32_t),
                                           buffer_struct2.size_of_buffer*sizeof(uint32_t) );
    buffer_struct2.loop = true;
    
    measurements_final = (uint32_t *)calloc(buffer_struct2.size_of_buffer, sizeof(uint32_t));

    ESP_LOGI(REGULAR_TAG, "Creating Mqtt task");

    xTaskCreate(mqtt_task, "mqtt_task", sizeof(uint32_t)*buffer_struct2.size_of_buffer + 4096, &buffer_struct2, 10, &mqttTaskHandle);
    
    ESP_LOGI(REGULAR_TAG, "Creating Measuring task for loop");
    xTaskCreatePinnedToCore(measure_task, "measure_task", sizeof(uint32_t)*buffer_struct2.size_of_buffer + 4096, &buffer_struct2, 10, &myTaskHandle, 1);

    ESP_LOGI(REGULAR_TAG, "Exiting");
    vTaskDelete(NULL);
}

void app_main(void)
{ 
    
    nvs_flash_init();
    ESP_LOGI(MAIN_TAG, "Setting wifi");
    wifi_connection();

    esp_err_t ret;
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK) {
        ESP_LOGI(MAIN_TAG, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }
    ESP_LOGI(MAIN_TAG, "FTT ready to work");

    ESP_LOGI(MAIN_TAG, "Delay to set things");

    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(MAIN_TAG, "starting regular task");
    xTaskCreate(regular_task, "regular_task", sizeof(uint32_t)*5000 + 4096, NULL, 10, &regularTaskHandle);
    
}


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

void regular_task(void){
    
    buffer_struct.wait_time = ceil(1000/init_sample_size);
    buffer_struct.size_of_buffer = init_sample_size;
    buffer_struct.buffer = xStreamBufferCreate( buffer_struct.size_of_buffer*sizeof(uint32_t),
                                           buffer_struct.size_of_buffer*sizeof(uint32_t) );
    buffer_struct.loop = false;
    
    ESP_LOGI(MAIN_TAG, "Created buffer\n");
    
    ESP_LOGI(MAIN_TAG, "First measuring...\n");

    xTaskCreatePinnedToCore(measure_task, "measure_task", sizeof(uint32_t)*init_sample_size + 4096, &buffer_struct, 10, &myTaskHandle, 1);

    int n_b = xStreamBufferReceive( buffer_struct.buffer,
                             measurements,
                             sizeof(uint32_t)*init_sample_size,
                             1000*60 );

    ESP_LOGI(MAIN_TAG, "Data needed: %d, Data received: %d\n", sizeof(uint32_t)*(int)init_sample_size, n_b);

    vStreamBufferDelete(buffer_struct.buffer);
    
    ESP_LOGI(MAIN_TAG, "Done!\n");
    
    ESP_LOGI(MAIN_TAG, "Doing FFT...\n");

    float max_frequency = compute_max_frequency(measurements, 1000);
    
    ESP_LOGI(MAIN_TAG, "Done!\n");
    
    ESP_LOGI(MAIN_TAG, "Creating new buffer...\n");

    //COmpute parameters and variables for usual work
    if(max_frequency == 0){
        ESP_LOGI(MAIN_TAG, "Max frequency not correct!\n");
        vTaskDelete(NULL);
    }
    float wait_time = 1000/(max_frequency*2);
    buffer_struct2.wait_time = floor(wait_time);
    buffer_struct2.size_of_buffer = ceil(2000/wait_time);
    buffer_struct2.buffer = xStreamBufferCreate( buffer_struct2.size_of_buffer*sizeof(uint32_t),
                                           buffer_struct2.size_of_buffer*sizeof(uint32_t) );
    buffer_struct2.loop = true;
    
    measurements_final = (uint32_t *)calloc(buffer_struct2.size_of_buffer, sizeof(uint32_t));
    
    ESP_LOGI(MAIN_TAG, "--- Looping ---\n");
    xTaskCreatePinnedToCore(measure_task, "measure_task", sizeof(uint32_t)*buffer_struct2.size_of_buffer + 4096, &buffer_struct2, 10, &myTaskHandle, 1);
    while(1){
        
        ESP_LOGI(MAIN_TAG, "Waiting for data...\n");
        xStreamBufferReceive( buffer_struct2.buffer,
                             measurements_final,
                             buffer_struct2.size_of_buffer*sizeof(uint32_t),
                              portMAX_DELAY );
        
        ESP_LOGI(MAIN_TAG, "Data received!\n");

        uint32_t sum = 0;
        for (int j = 0; j < buffer_struct2.size_of_buffer; j++){
            sum += measurements_final[j];
        }

        float average = ((float)sum)/buffer_struct2.size_of_buffer;

        ESP_LOGI(MAIN_TAG, "Average is: %f\n", average);
    }
}

void app_main(void)
{ 
    
    nvs_flash_init();
    wifi_connection();
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //mqtt_initialize();

    xTaskCreate(mqtt_task, "mqtt_task", 4096, NULL, 10, &mqttTaskHandle);

    esp_err_t ret;
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK) {
        ESP_LOGI(MAIN_TAG, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }
    ESP_LOGI(MAIN_TAG, "FTT ready to work\n");

    //xTaskCreate(regular_task, "regular_task", sizeof(uint32_t)*5000 + 4096, NULL, 10, &regularTaskHandle);
    
}


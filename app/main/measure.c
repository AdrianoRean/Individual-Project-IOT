#include "main.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

const char *MEASURE_TAG = "Measure";

static esp_adc_cal_characteristics_t adc1_chars;

void measure_task(void* buff){
    struct BufferStructure buffer_to_aggregate = *(struct BufferStructure*)buff;
    uint32_t voltage_buff[buffer_to_aggregate.size_of_buffer];
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11));
    
    ESP_LOGI(MEASURE_TAG, "Wait time %d",buffer_to_aggregate.wait_time);

    if (!buffer_to_aggregate.loop){
        ESP_LOGI(MEASURE_TAG, "No loop");
        for (int i = 0; i < buffer_to_aggregate.size_of_buffer; i++){
            voltage_buff[i] = (uint32_t)esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_0), &adc1_chars);
            //ESP_LOGI(MEASURE_TAG, "Measurement %d, value: %"PRIu32"",i,voltage_buff[i]);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        int n_b = xStreamBufferSend( buffer_to_aggregate.buffer,
                            voltage_buff,
                            buffer_to_aggregate.size_of_buffer*sizeof(uint32_t),
                            0);
        
        ESP_LOGI(MEASURE_TAG, "Data expected: %d, Data written: %d", sizeof(uint32_t)*(int)init_sample_size, n_b);
        
        ESP_LOGI(MEASURE_TAG, "Task closing");
        vTaskDelete( NULL );
    }
    else{
        
        ESP_LOGI(MEASURE_TAG, "Loop");
        while(1){
            
            ESP_LOGI(MEASURE_TAG, "Measuring %d samples...", buffer_to_aggregate.size_of_buffer);
            for (int i = 0; i < buffer_to_aggregate.size_of_buffer; i++){
                voltage_buff[i] = (uint32_t)esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_0), &adc1_chars);
                vTaskDelay(pdMS_TO_TICKS(buffer_to_aggregate.wait_time));
            }
            ESP_LOGI(MEASURE_TAG, "Sending Data");
            xStreamBufferSend( buffer_to_aggregate.buffer,
                                voltage_buff,
                                sizeof(uint32_t)*buffer_to_aggregate.size_of_buffer,
                                0);
        }
    }
    
}
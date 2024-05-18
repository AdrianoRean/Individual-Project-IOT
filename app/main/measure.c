#include "main.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

static esp_adc_cal_characteristics_t adc1_chars;

void measure_task(void* buff){
    struct BufferStructure buffer_to_aggregate = *(struct BufferStructure*)buff;
    uint32_t voltage_buff[buffer_to_aggregate.size_of_buffer];
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11));
    
    printf("Wait time %d\n",buffer_to_aggregate.wait_time);

    if (!buffer_to_aggregate.loop){
        printf("No loop\n");
        for (int i = 0; i < buffer_to_aggregate.size_of_buffer; i++){
            voltage_buff[i] = (uint32_t)esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_0), &adc1_chars);
            //printf("Measurement %d, value: %"PRIu32"\n",i,voltage_buff[i]);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        int n_b = xStreamBufferSend( buffer_to_aggregate.buffer,
                            voltage_buff,
                            buffer_to_aggregate.size_of_buffer*sizeof(uint32_t),
                            0);
        
        printf("Data expected: %d, Data written: %d\n", sizeof(uint32_t)*(int)init_sample_size, n_b);
        
        printf("Task closing\n");
        vTaskDelete( NULL );
    }
    else{
        
        printf("Loop\n");
        while(1){
            for (int i = 0; i < buffer_to_aggregate.size_of_buffer; i++){
                voltage_buff[i] = (uint32_t)esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_0), &adc1_chars);
                printf("Measurement %d\n",i);
                vTaskDelay(buffer_to_aggregate.wait_time);
            }
            xStreamBufferSend( buffer_to_aggregate.buffer,
                                voltage_buff,
                                sizeof(uint32_t)*buffer_to_aggregate.size_of_buffer,
                                0);
        }
    }
    
}
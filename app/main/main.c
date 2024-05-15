#include <stdio.h>
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

#include "esp_dsp.h"
#include <math.h>

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <inttypes.h>

#include "freertos/stream_buffer.h"

TaskHandle_t myTaskHandle = NULL;
static esp_adc_cal_characteristics_t adc1_chars;
struct BufferStructure {   // Structure declaration
    int wait_time;
    int size_of_buffer;           // Member (int variable)
    StreamBufferHandle_t buffer;       // Member (char variable)
};
int init_sample_size = 1000;

float compute_max_frequency(float *data, int length, int sampling_frequency)
{
    
    float y_cf[length * 2];
    float *y1_cf = &y_cf[0];
    float *y2_cf = &y_cf[length];
    float sum_y[length / 2];

    for (int i = 0 ; i < length ; i++) {
        y_cf[i * 2 + 0] = data[i] ;
        y_cf[i * 2 + 1] = 0;
    }
    // FFT
    dsps_fft2r_fc32(y_cf, length);
    // Bit reverse
    dsps_bit_rev_fc32(y_cf, length);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(y_cf, length);
    

    for (int i = 0 ; i < length / 2 ; i++) {
        y_cf[i] = 10 * log10f((y_cf[i * 2 + 0] * y_cf[i * 2 + 0] + y_cf[i * 2 + 1] * y_cf[i * 2 + 1]) / length);
        y2_cf[i] = 10 * log10f((y2_cf[i * 2 + 0] * y2_cf[i * 2 + 0] + y2_cf[i * 2 + 1] * y2_cf[i * 2 + 1]) / length);
        // Simple way to show two power spectrums as one plot
        sum_y[i] = fmax(y_cf[i], y2_cf[i]);
    }
    
    float y1max=0.0;
    int index1max=0;
    float y2max=0.0;
    int index2max=0;
    for(int i=0; i<length/2; i++){
        if(y1_cf[i]>y1max){
            y1max=y1_cf[i];
            index1max=i;
        }
        
        if(y2_cf[i]>y2max){
            y2max=y2_cf[i];
            index2max=i;
        }
        
    }
    printf("Y1 max => Index %d Magnitude %f\n", index1max, y1max);
    printf("Y2 max => Index %d Magnitude %f\n", index2max, y2max);
    float hz1=((float)index1max)*sampling_frequency/length;
    float hz2=((float)index2max)*sampling_frequency/length;
    printf("Max y1 frequency %f\n", hz1);
    printf("Max y2 frequency %f\n", hz2);

    return hz1;

}

void app_main(void)
{ 
    esp_err_t ret;
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK) {
        ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }

    //Compute FFT to extract maximum frequency
    
    uint32_t measurements[init_sample_size];
    struct BufferStructure buffer_struct;
    
    buffer_struct.wait_time = 1000/init_sample_size;
    buffer_struct.size_of_buffer = init_sample_size;
    buffer_struct.buffer = xStreamBufferCreate( size_t buffer_struct.size_of_buffer,
                                           size_t buffer_struct.size_of_buffer );

    xTaskCreatePinnedToCore(measure_task, "measure_task", 4096, &buffer_struct, 10, &myTaskHandle, 1);

    xStreamBufferReceive( buffer_struct.buffer,
                             measurements,
                             init_sample_size,
                             1000*60 );

    if( myTaskHandle != NULL )
    {
        vTaskDelete( myTaskHandle );
    }

    float max_frequency = compute_max_frequency(measurements, init_sample_size, init_sample_size);

    //COmpute parameters and variables for usual work
    float wait_time = max_frequency*2;
    buffer_struct.wait_time = wait_time;
    buffer_struct.size_of_buffer = 5000/wait_time;
    buffer_struct.buffer = xStreamBufferCreate( size_t buffer_struct.size_of_buffer,
                                           size_t buffer_struct.size_of_buffer );

    xTaskCreatePinnedToCore(measure_task, "measure_task", 4096, &buffer_struct, 10, &myTaskHandle, 1);

}


void measure_task(void* buff){
    struct BufferStructure buffer_to_aggregate = *(struct BufferStructure*)buff;
    uint32_t* voltage_buff[buffer_to_aggregate.size_of_buffer];
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11));

    while(1){
        for (int i = 0; i < buffer_to_aggregate.size_of_buffer; i++){
            voltage_buff[i] = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_0), &adc1_chars);
            
            vTaskDelay(pdMS_TO_TICKS(buffer_to_aggregate.wait_time));
        }
        xStreamBufferSend( buffer_to_aggregate.buffer,
                            voltage_buff,
                            buffer_to_aggregate.size_of_buffer,
                            0)
    }
}
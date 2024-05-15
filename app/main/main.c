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
TaskHandle_t regularTaskHandle = NULL;

static esp_adc_cal_characteristics_t adc1_chars;
struct BufferStructure {   // Structure declaration
    int wait_time;
    int size_of_buffer;           // Member (int variable)
    StreamBufferHandle_t buffer;       // Member (char variable)
    bool loop;
};

#define init_sample_size 1000
#define init_sample_size_2 2000

float y_cf[init_sample_size_2];
float *y1_cf = &y_cf[0];
float *y2_cf = &y_cf[init_sample_size];

float compute_max_frequency(uint32_t *data, int sampling_frequency)
{
    printf("Setting variables for FFT\n");

    for (int i = 0 ; i < init_sample_size ; i++) {
        y_cf[i * 2 + 0] = (float)data[i] ;
        y_cf[i * 2 + 1] = 0;
    }
    printf("Variables setted!\n");
    
    printf("Starting operations...\n");
    // FFT
    dsps_fft2r_fc32(y_cf, init_sample_size);
    // Bit reverse
    dsps_bit_rev_fc32(y_cf, init_sample_size);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(y_cf, init_sample_size);
    
    
    printf("Separating real from imaginary...\n");

    for (int i = 0 ; i < init_sample_size / 2 ; i++) {
        y_cf[i] = 10 * log10f((y_cf[i * 2 + 0] * y_cf[i * 2 + 0] + y_cf[i * 2 + 1] * y_cf[i * 2 + 1]) / init_sample_size);
        y2_cf[i] = 10 * log10f((y2_cf[i * 2 + 0] * y2_cf[i * 2 + 0] + y2_cf[i * 2 + 1] * y2_cf[i * 2 + 1]) / init_sample_size);
    }

    
    printf("Evaluating frequencies...\n");
    
    float y1max=0.0;
    int index1max=0;
    float y2max=0.0;
    int index2max=0;
    for(int i=1; i<init_sample_size/2; i++){
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
    float hz1=((float)index1max)*sampling_frequency/init_sample_size;
    float hz2=((float)index2max)*sampling_frequency/init_sample_size;
    printf("Max y1 frequency %f\n", hz1);
    printf("Max y2 frequency %f\n", hz2);

    printf("----------------------------------------------------------------------------------\n");

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    printf("Signal x1\n");
    dsps_view(y_cf, init_sample_size / 2, 128, 10,  0, 100, '|');
    printf("Signal x2\n");
    dsps_view(y2_cf, init_sample_size / 2, 128, 10,  0, 100, '|');

    return hz1;

}

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
            printf("Measurement %d, value: %"PRIu32"\n",i,voltage_buff[i]);
            vTaskDelay(buffer_to_aggregate.wait_time);
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


uint32_t measurements[init_sample_size];
struct BufferStructure buffer_struct;
struct BufferStructure buffer_struct2;

void regular_task(void){
    
    buffer_struct.wait_time = 1000/init_sample_size;
    buffer_struct.size_of_buffer = init_sample_size;
    buffer_struct.buffer = xStreamBufferCreate( buffer_struct.size_of_buffer*sizeof(uint32_t),
                                           buffer_struct.size_of_buffer*sizeof(uint32_t) );
    buffer_struct.loop = false;
    
    printf("Created buffer\n");
    
    printf("First measuring...\n");

    xTaskCreatePinnedToCore(measure_task, "measure_task", sizeof(uint32_t)*init_sample_size + 4096, &buffer_struct, 10, &myTaskHandle, 1);

    int n_b = xStreamBufferReceive( buffer_struct.buffer,
                             measurements,
                             sizeof(uint32_t)*init_sample_size,
                             1000*60 );

    printf("Data needed: %d, Data received: %d\n", sizeof(uint32_t)*(int)init_sample_size, n_b);

    vStreamBufferDelete(buffer_struct.buffer);
    
    printf("Done!\n");
    
    printf("Doing FFT...\n");

    float max_frequency = compute_max_frequency(measurements, init_sample_size);
    
    printf("Done!\n");
    
    printf("Creating new buffer...\n");

    //COmpute parameters and variables for usual work
    if(max_frequency == 0){
        printf("Max frequency not correct!\n");
        vTaskDelete(NULL);
    }
    float wait_time = max_frequency*2;
    buffer_struct2.wait_time = wait_time;
    buffer_struct2.size_of_buffer = 5000/wait_time;
    buffer_struct2.buffer = xStreamBufferCreate( buffer_struct.size_of_buffer*sizeof(uint32_t),
                                           buffer_struct.size_of_buffer*sizeof(uint32_t) );
    buffer_struct2.loop = true;
    
    uint32_t measurements_final[buffer_struct2.size_of_buffer];
    
    printf("--- Looping ---\n");
    xTaskCreatePinnedToCore(measure_task, "measure_task", sizeof(uint32_t)*init_sample_size + 4096, &buffer_struct2, 10, &myTaskHandle, 1);
    while(1){
        
        printf("Waiting for data...\n");
        xStreamBufferReceive( buffer_struct2.buffer,
                             measurements_final,
                             buffer_struct2.size_of_buffer*sizeof(uint32_t),
                              portMAX_DELAY );
        
        printf("Data received!\n");

        uint32_t sum = 0;
        for (int j = 0; j < buffer_struct.size_of_buffer; j++){
            sum += measurements_final[j];
        }

        printf("Average is: %"PRIu32"\n", sum);
    }
}

void app_main(void)
{ 
    esp_err_t ret;
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK) {
        printf("Not possible to initialize FFT. Error = %i", ret);
        return;
    }
    printf("FTT ready to work\n");

    xTaskCreate(regular_task, "regular_task", 4096, NULL, 10, &regularTaskHandle);
    
}


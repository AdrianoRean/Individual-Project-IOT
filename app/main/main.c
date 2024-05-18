
#include "main.h"

#include <freertos/task.h>

#include "measure.c"
#include "fft.c"

TaskHandle_t myTaskHandle = NULL;
TaskHandle_t regularTaskHandle = NULL;

uint32_t measurements[init_sample_size];
struct BufferStructure buffer_struct;
struct BufferStructure buffer_struct2;

void regular_task(void){
    
    buffer_struct.wait_time = ceil(1000/init_sample_size);
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

    float max_frequency = compute_max_frequency(measurements, 1000);
    
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


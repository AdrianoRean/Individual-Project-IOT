#include <stdio.h>
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

#include "esp_dsp.h"
#include <math.h>

#include "my_signal.c"

static const char *TAG = "main";

#define WINDOWS_SIZE 512
#define SAMPLING_FREQUENCY 1024
int N = WINDOWS_SIZE;
float sum_y[WINDOWS_SIZE / 2];
float y_cf[WINDOWS_SIZE * 2];
// Pointers to result arrays
float *y1_cf = &y_cf[0];
float *y2_cf = &y_cf[WINDOWS_SIZE];

void printFrequency(){
    float y1max=0.0;
    int index1max=0;
    float y2max=0.0;
    int index2max=0;
    for(int i=0; i<N/2; i++){
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
    float hz1=((float)index1max)*SAMPLING_FREQUENCY/N;
    float hz2=((float)index2max)*SAMPLING_FREQUENCY/N;
    printf("Max y1 frequency %f\n", hz1);
    printf("Max y2 frequency %f\n", hz2);
}

void process_and_show(float *data, int length)
{
    
    //float wind[N];
    //dsps_wind_hann_f32(wind, N);

    for (int i = 0 ; i < N ; i++) {
        y_cf[i * 2 + 0] = data[i] ;//* wind[i];
        y_cf[i * 2 + 1] = 0;
    }
    // FFT
    unsigned int start_b = dsp_get_cpu_cycle_count();
    dsps_fft2r_fc32(y_cf, N);
    unsigned int end_b = dsp_get_cpu_cycle_count();
    // Bit reverse
    dsps_bit_rev_fc32(y_cf, N);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(y_cf, N);
    

    for (int i = 0 ; i < N / 2 ; i++) {
        y_cf[i] = 10 * log10f((y_cf[i * 2 + 0] * y_cf[i * 2 + 0] + y_cf[i * 2 + 1] * y_cf[i * 2 + 1]) / N);
        y2_cf[i] = 10 * log10f((y2_cf[i * 2 + 0] * y2_cf[i * 2 + 0] + y2_cf[i * 2 + 1] * y2_cf[i * 2 + 1]) / N);
        // Simple way to show two power spectrums as one plot
        sum_y[i] = fmax(y_cf[i], y2_cf[i]);
    }
    

    printFrequency();

/*
    printf("%f\n", y2_cf[49]);
    printf("%f\n", y2_cf[50]);
    printf("%f\n", y2_cf[51]);

    printf("********************\n");
    
    printf("%f\n", y2_cf[100]);
    printf("%f\n", y2_cf[101]);
    printf("%f\n", y2_cf[102]);
*/
    printf("----------------------------------------------------------------------------------\n");

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    ESP_LOGW(TAG, "Signal x1");
    dsps_view(y_cf, N / 2, 128, 10,  -60, 40, '|');
    ESP_LOGW(TAG, "Signal x2");
    dsps_view(y2_cf, N / 2, 128, 10,  -60, 40, '|');
    ESP_LOGW(TAG, "Signals x1 and x2 on one plot");
    dsps_view(sum_y, N / 2, 128, 10,  -60, 40, '|');
    ESP_LOGI(TAG, "FFT for %i complex points take %i cycles", N, end_b - start_b);

    ESP_LOGI(TAG, "End Example.");

}

void app_main(void)
{ 
    ESP_LOGI(TAG, "Start Example.");
    esp_err_t ret;
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK) {
        ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }


    /*
    int i = 1000;
    for(i = 0; i < 1000; i++){
        printf("%f", signal[i]);
    }
    */
   process_and_show(signal, 1000);

}
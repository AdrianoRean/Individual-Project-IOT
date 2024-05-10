#include <stdio.h>
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

#include "esp_dsp.h"
#include <math.h>

#include "my_signal.c"

static const char *TAG = "main";

#define N_SAMPLES 2000
int N = N_SAMPLES;
float sum_y[N_SAMPLES / 2];
float y_cf[N_SAMPLES * 2];
// Pointers to result arrays
float *y1_cf = &y_cf[0];
float *y2_cf = &y_cf[N_SAMPLES];
void process_and_show(float *data, int length)
{
    for (int i = 0 ; i < N ; i++) {
        y_cf[i * 2 + 0] = data[i];
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
        y_cf[i] = 10 * log10f((y2_cf[i * 2 + 0] * y2_cf[i * 2 + 0] + y2_cf[i * 2 + 1] * y2_cf[i * 2 + 1]) / N);
        // Simple way to show two power spectrums as one plot
        sum_y[i] = fmax(y_cf[i], y2_cf[i]);
    }

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    ESP_LOGW(TAG, "Signal x1");
    dsps_view(data, N / 2, 64, 10,  -60, 40, '|');
    ESP_LOGW(TAG, "Signal x2");
    dsps_view(y2_cf, N / 2, 64, 10,  -60, 40, '|');
    ESP_LOGW(TAG, "Signals x1 and x2 on one plot");
    dsps_view(sum_y, N / 2, 64, 10,  -60, 40, '|');
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
#include <stdio.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "esp_dsp.h"

static const char *TAG = "main";

#define N_SAMPLES 1024
int N = N_SAMPLES;
float wind[N_SAMPLES];
float y_cf[N_SAMPLES * 2];
float *y1_cf = &y_cf[0];

void process_and_show(float *data, int length)
{
    
    dsps_wind_hann_f32(wind, N);
    
    dsps_fft2r_fc32(data, length);
    // Bit reverse
    dsps_bit_rev_fc32(data, length);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(data, length);

    for (int i = 0 ; i < length / 2 ; i++) {
        data[i] = 10 * log10f((data[i * 2 + 0] * data[i * 2 + 0] + data[i * 2 + 1] * data[i * 2 + 1]) / N);
    }

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    dsps_view(data, length / 2, 64, 10,  -120, 40, '|');

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

    //process_and_show(data, N)

}
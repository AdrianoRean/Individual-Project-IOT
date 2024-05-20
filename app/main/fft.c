#include "main.h"
#include "esp_dsp.h"
#include <math.h>

const char *FFT_TAG = "FFT";

float y_cf[init_sample_size_2];
float *y1_cf = &y_cf[0];
float *y2_cf = &y_cf[init_sample_size];

float compute_max_frequency(uint32_t *data, int sampling_frequency)
{
    ESP_LOGI(FFT_TAG, "Setting variables for FFT");

    float wind[init_sample_size];
    dsps_wind_hann_f32(wind, init_sample_size);

    for (int i = 0 ; i < init_sample_size ; i++) {
        y_cf[i * 2 + 0] = (float)data[i] * wind[i];
        y_cf[i * 2 + 1] = 0;
    }
    ESP_LOGI(FFT_TAG, "Variables setted!");
    
    ESP_LOGI(FFT_TAG, "Starting operations...");
    // FFT
    dsps_fft2r_fc32(y_cf, init_sample_size);
    // Bit reverse
    dsps_bit_rev_fc32(y_cf, init_sample_size);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(y_cf, init_sample_size);

    ESP_LOGI(FFT_TAG, "Separating real from imaginary and z score...");

    int sum = 0;

    for (int i = 0 ; i < init_sample_size/2 ; i++) {
        y1_cf[i] = 10*log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1])/init_sample_size);
        sum += y1_cf[i];
    }


    float average = sum/(init_sample_size/2);
    float differences = 0.0;
    float temp = 0.0;

    ESP_LOGI(FFT_TAG, "Sum: %d", sum);
    ESP_LOGI(FFT_TAG, "Average: %f", average);

    for (int i = 0 ; i < (init_sample_size/2) ; i++) {
        temp = y1_cf[i] - average;
        differences += temp*temp;
    }

    float st_deviation = sqrt(differences/((init_sample_size/2)-1));
    
    ESP_LOGI(FFT_TAG, "STD: %f", st_deviation);

    ESP_LOGI(FFT_TAG, "Finding outliers...");    

    float z = 0;
    float maxM = -1;
    int maxI = -1;

    for (int i = 0 ; i < (init_sample_size/2) ; i++) {
        z = (y1_cf[i] - average)/st_deviation;
        if (z > 4){
            ESP_LOGI(FFT_TAG, "frequency is: %d", maxI*sampling_frequency/init_sample_size);
            maxM = y1_cf[i];
            maxI = i;
        }
    }
    
    ESP_LOGI(FFT_TAG, "Y1 max => Index %d Magnitude %f", maxI, maxM);
    float hz1=(float)(maxI)*(float)sampling_frequency/init_sample_size;
    ESP_LOGI(FFT_TAG, "Max y1 frequency %f", hz1);

    ESP_LOGI(FFT_TAG, "------------------------------------- Plot ---------------------------------------------");

    /*
    for (int i = 0 ; i < init_sample_size/2 ; i++) {
        y1_cf[i] = 10*log10f(y1_cf[i]/init_sample_size);
    }
    */

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    ESP_LOGI(FFT_TAG, "Signal x1");
    dsps_view(y1_cf, init_sample_size / 2, 128, 10,  0, 100, '|');

    return hz1;

}
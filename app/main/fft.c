#include "main.h"
#include "esp_dsp.h"
#include <math.h>

float y_cf[init_sample_size_2];
float *y1_cf = &y_cf[0];
float *y2_cf = &y_cf[init_sample_size];

float compute_max_frequency(uint32_t *data, int sampling_frequency)
{
    printf("Setting variables for FFT\n");

    float wind[init_sample_size];
    dsps_wind_hann_f32(wind, init_sample_size);

    for (int i = 0 ; i < init_sample_size ; i++) {
        y_cf[i * 2 + 0] = (float)data[i] * wind[i];
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

    printf("Separating real from imaginary and z score...\n");

    int sum = 0;

    for (int i = 0 ; i < init_sample_size/2 ; i++) {
        y1_cf[i] = 10*log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1])/init_sample_size);
        sum += y1_cf[i];
    }


    float average = sum/(init_sample_size/2);
    float differences = 0.0;
    float temp = 0.0;

    printf("Sum: %d\n", sum);
    printf("Average: %f\n", average);

    for (int i = 0 ; i < (init_sample_size/2) ; i++) {
        temp = y1_cf[i] - average;
        differences += temp*temp;
    }

    float st_deviation = sqrt(differences/((init_sample_size/2)-1));
    
    printf("STD: %f\n", st_deviation);

    printf("Finding outliers...\n");    

    float z = 0;
    float maxM = -1;
    int maxI = -1;

    for (int i = 0 ; i < (init_sample_size/2) ; i++) {
        z = (y1_cf[i] - average)/st_deviation;
        if (z > 4){
            printf("frequency is: %d\n", maxI*sampling_frequency/init_sample_size);
            maxM = y1_cf[i];
            maxI = i;
        }
    }
    
    printf("Y1 max => Index %d Magnitude %f\n", maxI, maxM);
    float hz1=(float)(maxI)*(float)sampling_frequency/init_sample_size;
    printf("Max y1 frequency %f\n", hz1);

    printf("------------------------------------- Plot ---------------------------------------------\n");

    /*
    for (int i = 0 ; i < init_sample_size/2 ; i++) {
        y1_cf[i] = 10*log10f(y1_cf[i]/init_sample_size);
    }
    */

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    printf("Signal x1\n");
    dsps_view(y1_cf, init_sample_size / 2, 128, 10,  0, 100, '|');

    return hz1;

}
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

static esp_adc_cal_characteristics_t adc1_chars;

void read_sensor(){
    uint32_t voltage;

   esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);

   ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
   ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11));

   while (1)
   {
       voltage = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_0), &adc1_chars);
       printf("ADC1_CHANNEL_0: %" PRIu32 " mV\n", voltage);
       vTaskDelay(pdMS_TO_TICKS(100));
   }
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

    read_sensor();

    //process_and_show(data, N)

}
#ifndef main
#define main

#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include "freertos/stream_buffer.h"
#include "esp_log.h"
#include "nvs_flash.h" //non volatile storage
#include "esp_netif_sntp.h"

#include <inttypes.h>

// Ho deciso 1 millisecondo perchè meno di un millisecondo vuol dire utilizzare più tempo a processare ogni interrupt. Quindi prendo fino a 1kz
// Il massimo che posso samplare senza finire la memoria contando le varie operazioni è una finestra di 2048 valori, ovvero circa 2 secondi. Posso prendere quindi frequenze fino a 1 hz comodamente.

#define init_sample_size 2048
#define init_sample_size_2 4096

struct BufferStructure {   // Structure declaration
    int wait_time;
    int size_of_buffer;           // Member (int variable)
    StreamBufferHandle_t buffer;       // Member (char variable)
    bool loop;
};

#endif
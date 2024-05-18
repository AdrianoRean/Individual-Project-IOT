#ifndef main
#define main

#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include "freertos/stream_buffer.h"

#include <inttypes.h>

#define init_sample_size 1024
#define init_sample_size_2 2048

struct BufferStructure {   // Structure declaration
    int wait_time;
    int size_of_buffer;           // Member (int variable)
    StreamBufferHandle_t buffer;       // Member (char variable)
    bool loop;
};

#endif
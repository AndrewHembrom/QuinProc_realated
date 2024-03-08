#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include <math.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "WiFiHelper.h"
#include "dspHelper.h"

i2s_chan_handle_t rx_handle_1;
QueueHandle_t buffQueue_1;

i2s_chan_handle_t rx_handle_2;
QueueHandle_t buffQueue_2;

int Largestflag = 0;
float largest = 0;

#define _SAMPLE_RATE 8000

static void readTask_1 (void *args) {
    int32_t raw_buf[N_SAMPLES];
    int16_t output_buf[N_SAMPLES];
    size_t r_bytes = 0;
    int samples_read = 0;

    buffQueue_1 = xQueueCreate(2, sizeof(output_buf));

    while(1) {
        if(i2s_channel_read(rx_handle_1, raw_buf, sizeof(int32_t) * N_SAMPLES, &r_bytes, portMAX_DELAY) == ESP_OK) {
            samples_read = r_bytes/sizeof(int32_t);
            int sample_index = 0;
            for (int i = 0; i < samples_read; i++) {
                output_buf[sample_index++] = (raw_buf[i] & 0xFFFFFFF0) >> 11;
            }
            xQueueSend(buffQueue_1, (void*)output_buf, (TickType_t) 0);
        }
    }
}

static void printTask_1 (void *args) {
    int16_t rxBuffer[N_SAMPLES];
    float rBuff[N_SAMPLES/2];
    float lBuff[N_SAMPLES/2];
    
    while(1) {
        if(buffQueue_1 != 0) {
            if(xQueueReceive(buffQueue_1, &(rxBuffer), (TickType_t)10)){

                float* outputBuffer = (float*) calloc(N_SAMPLES, sizeof(float));

                buffSplit(rxBuffer, lBuff, rBuff);
                HPF(lBuff, rBuff, 50.0/_SAMPLE_RATE, 10);
                LPF(lBuff, rBuff, 400.0/_SAMPLE_RATE, 10);
                
                int lcount = 0;
                int rcount = 0;
                for(int i = 0; i < N_SAMPLES; i++) {
                    if(i < N_SAMPLES/2) {
                        outputBuffer[i] = lBuff[lcount++];
                    } else {
                        outputBuffer[i] = rBuff[rcount++];
                    }

                    if(outputBuffer[i] > largest && (Largestflag < 4096)) {
                        largest = outputBuffer[i];
                        Largestflag++;
                    }
                }

                for(int i = 0; i < N_SAMPLES; i++) {
                    outputBuffer[i] /= largest;
                    // printf("%f, ",outputBuffer[i]);
                }

                int err = sendto(sock, outputBuffer, (N_SAMPLES) * sizeof(float), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if (err < 0) {
                    ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
                }

                ESP_LOGI("TransmitTask", "Here");
                free(outputBuffer);

                // vTaskDelay(10/portTICK_PERIOD_MS);

            }
        }
    }
}

void app_main(void) {

    initNVS();

    WiFiConnectHelper();

    initUDP();

    //First Pair
    i2s_chan_config_t rx_chan_cfg_1 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&rx_chan_cfg_1, NULL, &rx_handle_1);

    i2s_std_config_t rx_std_cfg_1 = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_SAMPLE_RATE),
        // {
        //     .sample_rate_hz = _SAMPLE_RATE,
        //     .mclk_multiple = 256,
        // },
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            // .mclk = I2S_GPIO_UNUSED, .bclk = (gpio_num_t)25, .ws = (gpio_num_t)26, .dout = I2S_GPIO_UNUSED, .din = (gpio_num_t)32,

            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_25,
            .ws = GPIO_NUM_33,
            .dout = I2S_GPIO_UNUSED,
            .din = GPIO_NUM_34,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },

            // .mclk = I2S_GPIO_UNUSED,
            // .bclk = (gpio_num_t) 25,
            // .ws = (gpio_num_t) 26,
            // .dout = I2S_GPIO_UNUSED,
            // .din = (gpio_num_t) 32,
            // .invert_flags = {
            //     .mclk_inv = false,
            //     .bclk_inv = false,
            //     .ws_inv = false,
            // },
        },
    };
    i2s_channel_init_std_mode(rx_handle_1, &rx_std_cfg_1);
    i2s_channel_enable(rx_handle_1);

    //Second Pair
    // i2s_chan_config_t rx_chan_cfg_2 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    // i2s_new_channel(&rx_chan_cfg_2, NULL, &rx_handle_2);

    // i2s_std_config_t rx_std_cfg_2 = {
    //     .clk_cfg = {
    //         .sample_rate_hz = _SAMPLE_RATE,
    //         .mclk_multiple = 256,
    //     },
    //     .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    //     .gpio_cfg = {
    //         .mclk = I2S_GPIO_UNUSED,
    //         .bclk = (gpio_num_t) 25,
    //         .ws = (gpio_num_t) 33,
    //         .dout = I2S_GPIO_UNUSED,
    //         .din = (gpio_num_t) 34,
    //         .invert_flags = {
    //             .mclk_inv = false,
    //             .bclk_inv = false,
    //             .ws_inv = false,
    //         },
    //     },
    // };
    // i2s_channel_init_std_mode(rx_handle_2, &rx_std_cfg_2);
    // i2s_channel_enable(rx_handle_2);

    xTaskCreate(readTask_1, "readTask_1", 65536, NULL, 5, NULL);
    xTaskCreate(printTask_1, "printTask_1", 65536, NULL, 5, NULL);
}

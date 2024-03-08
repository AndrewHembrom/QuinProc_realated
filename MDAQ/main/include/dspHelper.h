#ifndef DSPHELPER_H
#define DSPHELPER_H

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_dsp.h"

#define N_SAMPLES 4096
#define HOST_IP_ADDR "192.168.63.252"
#define PORT 3333
struct sockaddr_in dest_addr;
static const char *TAG_UDP = "UDP";
int sock = 0;

void initUDP() {
    int addr_family = 0;
    int ip_protocol = 0;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG_UDP, "Unable to create socket: errno %d", errno);
    }

    // Set timeout
    // struct timeval timeout;
    // timeout.tv_sec = 10;
    // timeout.tv_usec = 0;
    // setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    ESP_LOGI(TAG_UDP, "Socket created");

    // float *count_1 = (float*) calloc(1024, sizeof(float));;

    // for(int i = 0; i < 1024; i++) {
    //     count_1[i] = 3.14;
    // }

    // while(1) {
    //     int err = sendto(sock, count_1, 4096, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    //     if (err < 0) {
    //         ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
    //         break;
    //     }
        
    //     vTaskDelay(512 / portTICK_PERIOD_MS);
    // }
}


void LPF(float *leftInBuffer, float *rightInBuffer, float freq, float qFactor) {

    esp_err_t ret = ESP_OK;
    float coeffs_lpf[5];
    float w_lpf[5] = {0,0};

    float leftOutBuffer[N_SAMPLES/2];
    float rightOutBuffer[N_SAMPLES/2];

    // Calculate iir filter coefficients
    ret = dsps_biquad_gen_lpf_f32(coeffs_lpf, freq, qFactor);
    if (ret  != ESP_OK) {
        ESP_LOGE("LPF", "Operation error = %i", ret);
        return;
    }

    // Process input signals
    //Left Channel
    ret = dsps_biquad_f32_ae32(leftInBuffer, leftOutBuffer, N_SAMPLES/2, coeffs_lpf, w_lpf);
    if (ret  != ESP_OK){
        ESP_LOGE("LPF", "Operation error = %i", ret);
        return;
    }

    //Right Channel
    ret = dsps_biquad_f32_ae32(rightInBuffer, rightOutBuffer, N_SAMPLES/2, coeffs_lpf, w_lpf);
    if (ret  != ESP_OK){
        ESP_LOGE("LPF", "Operation error = %i", ret);
        return;
    }

    memcpy(leftInBuffer, leftOutBuffer, (N_SAMPLES/2) * sizeof(float));
    memcpy(rightInBuffer, rightOutBuffer, (N_SAMPLES/2) * sizeof(float));
    
}

void HPF(float *leftInBuffer, float *rightInBuffer, float freq, float qFactor) {

    esp_err_t ret = ESP_OK;
    float coeffs_hpf[5];
    float w_hpf[5] = {0,0};

    float leftOutBuffer[N_SAMPLES/2];
    float rightOutBuffer[N_SAMPLES/2];

    // Calculate iir filter coefficients
    ret = dsps_biquad_gen_hpf_f32(coeffs_hpf, freq, qFactor);
    if (ret  != ESP_OK) {
        ESP_LOGE("HPF", "Operation error = %i", ret);
        return;
    }

    // Process input signals
    //Left Channel
    ret = dsps_biquad_f32_ae32(leftInBuffer, leftOutBuffer, N_SAMPLES/2, coeffs_hpf, w_hpf);
    if (ret  != ESP_OK){
        ESP_LOGE("HPF", "Operation error = %i", ret);
        return;
    }
    //Right Channel
    ret = dsps_biquad_f32_ae32(rightInBuffer, rightOutBuffer, N_SAMPLES/2, coeffs_hpf, w_hpf);
    if (ret  != ESP_OK){
        ESP_LOGE("HPF", "Operation error = %i", ret);
        return;
    }

    memcpy(leftInBuffer, leftOutBuffer, (N_SAMPLES/2) * sizeof(float));
    memcpy(rightInBuffer, rightOutBuffer, (N_SAMPLES/2) * sizeof(float));
    
}

void buffSplit(int16_t* buff, float* lBuffer, float* rBuffer) {

    int k = 0;
    for(int i = 0; i < N_SAMPLES; i += 2) {
        lBuffer[k] = (buff[i]);
        rBuffer[k++] = (buff[i + 1]);
    }

}

// void transmitBuff(float* buff, size_t samples) {
//     float *outputBuffer = (float*) calloc(samples, sizeof(float));

//     memcpy(outputBuffer, buff, (samples) * sizeof(float));

//     int err = sendto(sock, outputBuffer, (samples) * sizeof(float), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
//     if (err < 0) {
//         ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
//     }

//     free(outputBuffer);

//     vTaskDelay(10/portTICK_PERIOD_MS);
// }

#endif
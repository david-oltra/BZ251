/**
 * BZ251 used only with Galeleo
 * -------------------------------------
 * This code is an example implementation for the 
 * BZ251 module,designed specifically for use with 
 * the Galileo system.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/semphr.h>
#include <iostream>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <sstream>
#include <esp_timer.h>
#include <bz251.h>
#include <bz251_defs.h>

extern "C" void app_main();

static TaskHandle_t core_A = NULL;
static TaskHandle_t bz251_task = NULL;
SemaphoreHandle_t bz251_mutex;

Bz251 bz251;
Bz251Data bz251Data;

// PINOUT UART
#define UART_NUM UART_NUM_2
#define GPIO_UART_TX GPIO_NUM_17
#define GPIO_UART_RX GPIO_NUM_16

#if UART_NUM != UART_NUM_2
    #error "Dont use UART_NUM_0 Change for UART_NUM_2"
#endif

using namespace std;

static char TAG[] = "BZ251";

esp_err_t uart_init(void)
{
    uart_config_t uart_config;
        uart_config.baud_rate = 38400;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.source_clk = UART_SCLK_APB;

    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM, 1024 * 2, 2048, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, GPIO_UART_TX, GPIO_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    return ESP_OK;
}

esp_err_t bz251_init(void)
{
    Bz251Config conf;
        conf.uartNum = UART_NUM_2;
        conf.timeZone = 1;  // Timezone UTC +1
        conf.hasGps = 0;    // Disable GPS
        conf.dynmodel = 6;  // Airborne with <1g acceleration

    bz251.init(conf);

    return ESP_OK;
}

void bz251Task(void *arg)
{
    for(;;)
    {
        if(xSemaphoreTake(bz251_mutex,pdMS_TO_TICKS(100)))
        {
            bz251.read();
            xSemaphoreGive(bz251_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void coreAThread(void *arg)
{
    ESP_LOGW(TAG, "Starting CORE A");  

    for(;;)
    {
        if(xSemaphoreTake(bz251_mutex,pdMS_TO_TICKS(100)))
        {
            bz251.getData(bz251Data);
           
            ESP_LOGI(TAG, "\nlatitude\t%.8f\nlongitude\t%.8f\naltitude\t%.2f\nspeed\t%.8f\nsats\t%u\ndate\t%u/%u/%u\ntime\t%u:%u",
                            bz251Data.latitude,
                            bz251Data.longitude,
                            bz251Data.altitude,
                            bz251Data.speedKmh,
                            bz251Data.satellites,
                            bz251Data.day,bz251Data.month, bz251Data.year,
                            bz251Data.hour, bz251Data.minute
                        );
            
            xSemaphoreGive(bz251_mutex);
        }
    
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void app_main(void)
{
    ESP_LOGW(TAG, "Starting software");

    if (uart_init() == ESP_OK) {ESP_LOGI(TAG,"Config UART completed");}
    else {ESP_LOGE(TAG,"Config UART failed");}
    
    if (bz251_init() == ESP_OK) {ESP_LOGI(TAG,"Config bz251 completed");}
    else {ESP_LOGE(TAG,"Config bz251 failed");}

    bz251_mutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(bz251Task, "bz251_task", 4096, NULL, 1, &bz251_task, 1); 
    xTaskCreatePinnedToCore(coreAThread, "core_A", 4096, NULL, 3, &core_A, 0);
}
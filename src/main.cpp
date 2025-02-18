/**
 * BZ251 used only with Galeleo
 * -------------------------------------
 * This code is an example implementation for the 
 * BZ251 module,designed specifically for use with 
 * the Galileo system.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
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
static TaskHandle_t core_B = NULL;

Bz251 bz251;

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
    bz251.configSetValue(CFG_RATE_TIMEREF, 0x04); // Align measurements to Galileo time
    vTaskDelay(pdMS_TO_TICKS(500));
    bz251.configSetValue(CFG_NAVSPG_UTCSTANDARD, 0x05); // UTC as combined from multiple European laboratories; derived from Galileo time
    vTaskDelay(pdMS_TO_TICKS(500));
    bz251.configSetValue(CFG_NAVSPG_DYNMODEL,0x06); // Airborne with <1g acceleration
    vTaskDelay(pdMS_TO_TICKS(500));
    bz251.configSetValue(CFG_SIGNAL_SBAS_ENA,0); // Disable SBAS for GPS disable
    vTaskDelay(pdMS_TO_TICKS(500));
    bz251.configSetValue(CFG_SIGNAL_QZSS_ENA,0); // Disable QZSS for GPS disable
    vTaskDelay(pdMS_TO_TICKS(500));
    bz251.configSetValue(CFG_SIGNAL_GPS_ENA,0); // GPS disable
    vTaskDelay(pdMS_TO_TICKS(500));
    bz251.configSetValue(CFG_SIGNAL_BDS_ENA,0); // BeiDou disable
    vTaskDelay(pdMS_TO_TICKS(500));


    // bz251.configSetValue(CFG_MSGOUT_NMEA_ID_GGA_UART1, 0);
    bz251.configSetValue(CFG_MSGOUT_NMEA_ID_GSA_UART1, 0);
    bz251.configSetValue(CFG_MSGOUT_NMEA_ID_VTG_UART1, 0);
    bz251.configSetValue(CFG_MSGOUT_NMEA_ID_GSV_UART1, 0);
    bz251.configSetValue(CFG_MSGOUT_NMEA_ID_GLL_UART1, 0);
    // bz251.configSetValue(CFG_MSGOUT_NMEA_ID_RMC_UART1, 0);

    return ESP_OK;
}


void coreAThread(void *arg)
{
    ESP_LOGW(TAG, "Starting CORE A");  

    for(;;)
    {
        bz251.getPosition(bz251.data.latitude, bz251.data.longitude);
        bz251.getAltitude(bz251.data.altitude);
        bz251.getDate(bz251.data.day, bz251.data.month, bz251.data.year);
        bz251.getTime(bz251.data.hour, bz251.data.minute);

        ESP_LOGI(TAG, "\nlatitude\t%.8f\nlongitude\t%.8f\naltirude\t%.8f\ndate\t%u/%u/%u\ntime\t%u:%u",
                        bz251.data.latitude,
                        bz251.data.longitude,
                        bz251.data.altitude,
                        bz251.data.day,bz251.data.month, bz251.data.year,
                        bz251.data.hour, bz251.data.minute);
    
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

void coreBThread(void *arg)
{
    ESP_LOGW(TAG, "Starting CORE B");

    uint8_t* data = (uint8_t*) malloc(1024+1);

    for(;;){
        // bz251_get_value(CFG_SIGNAL_GPS_ENA);
        const int rxBytes = uart_read_bytes(UART_NUM, data, 1024,pdMS_TO_TICKS(100));
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(TAG, "Read %d bytes: \n%s", rxBytes, data);
            // ESP_LOG_BUFFER_HEX(TAG, data, rxBytes);
        }
        if (data[0] == 0xB5 && data[2] == 0x06){
            ESP_LOGE(TAG, "%x %x %x %x",data[0], data[1], data[2], data[3]);
            ESP_LOG_BUFFER_HEX(TAG, data, data[4]+6);
        }
    
        vTaskDelay(pdMS_TO_TICKS(1000));
    } 
}


void app_main(void)
{
    ESP_LOGW(TAG, "Starting software");

    if (uart_init() == ESP_OK) {ESP_LOGI(TAG,"Config UART completed");}
    else {ESP_LOGE(TAG,"Config UART failed");}
    
    if (bz251_init() == ESP_OK) {ESP_LOGI(TAG,"Config UART completed");}
    else {ESP_LOGE(TAG,"Config UART failed");}


    xTaskCreatePinnedToCore(coreBThread, "core_B", 4096, NULL, 2, &core_B, 1);    
    xTaskCreatePinnedToCore(coreAThread, "core_A", 4096, NULL, 1, &core_A, 0);
}
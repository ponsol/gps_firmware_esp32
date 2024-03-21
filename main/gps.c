
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <string.h>

#include <nmea.h>
#include <gpgll.h>
/*
#include <nmea/gpgga.h>
#include <nmea/gprmc.h>
#include <nmea/gpgsa.h>
#include <nmea/gpvtg.h>
#include <nmea/gptxt.h>
#include <nmea/gpgsv.h>
*/




#define TXD    7
#define RXD    6
#define RTS    UART_PIN_NO_CHANGE
#define CTS    UART_PIN_NO_CHANGE

#define GPOWER   19

#define UART_PORT       1 
#define UART_BAUD_RATE  9600
//#define UART_BAUD_RATE  57600
//#define UART_BAUD_RATE  38400
#define TASK_STACK      2048  

#define BUF_SIZE (1024)

static const char *TAG = "uart";


#define TIME_ZONE (-6)  
#define YEAR_BASE (2000) 

void myparse( char *mesg) {

    nmea_s *nmd;

    nmd = nmea_parse(mesg, strlen( mesg), 0);
    if (NMEA_GPGLL == nmd->type) {
      	nmea_gpgll_s *gpgll = (nmea_gpgll_s *) nmd;

      	printf("GPGLL Sentence\n");
      	printf("Longitude:\n");
      	printf("  Degrees: %d\n", gpgll->longitude.degrees);
      	printf("  Minutes: %f\n", gpgll->longitude.minutes);
      	printf("  Cardinal: %c\n", (char) gpgll->longitude.cardinal);
      	printf("Latitude:\n");
      	printf("  Degrees: %d\n", gpgll->latitude.degrees);
      	printf("  Minutes: %f\n", gpgll->latitude.minutes);
      	printf("  Cardinal: %c\n", (char) gpgll->latitude.cardinal);
    }

}

static void uart_task(void *arg)
{


    gpio_set_level(GPOWER , 0);

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << GPOWER,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;
    //intr_alloc_flags = ESP_INTR_FLAG_IRAM;

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, TXD, RXD, RTS, CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    gpio_set_level(GPOWER , 1);
    while (1) {

        int len = uart_read_bytes(UART_PORT, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len) {

            data[len] = '\0';
            ESP_LOGI(TAG, "Recv str: %s", (char *) data);
            myparse( (char *) data );
        }
    }

}

void uart_main(void)
{
    xTaskCreate(uart_task, "uart_task", TASK_STACK, NULL, 10, NULL);
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define BOARD_LED 2 //blue led
#define BOARD_BTN 0 //BOOT btn, EN - reset btn
#define GPIO_OUTPUT_PINS (1ULL << BOARD_LED)
#define GPIO_INPUT_PINS (1ULL << BOARD_BTN)

TaskHandle_t htask_1 = NULL;
TaskHandle_t htask_2 = NULL;

QueueHandle_t qlog_1 = NULL;
typedef struct
{
    char MessageID;
    char Data[ 50 ];
} LogMessage;

void send_logTask(LogMessage *_msg){
    xQueueSendToBack(qlog_1, _msg, (TickType_t)0);
}

void logger_task(void *pvParameters){

    LogMessage buff;
    
    qlog_1 = xQueueCreate(5, sizeof(LogMessage));

    if( qlog_1 == 0 ){
        ESP_LOGE("LOGGER", "ERROR: Fail create logger task!");
        vTaskSuspend(NULL);
    }

    for(;;){

        if(xQueueReceive(qlog_1, &buff, (TickType_t)5)){
            printf("LOGGER[%d]: %s\n", buff.MessageID, buff.Data);
        }

        vTaskDelay(10);
    }

}

void task1(void *pvParameters){

    LogMessage myMsg = {0, "Task create"};
    send_logTask(&myMsg);

    for(;;){
        sprintf(myMsg.Data, "Task suspend");
        send_logTask(&myMsg);
        vTaskSuspend(NULL);
    }

    vTaskSuspend(NULL);
}

void task2(void *pvParameters){
    LogMessage myMsg = {1, "Task create"};
    send_logTask(&myMsg);

    uint8_t counter = 0;

    for(;;){
        sprintf(myMsg.Data, "Counter: %d", counter);
        send_logTask(&myMsg);

        counter++;

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskSuspend(NULL);
}

void app_main(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PINS;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    //io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PINS;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    xTaskCreatePinnedToCore(logger_task, "LOGGER", 4096, NULL, 10, NULL, 0);

    xTaskCreatePinnedToCore(task1, "Task1", 4096, NULL, 10, &htask_1, 0);
    xTaskCreatePinnedToCore(task2, "Task2", 4096, NULL, 10, &htask_2, 1);

    int cnt = 0;
    while (1) {
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskSuspend(NULL);

}

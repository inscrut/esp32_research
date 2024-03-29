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

static QueueHandle_t gpio_evt_queue = NULL;

#define BOARD_LED 2
#define BOARD_BTN 0

#define GPIO_OUTPUT_PINS (1ULL << BOARD_LED)
#define GPIO_INPUT_PINS (1ULL << BOARD_BTN)

TaskHandle_t myTaskHandle = NULL;
TaskHandle_t myTask2Handle = NULL;
TaskHandle_t myTask3Handle = NULL;


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    bool led_state = false;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
            led_state = !led_state;
            gpio_set_level(BOARD_LED, led_state);
        }
    }
}

void task1(void *arg){
    while(1){
        printf("hello, world!\n");
        //vTaskDelay(1000 / portTICK_PERIOD_MS );
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task2(void *arg){
    while(1){
        printf("hello, world 2!\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS );
        //vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void task3(void *arg){
    char *thisTaskName = pcTaskGetName(NULL);
    while(1){
        ESP_LOGI(thisTaskName, "Hello, blyat!\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS );
        //vTaskDelay(pdMS_TO_TICKS(1000));
    }
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
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PINS;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    //gpio_set_intr_type(BOARD_BTN, GPIO_INTR_LOW_LEVEL);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BOARD_BTN, gpio_isr_handler, (void*) BOARD_BTN);

    //remove isr handler for gpio number.
    //gpio_isr_handler_remove(BOARD_BTN);
    //hook isr handler for specific gpio pin again
    //gpio_isr_handler_add(BOARD_BTN, gpio_isr_handler, (void*) BOARD_BTN);

    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //start other tasks
    xTaskCreate(task1, "task1", 4096, NULL, 10, &myTaskHandle);
    xTaskCreatePinnedToCore(task2, "task2", 4096, NULL, 10, &myTask2Handle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(task3, "task3", 4096, NULL, 10, &myTask3Handle, 1);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

    int cnt = 0;
    while (1) {
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    vTaskSuspend(NULL);
}

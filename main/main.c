#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

static const char *TAG = __FILE__;

extern void blufi_init(void);
void task_func(void *param);
extern void tcp_server(void *pvParameters);

void app_main(void)
{
    blufi_init();

    ESP_LOGI(TAG, "Hello world!\n");
    xTaskCreate(task_func, "task1", 4096, "task1", 5, NULL);
    xTaskCreate(task_func, "task2", 4096, "task2", 5, NULL);
    xTaskCreate(tcp_server, "tcp_server", 4096, 4, 5, NULL);
}

void task_func(void *param)
{
    char *str = (char *)param;

    for(;;) {
        ESP_LOGI(TAG, "this is %s\r\n", str);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

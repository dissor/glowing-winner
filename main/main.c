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

extern void gpio_test(void);

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!\n");
    gpio_test();

#if 0
    blufi_init();
    xTaskCreate(tcp_server, "tcp_server", 4096, 4, 5, NULL);
#endif
}

void task_func(void *param)
{
    char *str = (char *)param;

    for(;;) {
        ESP_LOGI(TAG, "this is %s\r\n", str);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define PORT 3333

static const char *TAG = __FILE__;
extern EventGroupHandle_t wifi_event_group;
extern int CONNECTED_BIT;

void tcp_server(void *pvParameters)
{
    EventBits_t xEventGroupValue;
    const EventBits_t xBitsToWaitFor = CONNECTED_BIT;

    char read_data[1024];
    int read_len = 0;

    int addr_family = ((int)pvParameters == 4) ? AF_INET : AF_INET6;
    int listen_sock = -1;
    struct sockaddr_in sa;

    xEventGroupValue = xEventGroupWaitBits(wifi_event_group, xBitsToWaitFor, pdTRUE, pdFALSE, portMAX_DELAY);

    ESP_LOGI(TAG, "tcp_server is CONNECTED");

    listen_sock = socket(addr_family, SOCK_STREAM, 0);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Socket created");

    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_sock, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        ESP_LOGE(TAG, "bind failed");
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    if (listen(listen_sock, 1) < 0)
    {
        ESP_LOGE(TAG, "listen failed");
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Socket listening");

    int ConnectFD = accept(listen_sock, NULL, NULL);
    if (ConnectFD < 0)
    {
        ESP_LOGE(TAG, "accept failed");
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    for (;;)
    {
        read_len = read(ConnectFD, read_data, sizeof(read_data));
        read_data[read_len] = '\0';
        ESP_LOGI(TAG, "read_len: %d, read_data: %s", read_len, read_data);
    }


}
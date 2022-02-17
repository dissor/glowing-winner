#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

/* TCK TMS TDI TRST_N OUT */
#define TCK (18)
#define TMS (19)
#define TDI (21)
#define TRST_N (23)
#define GPIO_OUTPUT_PIN_SEL ((1ULL << TCK) | (1ULL << TMS) | (1ULL << TDI) | (1ULL << TRST_N))

/* TDO IN  */
#define TDO (22)
#define GPIO_INPUT_PIN_SEL (1ULL << TDO)
#define ESP_INTR_FLAG_DEFAULT 0

typedef unsigned int u32;
typedef   signed int s32;

typedef unsigned short u16;
typedef   signed short s16;

typedef unsigned char u8;
typedef   signed char s8;

enum TAP_STATE_E {
    TAP_DREXIT2 = 0x0,
    TAP_DREXIT1 = 0x1,
    TAP_DRSHIFT = 0x2,
    TAP_DRPAUSE = 0x3,
    TAP_IRSELECT = 0x4,
    TAP_DRUPDATE = 0x5,
    TAP_DRCAPTURE = 0x6,
    TAP_DRSELECT = 0x7,
    TAP_IREXIT2 = 0x8,
    TAP_IREXIT1 = 0x9,
    TAP_IRSHIFT = 0xa,
    TAP_IRPAUSE = 0xb,
    TAP_IDLE = 0xc,
    TAP_IRUPDATE = 0xd,
    TAP_IRCAPTURE = 0xe,
    TAP_RESET = 0x0f,
};

u32 *idcode;
static xQueueHandle gpio_evt_queue = NULL;

u8 get_bit(void *map, u32 bit_max, u32 bit_index)
{
    u8  bit;
    u32 word_index, word_offset;
    u32 *pmap;

    bit_index = bit_index % bit_max;

    pmap = map;
    word_index  = bit_index / 32;
    word_offset = bit_index % 32;

    bit = pmap[word_index] & (0x1 << word_offset);

    return bit;
}

u8 set_bit(void *map, u32 bit_max, u32 bit_index, u8 bit)
{
    u32 word_index, word_offset;
    u32 *pmap;
    u32 bit_mask;

    bit_index = bit_index % bit_max;

    pmap = map;
    word_index  = bit_index / 32;
    word_offset = bit_index % 32;

    if (bit == 0) {
        bit_mask = ~(0x1 << word_offset);
        pmap[word_index] = (pmap[word_index]) & bit_mask;
    } else {    /* b == 1 */
        bit_mask = (0x1 << word_offset);
        pmap[word_index] = (pmap[word_index]) | bit_mask;
    }

    bit = pmap[word_index] & (0x1 << word_offset);
    return 0;
}

int gpio_get(int gpio_num)
{
    return gpio_get_level(gpio_num);
}

void gpio_set(int gpio_num, int value)
{
    gpio_set_level(gpio_num, value);
}

void gpio_init(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // //enable pull-up mode
    // io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

#if 0
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
#endif
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    gpio_set(TRST_N, 1);    /* we MUST always keep the TRST_N to 1 */
#if 0
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(TDO, gpio_isr_handler, (void *)TDO);

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
#endif
}

int jtag_clk()
{
    int tdo = 0;
    gpio_set(TCK, 0);
    // vTaskDelay(1);
    gpio_set(TCK, 1);
    // vTaskDelay(1);
    tdo = gpio_get(TDO);
    gpio_set(TCK, 0);

    return tdo;
}

void tap_reset()
{
    int i;
    gpio_set(TMS, 1);

    for(i = 0; i < 5; i++) {
        jtag_clk();
    }
}

void tap_state(int state)
{
    tap_reset();

    switch(state) {
        case (TAP_IRSHIFT):
            gpio_set(TMS, 0);
            jtag_clk();
            gpio_set(TMS, 1);
            jtag_clk();
            jtag_clk();
            gpio_set(TMS, 0);
            jtag_clk();
            jtag_clk();
            break;
        default:
            printf("unknown state %d \n", state);
    }

}


static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_entry_handler(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}


void gpio_test(void)
{
    int i = 0;
    u8 b = 0;
    u32 irlen = 0;
    u32 tap_num = 0;

    gpio_init();
    gpio_get(TCK);
    gpio_get(TMS);
    gpio_get(TDI);
    gpio_get(TDO);

    /* IR chain length */
    tap_reset();
    tap_reset();

    /* goto Select-DR */
    gpio_set(TMS, 0);
    jtag_clk();

    gpio_set(TMS, 1);
    jtag_clk();

    /* goto Shift-IR */
    gpio_set(TMS, 1);
    jtag_clk();

    gpio_set(TMS, 0);
    jtag_clk();

    gpio_set(TMS, 0);
    jtag_clk();

    printf("IR len: \n");
    /* fill the chain with 0 */
    gpio_set(TDI, 0);
    for(i = 0; i < 32; i++) {
        if (i != 0 && i % 4 == 0) {
            printf(" ");
        }

        b = jtag_clk();

        printf("%d", b);
    }

    printf("\n");

    gpio_set(TDI, 1);
    for(i = 0; i < 32; i++) {
        if (i != 0 && i % 4 == 0) {
            printf(" ");
        }

        b = jtag_clk();

        if (b == 0) {
            irlen++;
        }
        printf("%d", b);
    }

    printf("\nIR len: %d\n", irlen);
    /* now all IR Reg are filled with 1, means the BYPASS mode */

    /* goto Select-DR */
    gpio_set(TMS, 1);
    jtag_clk();

    gpio_set(TMS, 1);
    jtag_clk();

    gpio_set(TMS, 1);
    jtag_clk();

    /* goto Shift-DR */
    gpio_set(TDI, 0);

    gpio_set(TMS, 0);
    jtag_clk();

    gpio_set(TMS, 0);
    jtag_clk();

    printf("Tap Num:\n");
    /* fill the chain with 0 */
    gpio_set(TDI, 0);
    for(i = 0; i < 32; i++) {

        if (i != 0 && i % 4 == 0) {
            printf(" ");
        }

        b = jtag_clk();

        printf("%d", b);
    }

    printf("\n");

    gpio_set(TDI, 1);
    for(i = 0; i < 32; i++) {

        if (i != 0 && i % 4 == 0) {
            printf(" ");
        }

        b = jtag_clk();
        if (b == 0) {
            tap_num++;
        }

        printf("%d", b);
    }
    printf("\nTap Num: %d\n", tap_num);
    if (tap_num == 32) {
        return -1;
    }

    idcode = malloc(tap_num * sizeof(u32));

    /* number of devices in the jtag chain */
    tap_reset();
    tap_reset();

    /* goto Select-DR */
    gpio_set(TMS, 0);
    jtag_clk();

    gpio_set(TMS, 1);
    jtag_clk();

    /* goto Shift-DR */
    gpio_set(TMS, 0);
    jtag_clk();

    gpio_set(TMS, 0);
    jtag_clk();

    printf("IDCODE: \n");

    gpio_set(TDI, 0);

    for(i = 0; i < tap_num * sizeof(u32) * 8; i++) {
        if (i != 0 && i % 4 == 0) {
            printf(" ");
        }

        b = jtag_clk();
        set_bit(idcode, tap_num * sizeof(u32) * 8, i, b);

        printf("%d", b);
    }

    printf("\nIDCODE:\n");
    for(i = 0; i < tap_num; i++) {
        printf("[%d]: 0x%08x\n", i, idcode[i]);
    }

    free(idcode);

    tap_reset();
}


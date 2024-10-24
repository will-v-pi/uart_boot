#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "hardware/flash.h"

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 1000000

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5


void uart_boot() {

uart_boot_start:
    uart_putc_raw(UART_ID, 0x56);
    uart_putc_raw(UART_ID, 0xff);
    uart_putc_raw(UART_ID, 0x8b);
    uart_putc_raw(UART_ID, 0xe4);
    uart_putc_raw(UART_ID, 'n');

    if (uart_is_readable_within_us(UART_ID, 1000)) {
        char in = uart_getc(UART_ID);
        printf("%c\n", in);
    } else {
        printf("No response - knocking again\n");
        goto uart_boot_start;
    }

    printf("Boot starting\n");

    // Read binary
    const int buf_words = (16 * 4) + 1;   // maximum of 16 partitions, each with maximum of 4 words returned, plus 1
    uint32_t* buffer = malloc(buf_words * 4);

    int ret = rom_get_partition_table_info(buffer, buf_words, PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION | (0 << 24));
    assert(buffer[0] == (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION));
    assert(ret == 3);

    uint32_t location_and_permissions = buffer[1];
    uint32_t saddr = XIP_BASE + ((location_and_permissions >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) & 0x1fffu) * FLASH_SECTOR_SIZE;
    uint32_t eaddr = XIP_BASE + (((location_and_permissions >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB) & 0x1fffu) + 1) * FLASH_SECTOR_SIZE;
    printf("Start %08x, end %08x\n", saddr, eaddr);

    uint32_t caddr = saddr;
    while (caddr < eaddr) {
        uart_putc_raw(UART_ID, 'w');
        char *buf = (char*)caddr;
        for (int i=0; i < 32; i++) {
            uart_putc_raw(UART_ID, buf[i]);
        }
        printf("Written %08x\n", caddr);
        caddr += 32;
        char in = uart_getc(UART_ID);
        printf("%c\n", in);
    }

    uart_putc_raw(UART_ID, 'x');
    char in = uart_getc(UART_ID);
    printf("%c\n", in);
}


int main()
{
    stdio_init_all();

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    // Send out a string, with CR/LF conversions
    // uart_puts(UART_ID, " Hello, UART!\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    while (true) {
        char splash[] = "RP2350";
        char* buf = malloc(100);
        int i = 0;
        while (uart_is_readable(UART_ID)) {
            char in = uart_getc(UART_ID);
            printf("%c", in);
            buf[i] = in;
            i++;
        }
        if (i > 0) {
            printf("\nRead done\n");
        }
        bool found = true;
        for (int i=0; i < sizeof(splash); i++) {
            if (buf[i] != splash[i]) {
                found = false;
                break;
            }
        }
        if (found) {
            printf("Splash found\n");
            uart_boot();
        }
        sleep_ms(1000);
    }
}

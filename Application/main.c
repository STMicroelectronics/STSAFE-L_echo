/**
 ******************************************************************************
 * @file    main.c
 * @author  CS application team
 * @brief   STSAFE-L Echo Loop Example
 ******************************************************************************
 * @copyright 2022 STMicroelectronics
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "Drivers/delay_ms/delay_ms.h"
#include "Drivers/rng/rng.h"
#include "Drivers/uart/uart.h"
#include "stselib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Terminal control escape codes */
#define PRINT_CLEAR_SCREEN "\x1B[1;1H\x1B[2J"
#define PRINT_RESET "\x1B[0m"

/* STDIO redirect for UART output/input */
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar()
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE {
    uart_putc(ch);
    return ch;
}

GETCHAR_PROTOTYPE {
    return uart_getc();
}

/* --- Static Function Prototypes --- */
static void apps_terminal_init(uint32_t baudrate);
static void apps_print_hex_buffer(const uint8_t *buffer, uint16_t buffer_size);
static uint32_t apps_generate_random_number(void);
static void apps_randomize_buffer(uint8_t *pBuffer, uint16_t buffer_length);
static uint8_t apps_compare_buffers(const uint8_t *pBuffer1, const uint8_t *pBuffer2, uint16_t buffers_length);
static void apps_delay_ms(uint16_t ms);

/* --- Static Function Definitions --- */

/**
 * @brief  Initialize UART terminal for application output.
 * @param  baudrate: UART baudrate (not used, always 115200)
 */
static void apps_terminal_init(uint32_t baudrate) {
    (void)baudrate;
    uart_init(115200);
    setvbuf(stdout, NULL, _IONBF, 0); /* Disable buffering for stdout */
    printf(PRINT_RESET PRINT_CLEAR_SCREEN);
}

/**
 * @brief  Print a buffer as hex values, 16 bytes per line.
 * @param  buffer: Pointer to buffer
 * @param  buffer_size: Number of bytes to print
 */
static void apps_print_hex_buffer(const uint8_t *buffer, uint16_t buffer_size) {
    for (uint16_t i = 0; i < buffer_size; i++) {
        if (i % 16 == 0) {
            printf(" \n\r ");
        }
        printf(" 0x%02X", buffer[i]);
    }
}

/**
 * @brief  Generate a random 32-bit number using hardware RNG.
 * @retval Random 32-bit value
 */
static uint32_t apps_generate_random_number(void) {
    return rng_generate_random_number();
}

/**
 * @brief  Fill a buffer with random bytes.
 * @param  pBuffer: Pointer to buffer
 * @param  buffer_length: Number of bytes to fill
 */
static void apps_randomize_buffer(uint8_t *pBuffer, uint16_t buffer_length) {
    for (uint16_t i = 0; i < buffer_length; i++) {
        pBuffer[i] = (uint8_t)(rng_generate_random_number() & 0xFF);
    }
}

/**
 * @brief  Compare two buffers for equality.
 * @param  pBuffer1: Pointer to first buffer
 * @param  pBuffer2: Pointer to second buffer
 * @param  buffers_length: Number of bytes to compare
 * @retval 0 if equal, 1 if different
 */
static uint8_t apps_compare_buffers(const uint8_t *pBuffer1, const uint8_t *pBuffer2, uint16_t buffers_length) {
    for (uint16_t i = 0; i < buffers_length; i++) {
        if (pBuffer1[i] != pBuffer2[i]) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief  Delay for a specified number of milliseconds.
 * @param  ms: Number of milliseconds to delay
 */
static void apps_delay_ms(uint16_t ms) {
    delay_ms(ms);
}

/* --- Main application entry point --- */
int main(void) {
    stse_ReturnCode_t stse_ret = STSE_API_INVALID_PARAMETER;
    stse_Handler_t stse_handler;
    uint16_t message_length = 0;

    /* Initialize Terminal */
    apps_terminal_init(115200);

    /* Print Example instruction on terminal */
    printf(PRINT_CLEAR_SCREEN);
    printf("----------------------------------------------------------------------------------------------------------------");
    printf("\n\r-                                    STSAFE-L Echo loop example                                                -");
    printf("\n\r----------------------------------------------------------------------------------------------------------------");

    /* Initialize STSAFE-L010 device handler */
    stse_ret = stse_set_default_handler_value(&stse_handler);
    if (stse_ret != STSE_OK) {
        printf("\n\r ## stse_set_default_handler_value ERROR : 0x%04X\n\r", stse_ret);
        while (1)
            ;
    }
    stse_handler.device_type = STSAFE_L010;
    stse_handler.io.busID = 1;
    stse_handler.io.Devaddr = 0x0C;

    printf("\n\r - Initialize target STSAFE-L010");
    stse_ret = stse_init(&stse_handler);
    if (stse_ret != STSE_OK) {
        printf("\n\r ## stse_init ERROR : 0x%04X\n\r", stse_ret);
        while (1)
            ;
    }

    while (1) {
        /* Generate random message length (1..500) */
        message_length = (uint16_t)(apps_generate_random_number() & 0x1FF);
        if ((message_length > 500) || (message_length == 0)) {
            message_length = 1;
        }

        /* Create message and echo buffers (max 500 bytes) */
        uint8_t message[500] = {0};
        uint8_t echoed_message[500] = {0};

        /* Fill message with random content */
        apps_randomize_buffer(message, message_length);

        /* Print message */
        printf("\n\r ## Message :\n\r");
        apps_print_hex_buffer(message, message_length);

        /* Perform echo operation */
        stse_ret = stse_device_echo(&stse_handler, message, echoed_message, message_length);
        if (stse_ret != STSE_OK) {
            printf("\n\r## stse_device_echo ERROR : 0x%04X\n\r", stse_ret);
            while (1)
                ;
        }

        /* Compare message and echoed message */
        if (apps_compare_buffers(message, echoed_message, message_length)) {
            printf("\n\n \r ## ECHO MESSAGES COMPARE FAIL (%d)", message_length);
            printf("\n\r\t Echoed Message :\n\r");
            apps_print_hex_buffer(echoed_message, message_length);
            while (1)
                ;
        }
        printf("\n\n \r ## Echoed Message :\n\r");
        apps_print_hex_buffer(echoed_message, message_length);

        printf("\n\r----------------------------------------------------------------------------------------------------------------");

        /* Wait for 1s */
        apps_delay_ms(1000);
    }
}

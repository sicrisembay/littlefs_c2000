#ifndef DRIVER_DEF_H
#define DRIVER_DEF_H

#define CONFIG_SYSTEM_FREQ_MHZ  150

/*
 * UART Driver Configuration
 */
#define CONFIG_USE_UART                 1
#define CONFIG_ENABLE_UARTA             1
#if CONFIG_ENABLE_UARTA
#define CONFIG_BAUDRATE_UARTA           115200
#define CONFIG_TX_QUEUE_BUFF_SZ_UARTA   1024
#define CONFIG_RX_QUEUE_BUFF_SZ_UARTA   128
#endif

#define CONFIG_ENABLE_UARTB             0
#if CONFIG_ENABLE_UARTB
#define CONFIG_BAUDRATE_UARTB           115200
#define CONFIG_TX_QUEUE_BUFF_SZ_UARTB   256
#define CONFIG_RX_QUEUE_BUFF_SZ_UARTB   128
#endif

#define CONFIG_ENABLE_UARTC             0
#if CONFIG_ENABLE_UARTC
#define CONFIG_BAUDRATE_UARTC           115200
#define CONFIG_TX_QUEUE_BUFF_SZ_UARTC   256
#define CONFIG_RX_QUEUE_BUFF_SZ_UARTC   128
#endif

#endif /* DRIVER_DEF_H */

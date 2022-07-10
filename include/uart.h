// Daniel Gutierrez
// 18-jun-2022
// audobra@gmail.com
// SiFive FE310 UART Driver
// https://www.sifive.com/documentation
// Based on Chapter 18 v1p4 February 18, 2022

#include <stdint.h>
#include "gpio.h"


#define UART_BASE_ADDR0 0x10013000U
#define UART_BASE_ADDR1 0x10023000U
#define FREC_CLOCK      16000000        // 16 MHz
#define MAX_STR_LEN     80              // Max. Leghth of strings.

typedef volatile uint32_t addr_t;

struct txdata_bits
{
    addr_t data :8;                 // RW X Transmit data
    addr_t      :23;                // Reserved
    addr_t full :1;                 // RO X Transmit FIFO full
};

struct rxdata_bits
{
    addr_t data   :8;                // RO X Received data.
    addr_t        :23;               // Reserved
    addr_t empty  :1;                // RO X Receive FIFO empty
};

struct txctrl_bits
{
    addr_t txen  :1;                 // RW 0x0 Transmit enable
    addr_t nstop :1;                 // RW 0x0 Number of stop bits
    addr_t       :14;                // Reserved
    addr_t txcnt :3;                 // RW 0x0 Transmit watermark level
    addr_t       :13;                // Reserved
};

struct rxctrl_bits
{
    addr_t rxen  :1;                // RW 0x0 Receive enable
    addr_t       :15;               // Reserved
    addr_t rxcnt :3;                // Receive watermark level
    addr_t       :13;               // Reserved

};

struct ie_bits
{
    addr_t txwm :1;                 // RW 0x0 Transmit watermark interrupt enable
    addr_t rxwm :1;                 // RW 0x0 Receive watermark interrupt enable
    addr_t      :30;                // Reserved
};

struct ip_bits
{
    addr_t txwm :1;                 // RO X Transmit watermark interrupt pending
    addr_t rxwm :1;                 // RO X Receive watermark interrupt pending
    addr_t      :30;                // Reserved
};

struct div_bits
{
    addr_t div :16;                 // RW XBaud rate divisor. div_width bits 
                                    //wide, and the reset value is div_init
    addr_t     :16;                 // Reserved
};


struct UART_ADDRS
{
    struct txdata_bits txdata;      // Transmit data register
    struct rxdata_bits rxdata;      // Receive data register
    struct txctrl_bits txctrl;      // Transmit control register
    struct rxctrl_bits rxctrl;      // Receive control register
    struct ie_bits ie;              // UART interrupt enable
    struct ip_bits ip;              // UART interrupt pending
    struct div_bits div;            // Baud rate divisor
};

struct UART_ADDRS *UART0 = (struct UART_ADDRS *) UART_BASE_ADDR0;
struct UART_ADDRS *UART1 = (struct UART_ADDRS *) UART_BASE_ADDR1;

int uart_setup (char instance, uint32_t baud)
{
    uint16_t div = FREC_CLOCK / baud;

    switch (instance)
    {
    case 0:
        pin_setup (16, 'U');        // UART0_RX
        pin_setup (17, 'U');        // UART0_TX
        UART0->div.div = div;
        UART0->txctrl.txen  = 1;    // To enable transmission.
        UART0->txctrl.nstop = 1;    // 1 stop bit
        UART0->rxctrl.rxen  = 1;    // To enable receiver.
        break;
    
    case 1:
        pin_setup (18, 'U');        // UART1_TX
        pin_setup (23, 'U');        // UART1_RX
        UART1->div.div = div;
        UART1->txctrl.txen  = 1;
        UART1->txctrl.nstop = 1;
        UART1->rxctrl.rxen  = 1;
        break;
    }

    return 0;
}


uint8_t get_serial_char0 ()
{
    volatile struct rxdata_bits rx_reg;
    // Así se genera un registro.
    // Hay que descargar toda la estructra para leer
    // los campos todos a la vez.

    while (1)
    {
        rx_reg = UART0->rxdata;
        if (rx_reg.empty == 0)
            return (uint8_t) rx_reg.data;
    }
}

void put_serial_char0 (uint8_t txchar)
{
    while (UART0->txdata.full);
    UART0->txdata.data = txchar;
}

void get_serial_str0 (char *str)
{
    int i = 0;
    do
    {
        str[i] = get_serial_char0();
    } while (str[i++] != '\r' && (i < MAX_STR_LEN));
    str[i-1] = 0; 
}

int put_serial_str0 (char *str)
{
    int i = 0;
    while (str[i] != 0)
        put_serial_char0(str[i++]);

    return i;   
}
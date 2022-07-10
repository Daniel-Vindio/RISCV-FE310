// Daniel Gutierrez
// 18-jun-2022
// audobra@gmail.com
// SiFive FE310 GPIO Driver
// https://www.sifive.com/documentation
// Based on Chapter 18 v1p4 February 18, 2022


#include "gpio.h"

struct GPIO_ADDRS *GPIO = (struct GPIO_ADDRS*) GPIO_BASE_ADDR;

void pin_setup (int pin, char mode)
{
// mode: I (GPIO input), O (GPIO output)
// mode: S (SPI), P (PWM), U (UART)

    switch (mode)
    {
    case 'I':
        GPIO->iof_en    &= ~(1 << pin);
        GPIO->output_en &= ~(1 << pin);
        GPIO->input_en  |=  (1 << pin);
        break;

    case 'O':
        GPIO->iof_en    &= ~(1 << pin);
        GPIO->output_en |=  (1 << pin);
        GPIO->input_en  &= ~(1 << pin);
        break;

    case 'S':
        GPIO->iof_en    |=  (1 << pin);
        GPIO->iof_sel   &= ~(1 << pin);
        break;
        
    case 'P':
        GPIO->iof_en    |=  (1 << pin);
        GPIO->iof_sel   |=  (1 << pin);
        break;
    
    case 'U':
        GPIO->iof_en    |=  (1 << pin);
        GPIO->iof_sel   &= ~(1 << pin);
        break;
    
    default:
        GPIO->iof_en    &= ~(1 << pin);
        GPIO->output_en &= ~(1 << pin);
        GPIO->input_en  &= ~(1 << pin);
        break;
    }

}

void gpio_clear_set (int pin, int val)
{
    if (val) GPIO->output_val |=  (1 << pin);
    else     GPIO->output_val &= ~(1 << pin);
}

int read_pin (int pin)
{
    return ((GPIO->input_val >> pin) & 0x1);
}

int read_output_pin (int pin)
{
    // Es para ver qué está saliendo por el pin.
    return ((GPIO->output_val >> pin) & 0x1);
}

// Daniel Gutierrez
// 18-jun-2022
// audobra@gmail.com
// SiFive FE310 GPIO Driver
// https://www.sifive.com/documentation
// Based on Chapter 18 v1p4 February 18, 2022

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define GPIO_BASE_ADDR 0x10012000U
typedef volatile uint32_t addr_t;

// Controller GPIO memory address map.
struct GPIO_ADDRS
{
    addr_t input_val;
    addr_t input_en;
    addr_t output_en;
    addr_t output_val;
    addr_t pue;
    addr_t ds;
    addr_t rise_ie;
    addr_t rise_ip;
    addr_t fall_ie;
    addr_t fall_ip;
    addr_t high_ie;
    addr_t high_ip;
    addr_t low_ie;
    addr_t low_ip;
    addr_t iof_en;
    addr_t iof_sel;
    addr_t out_xor;
    addr_t passthru_high_ie;
    addr_t passthru_low_ie;
};

struct GPIO_ADDRS *GPIO;

void pin_setup (int, char);
void gpio_clear_set (int, int);
int read_pin (int);
int read_output_pin (int);

#endif  //GPIO_H
// Daniel Gutierrez
// audobra@gmail.com
// 26-jun-2022
// Adaptación para generar contadores de microsegundos.

#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "gpio.h"

#define FREQ_BASE   16  // MHz
#define PIN_PWM     11  // Pin dedicado a PWM

// For the time being, we will only work with instance number 2.
#define PWM_BASE_ADDR2 0x10035000U

typedef volatile uint32_t addr_t;

struct pwmcfg_bits
{
   addr_t pwmscale      :4;   // PWM Counter scale
   addr_t               :4;   // Reserved
   addr_t pwmsticky     :1;   // PWM Sticky - disallow clearing pwmcmp ip bits
   addr_t pwmzerocmp    :1;   // PWM Zero - counter resets to zero after match
   addr_t pwmdeglitch   :1;   // PWM Deglitch - latch pwmcmp ip within same cycle
   addr_t               :1;   // Reserved
   addr_t pwmenalways   :1;   // PWM enable always - run continuously
   addr_t pwmenoneshot  :1;   // PWM enable one shot - run one cycle
   addr_t               :2;   // Reserved    
   addr_t pwmcmp0center :1;   // PWMX Compare Center
   addr_t pwmcmp1center :1;   // PWMX Compare Center
   addr_t pwmcmp2center :1;   // PWMX Compare Center
   addr_t pwmcmp3center :1;   // PWMX Compare Center
   addr_t               :4;   // Reserved
   addr_t pwmcmp0gang   :1;   // PWM0/PWM1 Compare Gang
   addr_t pwmcmp1gang   :1;   // PWM1/PWM2 Compare Gang
   addr_t pwmcmp2gang   :1;   // PWM2/PWM3 Compare Gang
   addr_t pwmcmp3gang   :1;   // PWM3/PWM0 Compare Gang
   addr_t pwmcmp0ip     :1;   // PWM0 Interrupt Pending
   addr_t pwmcmp1ip     :1;   // PWM1 Interrupt Pending
   addr_t pwmcmp2ip     :1;   // PWM2 Interrupt Pending
   addr_t pwmcmp3ip     :1;   // PWM3 Interrupt Pending
};

struct pwmcount_bits
{
    addr_t pwmcount :31;    // PWM count register. cmpwidth + 15 bits wide.
    addr_t          : 1;    // Reserved
};

struct pwms_bits
{
    addr_t pwms :16;        // Scaled PWM count register. cmpwidth bits wide.
    addr_t      :16;
};

struct pwmcmp_bits
{
    addr_t pwmcmp  :16;        // PWM X Compare Value
    addr_t         :16;
};

struct PWM_ADDRS
{
    struct pwmcfg_bits pwmcfg;
    addr_t reserved1;
    struct pwmcount_bits pwmcount;
    addr_t reserved2;
    struct pwms_bits pwms;
    addr_t reserved3;
    addr_t reserved4;
    addr_t reserved5;
    struct pwmcmp_bits pwmcmp0;
    struct pwmcmp_bits pwmcmp1;
    struct pwmcmp_bits pwmcmp2;
    struct pwmcmp_bits pwmcmp3;
};

struct PWM_ADDRS *PWM02;

void pwmInit(void);
void pwm(float, float);

#endif  //PWM_H
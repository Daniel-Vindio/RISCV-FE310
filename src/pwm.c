// Daniel Gutierrez
// audobra@gmail.com
// 26-jun-2022
// Adaptación para generar contadores de microsegundos.

#include "pwm.h"

struct PWM_ADDRS *PWM02 = (struct PWM_ADDRS*) PWM_BASE_ADDR2;

void pwmInit(void) {
    pin_setup (PIN_PWM, 'P');
    memset (PWM02, 0, sizeof(&PWM02)); //Pone a cero todos los bits.
    PWM02->pwmcfg.pwmenalways = 1;
    PWM02->pwmcfg.pwmzerocmp  = 1;
    //PWM02->pwmcfg.pwmsticky   = 1;
}

void pwm(float freq, float duty) {
    
    int comp_max = 65535;
    int comp;
    int8_t scale = -1;

    do
    {
        scale++;
        comp = (FREQ_BASE * 1E6) / (freq * pow(2,scale));
    } while (comp > comp_max);
        
    PWM02->pwmcmp0.pwmcmp = comp;
    PWM02->pwmcfg.pwmscale = scale;
    PWM02->pwmcmp1.pwmcmp = comp * (1 - duty);
}

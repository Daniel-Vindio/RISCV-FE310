// Daniel Gutierrez
// audobra@gmail.com
// 8-aug-2022


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef volatile uint32_t addr_t;

#define MAX_INTERRUPTS 16
// Arrays of pointers to void functions.
void (*interrupt_handler [MAX_INTERRUPTS]) ();
void (*exception_handler [MAX_INTERRUPTS]) ();

// -------- Pulse Width Modulator (PWM) -----------------------
//#define FREQ_BASE 16        // Usually 16 MHz, but now it depends on HFROSC
#define PIN_PWM     11        // PWM dedicated pin
// For the time being, we will only work with instance number 2.
#define PWM_BASE_ADDR2 0x10035000U
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
    addr_t pwmcount :31;      // PWM count register. cmpwidth + 15 bits wide.
    addr_t          : 1;      // Reserved
};
struct pwms_bits
{
    addr_t pwms :16;          // Scaled PWM count register. cmpwidth bits wide.
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
struct PWM_ADDRS *PWM02 = (struct PWM_ADDRS*) PWM_BASE_ADDR2;

// -------- General Purpose Input/Output Controller (GPIO) ---
#define GPIO_BASE_ADDR 0x10012000U
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
struct GPIO_ADDRS *GPIO = (struct GPIO_ADDRS*) GPIO_BASE_ADDR;

// -------- Platform-Level Interrupt Controller (PLIC) --------
#define PLIC_ENABLE_REG2        0x0C002004U
#define PLIC_PRIORITY_REG_48    0x0C0000C0U     // PWM2 Interrupt source 48->16
#define PLIC_THRESHOLD_REG      0x0C200000U
#define PLIC_CLAIM_COMP_REG     0x0C200004U
#define PLIC_PENDING_REG2       0x0C001004U     // Not used in this case.

// -------- Core-Local Interruptor (CLINT) --------------------
#define MTIMECMP    (uint64_t *) 0x02004000UL
#define MTIME       (uint64_t *) 0x0200bff8UL
#define TIMER_FREQ  32768       // 2^15 Hz
#define BLINK_FREQ  4           // For debugging, in LED 5.

// -------- Phase-locked loop (PLL) ---------------------------
#define PLL_CONFG_STATUS_REG    0X10008008U

// Ring Oscillator Configuration and Status (72 MHz) ----------
#define HFROSC_CONFG_REG        0X10008000U
struct hfrosc_bits
{
    addr_t hfroscdiv  : 6;      // Ring Oscillator Divider Register 0x4
    addr_t            :10;      // Reserved
    addr_t hfrosctrim : 5;      // Ring Oscillator Trim Register 0x10
    addr_t            : 9;      // Reserved
    addr_t hfroscen   : 1;      // 0x1 Ring Oscillator Enable
    addr_t hfroscrdy  : 1;      // RO X Ring Oscillator Ready
};

// Macros for reading and writing the control and status registers (CSRs)
#define read_csr(reg) ({ unsigned long __tmp; asm volatile ("csrr %0, " #reg : "=r"(__tmp)); __tmp; })
#define write_csr(reg, val) ({ asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

void pin_setup (int, char);
void gpio_clear_set (int, int);
void pwmInit (void);
void pwm (double, float);
void timer_handler (void);
void PLIC_handler (void);
void reset_timer (int);
void reset_pwm_count (void);
void handle_trap (void) __attribute((interrupt));


volatile uint64_t contador;

// HFROSC frequency range of 1.125 MHz–72 MHz
volatile float FREQ_BASE;


int main(void)
{
    // General setup. Three LEDs have been configured to display the frequencies.
    // Pin 5 for CLINT timer interrupts. Pin 11 is dedicated to the PWM output. 
    // Pin 10 will display each time the PLIC handler is called.
    // "contador" is the variable in which a counter that tracks the PWM frequency
    // is expected to be stored.
    pin_setup (5, 'O');
    pin_setup (10, 'O');
    contador = 0;

    reset_timer (TIMER_FREQ / (2 * BLINK_FREQ));
    reset_pwm_count();

    interrupt_handler [7] = timer_handler;
    interrupt_handler [11] = PLIC_handler;
    
    // Registration of the trap handler in mtvec.
    write_csr(mtvec, ((unsigned long) handle_trap) & ~(0b11));

    // Enabling of the interrupts
    write_csr(mstatus, read_csr(mstatus) | (1 << 3));
    //write_csr(mie, read_csr(mie) |  (1 << 3));
    //write_csr(mie, read_csr(mie) |  (1 << 7));
    write_csr(mie, read_csr(mie) |  (1 << 11));   // Enable general PLIC


    // PLIC setup.
    // 1.- Enabling in this case PWM2 Interrupt source 48, Interrupt ID 16
    // whitch corresponds to pwmcmp0 of instance number 2 of PWM.
    uint32_t *PLIC_I_EN = (uint32_t *) PLIC_ENABLE_REG2;
    int val_en = (*PLIC_I_EN >> 16) & 0x1;
    if (val_en) *PLIC_I_EN  &= ~(1 << 16);      // Disable


    // 2.- Priority setup of th interrupt source 48.
    uint32_t *PLIC_PRTY = (uint32_t *) PLIC_PRIORITY_REG_48;
    *PLIC_PRTY &= 0x0;
    *PLIC_PRTY |= 0x1;

    // 3.- Threshold setup of the HART.
    uint32_t *PLIC_THR = (uint32_t*) PLIC_THRESHOLD_REG;
    *PLIC_THR &= 0x0;

    // Internal High-Frequency PLL (HFPLL) Setup
    // Enabling for HFROSC
    uint32_t *PLL_CFG =  (uint32_t*) PLL_CONFG_STATUS_REG;
    // Bit 16 PLL Select default in reset -> 0. OK.
    // Bit 18 PLL Bypass default in reset -> ~0. Needed ~1.
    *PLL_CFG &= ~(1 << 18);       // Enable HFROSC
    //*PLL_CFG |=  (1 << 18);     // Disable HFROSC -> FREQ_BASE = 16;
    

    // Bits [5:0] hfroscdiv Reset -> 0x4 Ring Oscillator Divider Register
    // 0 -> Divided by 1, 1 -> Divided by 2 ... 63 Divided by 64
    struct hfrosc_bits *HFROSC_CFG =  (struct hfrosc_bits*) HFROSC_CONFG_REG;
    uint8_t divider = 17;
    HFROSC_CFG->hfroscdiv = divider;
    FREQ_BASE = (float) 72 / (divider + 1);

    // PWM setup. 
    pwmInit ();
    pwm (1e5, 0.5);     //No more breakpoints until printf.

    *PLIC_I_EN |= (1 << 16);
    *MTIME = 0;
    do{} while (contador  < 1000);
    *PLIC_I_EN  &= ~(1 << 16);

    volatile float duration = (float) *MTIME / TIMER_FREQ;
    volatile float frec = contador / duration; 
    printf ("FREC %f\n", frec);

    while(1) {};
    return 0;
}

void PLIC_handler (void)
{
    // Dummy handler function that simply toggles the LED in pin 10 whenever 
    // the PLIC interrupt is served, and also increments the counter.
    int pin_val = (GPIO->output_val >> 10) & 1;
    if (pin_val) gpio_clear_set (10,0);
    else         gpio_clear_set (10,1);
    contador++;
}

void reset_pwm_count (void)
{
    PWM02->pwmcount.pwmcount = 0;
}

void timer_handler (void)
{
    // Dummy handler function that simply toggles LED in pin 5 whenever the   
    // CLINT interrupt is served.
    int pin_val = (GPIO->output_val >> 5) & 1;
    if (pin_val) gpio_clear_set (5,0);
    else         gpio_clear_set (5,1);
    reset_timer (TIMER_FREQ / (2 * BLINK_FREQ));
}

void reset_timer (int count_val)
{
    *MTIME = 0;
    *MTIMECMP = count_val;
}

void handle_trap (void) 
{
    uint32_t mcause_value = read_csr (mcause);
    uint32_t mcause_type = (mcause_value >> 31) & 1;
    uint32_t mcause_code = mcause_value & 0x3FF;
    uint32_t *plic_id = (uint32_t *) PLIC_CLAIM_COMP_REG;


    // Minimal handler avoid possible latency problems.
    if (*plic_id == 48)
        {
            interrupt_handler [11] ();
            *plic_id = 48;
        }
    else *plic_id = *plic_id;

    /*
    // More general case to handle CLINT, PLIC and exception interrupts.
    if (mcause_type == 1)
        switch (mcause_code)
        {
        case 7:
            interrupt_handler [7] ();
            break;
        case 11:
            if (*plic_id == 48)
            {
                interrupt_handler [11] ();
                *plic_id = 48;
            }
            else *plic_id = *plic_id;
            break;
        }
        
    else
        exception_handler [mcause_code] ();
    */

}

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

void pwmInit (void) 
{
    pin_setup (PIN_PWM, 'P');
    memset (PWM02, 0, sizeof(&PWM02)); // To clear all bits.
    PWM02->pwmcfg.pwmenalways = 1;
    PWM02->pwmcfg.pwmzerocmp  = 1;
    PWM02->pwmcfg.pwmsticky   = 0;
    PWM02->pwmcfg.pwmdeglitch = 0;
}

void pwm (double freq, float duty) 
{
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

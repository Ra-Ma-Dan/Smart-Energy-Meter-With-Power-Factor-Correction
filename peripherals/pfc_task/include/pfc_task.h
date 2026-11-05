#ifndef PFC_TASK_H
#define PFC_TASK_H


#include "my_gpio.h"


// Precise reactive capacity values (VAR) calculated for each of your unique physical capacitors
static const float bank_var_values[NUM_CAP_BANKS] = {
    76.0f,   // Bank 1 value (e.g. 5uF)
    152.0f,  // Bank 2 value (e.g. 10uF)
    304.0f,  // Bank 3 value (e.g. 20uF)
    608.0f   // Bank 4 value (e.g. 40uF)
};


void power_factor_correction_task(void *pvParameters);



#endif

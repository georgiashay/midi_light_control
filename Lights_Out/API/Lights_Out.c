/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include <cytypes.h>
#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_BUCK_DUTY.h"
#include "`$INSTANCE_NAME`_LIGHT_COUNTER.h"
#include "`$INSTANCE_NAME`_LED_STATUS.h"

// Duty cycles for each number of LEDs 0 - 7
uint8 `$INSTANCE_NAME`_dutyCycles[8] = { 0u, 8u, 20u, 30u, 46u, 77u, 91u, 101u };

uint8 `$INSTANCE_NAME`_lastNum = 0u;

void `$INSTANCE_NAME`_Start() {
    `$INSTANCE_NAME`_BUCK_DUTY_Start();
    `$INSTANCE_NAME`_LIGHT_COUNTER_Start();
}

void `$INSTANCE_NAME`_Poll() {
    // Count the number of LEDs currently on
    int numLeds = 0u;
    uint8_t hotLeds = `$INSTANCE_NAME`_LED_STATUS_Read();
    while (hotLeds) {
        numLeds += hotLeds & 1u;
        hotLeds = hotLeds >> 1;
    }
    // Set PWM to appropriate duty cycle
    if (`$INSTANCE_NAME`_lastNum != numLeds) {
        `$INSTANCE_NAME`_BUCK_DUTY_WriteCompare(`$INSTANCE_NAME`_dutyCycles[numLeds]);
        `$INSTANCE_NAME`_lastNum = numLeds;
    } 
}


/* [] END OF FILE */

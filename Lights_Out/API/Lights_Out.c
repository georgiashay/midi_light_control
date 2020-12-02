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

uint8 `$INSTANCE_NAME`_dutyCycles[9] = { 0u, 5u, 16u, 25u, 37u, 72u, 88u, 94u, 0u };

uint8 `$INSTANCE_NAME`_lastNum = 0u;

void `$INSTANCE_NAME`_Start() {
    `$INSTANCE_NAME`_BUCK_DUTY_Start();
    `$INSTANCE_NAME`_LIGHT_COUNTER_Start();
}

void `$INSTANCE_NAME`_Poll() {
    int numLeds = 0u;
    uint8_t hotLeds = `$INSTANCE_NAME`_LED_STATUS_Read();
    while (hotLeds) {
        numLeds += hotLeds & 1u;
        hotLeds = hotLeds >> 1;
    }
    if (`$INSTANCE_NAME`_lastNum != numLeds) {
        `$INSTANCE_NAME`_BUCK_DUTY_WriteCompare(`$INSTANCE_NAME`_dutyCycles[numLeds]);
        `$INSTANCE_NAME`_lastNum = numLeds;
    } 
}


/* [] END OF FILE */

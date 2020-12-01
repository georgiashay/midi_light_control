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

#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_LED_Out_1.h"
#include "`$INSTANCE_NAME`_LED_Out_2.h"
#include "`$INSTANCE_NAME`_LED_Out_3.h"
#include "`$INSTANCE_NAME`_LED_Out_4.h"
#include "`$INSTANCE_NAME`_LED_Out_5.h"
#include "`$INSTANCE_NAME`_LED_Out_6.h"
#include "`$INSTANCE_NAME`_LED_Out_7.h"
#include "`$INSTANCE_NAME`_PWM_COUNTER.h"

void `$INSTANCE_NAME`_Start() {
    `$INSTANCE_NAME`_LED_Out_1_Start();
    `$INSTANCE_NAME`_LED_Out_2_Start();
    `$INSTANCE_NAME`_LED_Out_3_Start();
    `$INSTANCE_NAME`_LED_Out_4_Start();
    `$INSTANCE_NAME`_LED_Out_5_Start();
    `$INSTANCE_NAME`_LED_Out_6_Start();
    `$INSTANCE_NAME`_LED_Out_7_Start();
    `$INSTANCE_NAME`_PWM_COUNTER_Start();
}

/* [] END OF FILE */

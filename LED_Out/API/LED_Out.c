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
#include "`$INSTANCE_NAME`_TRIANGLE_OUT.h"
#include "`$INSTANCE_NAME`_OUT_COMP.h"
#include "`$INSTANCE_NAME`_BRIGHTNESS_DAC.h"

void `$INSTANCE_NAME`_Start() {
    `$INSTANCE_NAME`_TRIANGLE_OUT_Start();
    `$INSTANCE_NAME`_OUT_COMP_Start();
    `$INSTANCE_NAME`_BRIGHTNESS_DAC_Start();
}

/* [] END OF FILE */

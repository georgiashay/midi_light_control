/*******************************************************************************
* File Name: main.c
*
* Version: 1.0
*
* Description:
*  This example project demonstrates the MIDI interface device operation.  The 
*  project enumerates as a USB Audio Device with the MIDI feature and does not 
*  require additional drivers. The main goal of the USB MIDI interface is to 
*  transfer and convert MIDI data between external MIDI devices that use the 
*  UART interface, and a PC through the USB bus.			
*   
* Related Document:
*  Universal Serial Bus Specification Revision 2.0 
*  Universal Serial Bus Device Class Definition for MIDI Devices Release 1.0
*  MIDI 1.0 Detailed Specification Document Version 4.2
*
********************************************************************************
* Copyright 2012-2015, Cypress Semiconductor Corporation. All rights reserved.
* This software is owned by Cypress Semiconductor Corporation and is protected
* by and subject to worldwide patent and copyright laws and treaties.
* Therefore, you may use this software only as provided in the license agreement
* accompanying the software package from which you obtained this software.
* CYPRESS AND ITS SUPPLIERS MAKE NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* WITH REGARD TO THIS SOFTWARE, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT,
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*******************************************************************************/

#include <project.h>

#define DEVICE                  (0u)
#define MIDI_MSG_SIZE           (4u)

/*MIDI Message Fields */
#define MIDI_MSG_TYPE           (0u)
#define MIDI_NOTE_NUMBER        (1u)
#define MIDI_NOTE_VELOCITY      (2u)

#define USB_SUSPEND_TIMEOUT     (2u)

/* Identity Reply message */
const uint8 CYCODE MIDI_IDENTITY_REPLY[] = {
    0xF0u,      /* SysEx */
    0x7Eu,      /* Non-real time */
    0x7Fu,      /* ID of target device (7F - "All Call") */
    0x06u,      /* Sub-ID#1 - General Information */
    0x02u,      /* Sub-ID#2 - Identity Reply */
    0x7Du,      /* Manufacturer's ID: 7D - Educational Use */
    0xB4u, 0x04u,               /* Family code */
    0x32u, 0xD2u,               /* Model number */
    0x01u, 0x00u, 0x00u, 0x00u, /* Version number */
    /*0xF7         End of SysEx automatically appended */
};

/* Need for Identity Reply message */
volatile uint8 USB_MIDI1_InqFlags;
volatile uint8 USB_MIDI2_InqFlags;

volatile uint8 usbActivityCounter = 0u;

uint8 inqFlagsOld = 0u;


/*******************************************************************************
* Function Name: SleepIsr
********************************************************************************
* Summary:
*  The sleep interrupt-service-routine used to determine a sleep condition.
*  The device goes into the Suspend state when there is a constant Idle 
*  state on its upstream-facing bus-lines for more than 3.0 ms. 
*  The device must be suspended drawing only suspend current from the 
*  bus after no more than 10 ms of the bus inactivity on all its ports.
*  This ISR is run each 4 ms, so after a second turn without the USB activity,  
*  the device should be suspended.
*
*******************************************************************************/
CY_ISR(SleepIsr)
{
    /* Check USB activity */
    if(0u != USB_CheckActivity()) 
    {
        usbActivityCounter = 0u;
    } 
    else 
    {
        usbActivityCounter++;
    }
    /* Clear Pending Interrupt */
    SleepTimer_GetStatus();
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*       1. Starts the USBFS device and waits for enumaration.
*
*******************************************************************************/
int main()
{
    /* Enable Global Interrupts */
    CyGlobalIntEnable;

    /* Start USBFS device 0 with VDDD operation */
    USB_Start(DEVICE, USB_DWR_VDDD_OPERATION); 
   
    while(1u)
    {
        /* Host can send double SET_INTERFACE request */
        if(0u != USB_IsConfigurationChanged())
        {
            /* Initialize IN endpoints when device configured */
            if(0u != USB_GetConfiguration())   
            {
                /* Start ISR to determine sleep condition */		
                Sleep_isr_StartEx(SleepIsr);
                
                /* Start SleepTimer's operation */
                SleepTimer_Start();
                
            	/* Enable output endpoint */
                USB_MIDI_Init();
            }
            else
            {
                SleepTimer_Stop();
            }    
        }        
        
        /* Service USB MIDI when device is configured */
        if(0u != USB_GetConfiguration())    
        {
            /* Call this API from UART RX ISR for Auto DMA mode */
            #if(!USB_EP_MANAGEMENT_DMA_AUTO) 
                USB_MIDI_IN_Service();
            #endif
            /* In Manual EP Memory Management mode OUT_EP_Service() 
            *  may have to be called from main foreground or from OUT EP ISR
            */
            #if(!USB_EP_MANAGEMENT_DMA_AUTO) 
                USB_MIDI_OUT_Service();
            #endif

            /* Sending Identity Reply Universal System Exclusive message 
             * back to computer */
            if(0u != (USB_MIDI1_InqFlags & USB_INQ_IDENTITY_REQ_FLAG))
            {
                USB_PutUsbMidiIn(sizeof(MIDI_IDENTITY_REPLY), \
                            (uint8 *)MIDI_IDENTITY_REPLY, USB_MIDI_CABLE_00);
                USB_MIDI1_InqFlags &= ~USB_INQ_IDENTITY_REQ_FLAG;
            }
            #if (USB_MIDI_EXT_MODE >= USB_TWO_EXT_INTRF)
                if(0u != (USB_MIDI2_InqFlags & USB_INQ_IDENTITY_REQ_FLAG))
                {
                    USB_PutUsbMidiIn(sizeof(MIDI_IDENTITY_REPLY), \
                            (uint8 *)MIDI_IDENTITY_REPLY, USB_MIDI_CABLE_01);
                    USB_MIDI2_InqFlags &= ~USB_INQ_IDENTITY_REQ_FLAG;
                }
            #endif /* End USB_MIDI_EXT_MODE >= USB_TWO_EXT_INTRF */
    			
            #if(USB_EP_MANAGEMENT_DMA_AUTO) 
               #if (USB_MIDI_EXT_MODE >= USB_ONE_EXT_INTRF)
                    MIDI1_UART_DisableRxInt();
                    #if (USB_MIDI_EXT_MODE >= USB_TWO_EXT_INTRF)
                        MIDI2_UART_DisableRxInt();
                    #endif /* End USB_MIDI_EXT_MODE >= USB_TWO_EXT_INTRF */
                #endif /* End USB_MIDI_EXT_MODE >= USB_ONE_EXT_INTRF */            
                USB_MIDI_IN_Service();
                #if (USB_MIDI_EXT_MODE >= USB_ONE_EXT_INTRF)
                    MIDI1_UART_EnableRxInt();
                    #if (USB_MIDI_EXT_MODE >= USB_TWO_EXT_INTRF)
                        MIDI2_UART_EnableRxInt();
                    #endif /* End USB_MIDI_EXT_MODE >= USB_TWO_EXT_INTRF */
                #endif /* End USB_MIDI_EXT_MODE >= USB_ONE_EXT_INTRF */                
            #endif
    		
            /* Check if host requested USB Suspend */
            if( usbActivityCounter >= USB_SUSPEND_TIMEOUT ) 
            {    
           
                /***************************************************************
                * Disable USBFS block and set DP Interrupt for wake-up 
                * from sleep mode. 
                ***************************************************************/
                USB_Suspend(); 
                /* Prepares system clocks for sleep mode */
                CyPmSaveClocks();
                /***************************************************************
                * Switch to the Sleep Mode for the PSoC 3 or PSoC 5LP devices:
                *  - PM_SLEEP_TIME_NONE: wakeup time is defined by PICU source
                *  - PM_SLEEP_SRC_PICU: PICU wakeup source 
                ***************************************************************/
                CyPmSleep(PM_SLEEP_TIME_NONE, PM_SLEEP_SRC_PICU);
                /* Restore clock configuration */
                CyPmRestoreClocks();
                /* Enable USBFS block after power-down mode */
                USB_Resume();
                
                /* Enable output endpoint */
                USB_MIDI_Init();

                usbActivityCounter = 0u; /* Re-init USB Activity Counter*/
            }
        }
    }
}


/*******************************************************************************
* Function Name: USB_callbackLocalMidiEvent
********************************************************************************
* Summary: Local processing of the USB MIDI out-events.
*
*******************************************************************************/
void USB_callbackLocalMidiEvent(uint8 cable, uint8 *midiMsg) CYREENTRANT
{
    
    uint8 pitchNumber = (midiMsg[MIDI_NOTE_NUMBER] + 7) % 12;
    uint8 keyNumber;
    short isValidKey;
    if (pitchNumber < 7) {
        isValidKey = pitchNumber % 2 == 0;
        keyNumber = pitchNumber / 2;
    } else {
        isValidKey = pitchNumber % 2 == 1;
        keyNumber = (pitchNumber + 1)/2;
    }
    
    MSG_TYPE_REG_Write(midiMsg[MIDI_MSG_TYPE]);
    NOTE_PITCH_REG_Write(keyNumber);
    NOTE_VEL_REG_Write(midiMsg[MIDI_NOTE_VELOCITY]);
    MSG_VALID_Write(1u);
    
//    if (midiMsg[MIDI_MSG_TYPE] == USB_MIDI_NOTE_ON) {
//        NOTE_NUM_REG_Write(keyNumber);
//        if (midiMsg[MIDI_NOTE_NUMBER] % 2u == 0) {
//            //LED_InA_Write(1u);
//        } else {
//            //LED_InB_Write(1u);
//        }
//    }
//    if (midiMsg[MIDI_MSG_TYPE] == USB_MIDI_NOTE_OFF || (midiMsg[MIDI_MSG_TYPE] == USB_MIDI_NOTE_ON && midiMsg[MIDI_NOTE_VELOCITY] == 0u)) {
//        NOTE_NUM_REG_Write(0u);
//        if (midiMsg[MIDI_NOTE_NUMBER] % 2u == 0) {
//            //LED_InA_Write(0u);
//        } else {
//            //LED_InB_Write(0u);
//        }
//    }
    
    inqFlagsOld = USB_MIDI1_InqFlags;
    cable = cable;
}    

/* [] END OF FILE */

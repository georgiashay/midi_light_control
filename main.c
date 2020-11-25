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

#define PLAYBACK_MODE           (0u)
#define PROGRAM_MODE            (1u)
#define PRESET_MODE             (2u)

#define KEY_LIGHT_0             (53u)
#define KEY_LIGHT_1             (55u)
#define KEY_LIGHT_2             (57u)
#define KEY_LIGHT_3             (59u)
#define KEY_LIGHT_4             (60u)
#define KEY_LIGHT_5             (62u)
#define KEY_LIGHT_6             (64u)
#define PRESET_BUTTON           (44u)
#define PLAY_PAUSE_BUTTON       (48u)
#define PROGRAM_BUTTON          (49u)
#define PREV_PRESET_BUTTON      (50u)
#define NEXT_PRESET_BUTTON      (51u)
#define UNUSED_BUTTON_1         (45u)
#define UNUSED_BUTTON_2         (46u)
#define UNUSED_BUTTON_3         (47u)

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

uint8 currentKeyNumber = 0u;

uint8 storedBrightnesses[8][7] = {
    {0u, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u, 0u, 0u}
};

uint8 mode = PROGRAM_MODE;
uint8 preset = 0;

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
    
    BRIGHTNESS_RAMP_Start();
    TRIANGLE_SEL_Start();
    RAMP_COMP_Start();

    LEDs_Out_1_Start();
        
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
        
        if (mode == PRESET_MODE) {
            STORED_BRIGHTNESS_1_Write(storedBrightnesses[preset][0]);
            STORED_BRIGHTNESS_2_Write(storedBrightnesses[preset][1]);
            STORED_BRIGHTNESS_3_Write(storedBrightnesses[preset][2]);
            STORED_BRIGHTNESS_4_Write(storedBrightnesses[preset][3]);
            STORED_BRIGHTNESS_5_Write(storedBrightnesses[preset][4]);
            STORED_BRIGHTNESS_6_Write(storedBrightnesses[preset][5]);
            STORED_BRIGHTNESS_7_Write(storedBrightnesses[preset][6]);
            LED_8_VALUE_Write(0u);
        } else {
            STORED_BRIGHTNESS_1_Write(preset == 0 ? 255u : 0u);
            STORED_BRIGHTNESS_2_Write(preset == 1 ? 255u : 0u);
            STORED_BRIGHTNESS_3_Write(preset == 2 ? 255u : 0u);
            STORED_BRIGHTNESS_4_Write(preset == 3 ? 255u : 0u);
            STORED_BRIGHTNESS_5_Write(preset == 4 ? 255u : 0u);
            STORED_BRIGHTNESS_6_Write(preset == 5 ? 255u : 0u);
            STORED_BRIGHTNESS_7_Write(preset == 6 ? 255u : 0u);
            LED_8_VALUE_Write(preset == 7 ? 1u : 0u);
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
    
    uint8 keyNumber;
    short isLightKey = 0u;
    
    short isNoteOn = (midiMsg[MIDI_MSG_TYPE] == USB_MIDI_NOTE_ON) && (midiMsg[MIDI_NOTE_VELOCITY] != 0u);
    
    if (isNoteOn) {
        uint8 readNumber = midiMsg[MIDI_NOTE_NUMBER];
        if (readNumber == 0u) {
            int hi = 0;
        }
    }
    switch (midiMsg[MIDI_NOTE_NUMBER]) {
        case KEY_LIGHT_0 :
            keyNumber = 0;
            isLightKey = 1u;
            break;
        case KEY_LIGHT_1 :
            keyNumber = 1;
            isLightKey = 1u;
            break;
        case KEY_LIGHT_2 :
            keyNumber = 2;
            isLightKey = 1u;
            break;
        case KEY_LIGHT_3 :
            keyNumber = 3;
            isLightKey = 1u;
            break;
        case KEY_LIGHT_4 :
            keyNumber = 4;
            isLightKey = 1u;
            break;
        case KEY_LIGHT_5 :
            keyNumber = 5;
            isLightKey = 1u;
            break;
        case KEY_LIGHT_6 :
            keyNumber = 6;
            isLightKey = 1u;
            break;
        case PRESET_BUTTON :
            if (isNoteOn == 1u) {
                if (mode == PRESET_MODE) {
                    mode = PROGRAM_MODE;
                } else if (mode == PROGRAM_MODE) {
                    mode = PRESET_MODE;
                }
            }
            break;
        case NEXT_PRESET_BUTTON :
            if (isNoteOn == 1u) {
                if (mode == PLAYBACK_MODE || mode == PROGRAM_MODE) {
                    preset++;
                    preset %= 8;
                }
            }
            break;
        case PREV_PRESET_BUTTON :
            if (isNoteOn == 1u) {
                if (mode == PLAYBACK_MODE || mode == PROGRAM_MODE) {
                    preset += 7;
                    preset %= 8;
                }
            }
            break;
        default:
            isLightKey = 0u;    
    }
    
    //uint8 oneHotKey = 1u << keyNumber;    
    
    if (mode == PRESET_MODE && isLightKey) {
        if (midiMsg[MIDI_MSG_TYPE] == USB_MIDI_NOTE_OFF || (midiMsg[MIDI_MSG_TYPE] == USB_MIDI_NOTE_ON && midiMsg[MIDI_NOTE_VELOCITY] == 0u)) {
            //MIDI_NOTES_REG_Write(oneHotKey);
            MIDI_BIN_REG_Write(0u);
            if (keyNumber == currentKeyNumber) {
                NOTE_ON_REG_Write(0u);
            }
           
            uint16 indexHigh = WAVE_STATUS_HIGH_Read();
            uint16 indexLow = WAVE_STATUS_HIGH_Read();
            uint16 waveIndex = (indexHigh << 8) + indexLow;
            // Fudge factor for signaling delays from Keyboard -> Computer -> PSoC -> Register
            waveIndex += 100;
            waveIndex %= 2048;
            storedBrightnesses[preset][keyNumber] = BRIGHTNESS_RAMP_wave1[waveIndex];

            // MIDI_NOTES_REG_Write(MIDI_NOTES_REG_Read() & ~oneHotKey);
        } else if (midiMsg[MIDI_MSG_TYPE] == USB_MIDI_NOTE_ON) {
            //MIDI_NOTES_REG_Write(0u);
            MIDI_BIN_REG_Write(keyNumber);
            NOTE_ON_REG_Write(1u);
            currentKeyNumber = keyNumber;
            //MIDI_NOTES_REG_Write(MIDI_NOTES_REG_Read() | oneHotKey);
        }
    }
    
    inqFlagsOld = USB_MIDI1_InqFlags;
    cable = cable;
}    

/* [] END OF FILE */

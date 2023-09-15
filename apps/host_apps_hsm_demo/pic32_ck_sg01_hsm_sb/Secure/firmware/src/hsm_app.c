/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <toolchain_specifics.h>
#include <xc.h>

#include "pic32ck2051sg01144.h"
#include "core_cm33.h"

#include "hsm_app.h"

#include "kitprotocol_parser/usb_hid/usb_hid.h"
#include "kitprotocol_parser/kit_protocol/kit_protocol_init.h"
#include "kitprotocol_parser/kitprotocol_parser_info.h"
#include "kitprotocol_parser/kit_hal_interface.h"
#include "boot.h"
#include "hsm_test_suite.h"
#define HID_REPORT_PACKET_SIZE_BYTES 64

static bool bootFailed = false;
bool readystate = false;

size_t usb_report_length = 0;
size_t current_response_location = 0;

uint8_t * transmitDataBuffer;
uint8_t transmitDataBuffer_sec[HID_REPORT_PACKET_SIZE_BYTES] CACHE_ALIGN;

typedef enum {
    APP_STATE_NOCHANGE = 3, APP_STATE_KIT_PROCESS, APP_STATE_HSM_COMMAND = 5, APP_STATE_MAIN = 7, APP_STATE_ERROR
} HSM_READY_NEXT_STATE;
#define KIT_NAME_HEADER "CryptoAuth Trust Platform "

#define STRING_EOL      "\r"

#define STRING_HEADER   "\r-- " KIT_NAME_HEADER " --\r\n" \
"-- Compiled: "__DATE__ " "__TIME__ " --\r\n" \
"-- Console log (115200-8-N-1) on USB --\r\n"STRING_EOL
uint8_t data_loc = 0;


/*****************************************************************/
/***************HSM Intialize********************************************/

/**********************************************************************/

void HSM_INIT() {
    
    char __attribute__((unused)) * initMessage =
            "\r\nApplication created " __DATE__ " " __TIME__ " initialized!\r\n";

    SYS_MESSAGE(initMessage);


#ifdef SECURE_BOOT
    SYS_MESSAGE("SECURE BOOT (from flash)\r\n");
#else
    SYS_MESSAGE("DUAL SESSION DEBUG BOOT\r\n");
#endif //SECURE_BOOT


#ifdef __ICCARM__
    //Enable global IRQ for CPU
    //cpu_irq_enable();

    // Enable PA11-PA03 pins as ouptputs for displaying HSM M0 printf messages
    //PORT->Group[0].DIR.reg = 0x00000FF8;
#endif //__ICCARM__

    //HSM Enable
    //--HSM CTRLA Reg:
    //    RUNSTDBY[6]: HSM main clock remains enabled in standby mode.
    //    PRIV(2)    : Host Interface registers only accessible in privileged mode.
    //    ENABLE(1)   : HSM main clock Enable bit (1)
    HSM_REGS->HSM_CTRLA = HSM_CTRLA_ENABLE_Msk;
    SYS_MESSAGE("HSM Enabled\r\n");


    //RX Interrupts
    NVIC_ClearPendingIRQ(HSM_RXINT_IRQn);
    NVIC_SetPriority(HSM_RXINT_IRQn, 0x1);
    NVIC_EnableIRQ(HSM_RXINT_IRQn);

    //Clear COM Receive Data Buffer
    //--Set to 0 so the parsing can detect the EOS
    //--This happens at the end of every HSM command 
    for (int j = 0; j < sizeof (gRcvCmdStr); j++) {
        gRcvCmdStr[j] = 0;
    } //String termination char gRcvCnt = 0; gRcvCmdRdy = 0;    
    /* Place the App state machine in its initial state.*/


} //End APP_Initialize()



/*****************************************************************/
/***************LOAD HSM BOOT HEX********************************************/

/**********************************************************************/
void HSM_Boot_Firmware() {
    //APP_STATE_INIT_HSM
    int __attribute__((unused)) hsmStatus = HSM_REGS->HSM_STATUS;
    GetHsmStatus(&busy, &ecode, &sbs, &lcs, &ps);
    SYS_PRINT("Initial HSM Status: 0x%08x\r\n", hsmStatus);
    SYS_PRINT("    %s  ECODE: %s\r\n    SBS: %s  LCS: %s  PS: %s\r\n",
            busy ? "BUSY" : "NOT busy",
            ecodeStr[ecode], sbsStr[sbs], lcsStr[sbs], psStr[ps]);




#if defined(SECURE_BOOT)
    uint8_t hsmFirmware[1024];
    uint32_t * fwP = (uint32_t *) hsmFirmware;
    //char buff[7]="Hello";
    //HSM Firmware image is loaded via .hex project to flash
    uint8_t * hsmFirmwareLoc = (uint8_t *) HSM_FIRMWARE_INIT_ADDR; //0x0c05fc00 

    // Check to make sure Boot ROM is ready to accept commands
    SYS_MESSAGE("\r\nHSM FLASH BOOT: Waiting for HSM FIRMWARE to be ready\r\n");
    memcpy(hsmFirmware, hsmFirmwareLoc, sizeof (hsmFirmware)); //0x0c0107ec

    SYS_PRINT("HSM Firmware Metadata:  0x%08lx %08lx %08lx %08lx...\r\n",
            *fwP, *(fwP + 1), *(fwP + 2), *(fwP + 3));
    if (*fwP == 0xFFFFFFFF) {
        SYS_PRINT("HSM Firmware Not Loaded - run host_firmware_boot\r\n");
    }
    hsmFirmwareLoc = (uint8_t *) HSM_FIRMWARE_ADDR; // 0x0c060000;
    memcpy(hsmFirmware, hsmFirmwareLoc, sizeof (hsmFirmware));
    SYS_PRINT("         HSM Firmware:  0x%08lx %08lx %08lx %08lx...\r\n",
            *fwP, *(fwP + 1), *(fwP + 2), *(fwP + 3));

    SYS_MESSAGE("HSM Load Firmware...\r\n");

    HsmCmdBootLoadFirmware();
    SYS_MESSAGE("\r\n HSM LOAD Firmware Complete\r\n");
#endif//SECURE_BOOT
    SYS_MESSAGE("--Waiting for HSM to become OPERATIONAL\r\n...\r\n");

}

bool HSM_Wait() {
    //Check HSM Status
    //   ECODE[19:16]: Error Code (1)
    //       0111 = HSM firmware authentication failure.
    //       0110 = Host safe mode command.
    //       0101 = Safe mode tamper event.
    //       0100 = Unrecoverable fault.
    //       0011 = Uncorrectable local memory bit error.
    //       0010 = Secure flash integrity error.
    //       0001 = Self test failure.
    //       0000 = No error.
    //    SBS[14-12]: Secure Boot State bits
    //       101 = Passed - secure boot completed with all required software authentications passing.
    //       100 = Failed - secure boot failed due to a required software authentication failing.
    //       011 = Additional Authentication - host directed authentication in progress.
    //       010 = Boot Flash Authentication - boot flash authentication in progress.
    //       001 = Disabled - secure boot disabled.
    //       000 = Reset - secure boot state not determined.
    //    LCS[10-8]: Lifecycle State bits
    //       111-101 = Reserved
    //       100 = Secured - secured, secure flash holds data.
    //       011 = Open - not secured, secure flash holds data.
    //       010 = Erased - not secured, secure flash erased.
    //       001 = IC Manufacturing - not secured, secure flash available for test.
    //       000 = Reset - lifecycle state not determined.
    //    PS[6-4]: Processing State bits
    //       111-100 = Reserved
    //       011 = Safe Mode - limited functionality mode due to an error, sensitive data erased.
    //       010 = Operational - executing loaded operational firmware.
    //       001 = Boot - executing from boot ROM, ready to load operational firmware.
    //       000 = Reset.



    //Wait for HSM Ready and Operational Mode
    //  while((HSM_REGS->HSM_STATUS & HSM_STATUS_PS_Msk) != HSM_PS_OPERATIONAL);
    int __attribute__((unused)) hsmStatus = HSM_REGS->HSM_STATUS;
    HSM_READY_NEXT_STATE hsm_state;
    GetHsmStatus(&busy, &ecode, &sbs, &lcs, &ps);
    if (busy == false && ps == HSM_PS_OPERATIONAL) {
        hsmStatus = HSM_REGS->HSM_STATUS;
        SYS_PRINT("HSM Status: 0x%08x\r\n", hsmStatus);
        SYS_PRINT("    %s  ECODE: %s\r\n    SBS: %s  LCS: %s  PS: %s\r\n",
                busy ? "BUSY" : "NOT busy",
                ecodeStr[ecode], sbsStr[sbs], lcsStr[sbs], psStr[ps]);

#ifdef SECURE_BOOT
        //HSM Init (CMD_BOOT_SELF_TEST)
        SYS_MESSAGE
                ("\r\n---------------------------------------------------------");

        SYS_MESSAGE("HSM (CMD_BOOT_SELF_TEST)...\r\n");

        //Validate the flash image 
        RSP_DATA * rsp = 0;
        bootFailed = false;
        rsp = HsmCmdBootTestHashInit();
        if (rsp->resultCode != S_OK) {
            char * str;
            str = CmdResultCodeStr(rsp->resultCode);
            SYS_PRINT("HSM ROM Test FAIL: CMD_BOOT_SELF_TEST - %s\r\n",
                    str);
            bootFailed = true;
        }
        SYS_MESSAGE("HSM ROM Test: CMD_BOOT_SELF_TEST Successful.\r\n");
#else
        bootFailed = false;
#endif //SECURE_BOOT



        SYS_MESSAGE("\r\nRunning HSM MB Command Test Suite\r\n");
        
        hsm_state = true;

    } else {
        hsm_state = false; // APP_STATE_IDLE
    }
    return (hsm_state);

} //End case APP_STATE_IDLE:

void HSM_command() {
    int __attribute__((unused)) retVal = 0;
    int __attribute__((unused)) numDataBytes = 0;




#define VSSLOT  15 //NOTE: Slot Number to use. 0 and 255 are reserved
#define AESSLOT -1
    int __attribute__((unused)) hsmStatus = HSM_REGS->HSM_STATUS;
    GetHsmStatus(&busy, &ecode, &sbs, &lcs, &ps);
    SYS_PRINT("PreTEST HSM Status: 0x%08x\r\n", hsmStatus);
    SYS_PRINT("    %s  ECODE: %s\r\n    SBS: %s  LCS: %s  PS: %s\r\n",
            busy ? "BUSY" : "NOT busy",
            ecodeStr[ecode], sbsStr[sbs], lcsStr[lcs], psStr[ps]);
    SYS_MESSAGE("\r\n");

    hsm_test_suite(VSSLOT, AESSLOT);

    SYS_MESSAGE("\r\n");
    hsmStatus = HSM_REGS->HSM_STATUS;
    GetHsmStatus(&busy, &ecode, &sbs, &lcs, &ps);
    SYS_PRINT("PostTEST HSM Status: 0x%08x\r\n", hsmStatus);
    SYS_PRINT("    %s  ECODE: %s\r\n    SBS: %s  LCS: %s  PS: %s\r\n",
            busy ? "BUSY" : "NOT busy",
            ecodeStr[ecode], sbsStr[sbs], lcsStr[sbs], psStr[ps]);

    SYS_MESSAGE("\r\n**** COMPLETED HSM MB TEST Suite ****");

}


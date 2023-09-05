/**
 * \file
 * \brief Hardware abstraction layer HSM Mailbox interface.
 *
 * Prerequisite: add SERCOM SPI Master Interrupt support to application in Mplab Harmony 3
 *
 * \copyright (c) 2022 Microchip Technology Inc. and its subsidiaries.
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT,
 * SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE
 * OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
 * MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
 * FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL
 * LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED
 * THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR
 * THIS SOFTWARE.
 */

#include <string.h>
#include <stdio.h>

#include "definitions.h"
//#include "../utilities/crc/crc_engines.h"
//#include "../kit_protocol/kit_protocol_interpreter.h"
#include "hal_delay.h"
#include "hal_harmony.h"

#include "../kit_device_info.h"
#include "../kit_protocol/kit_protocol_status.h"

#include "kitprotocol_parser/kit_protocol/kit_protocol_utilities.h"
#include "kitprotocol_parser/kit_protocol/kit_protocol_interpreter.h"

//NOTE: This is HSM Mailbox Interface 
#include "hsm_command.h"
#include "vsm.h"

uint32_t CACHE_ALIGN inData[MAXDATAWORDS];
uint32_t CACHE_ALIGN outData[MAXDATAWORDS];

char kitCmdRsp[MAXRSPBYTES];

static void hsmOutputBufferInit();
static void hsmInputBufferInit();


/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a HSM device
 *
   @{ */

/** \brief initialize an HSM interface
 */
void hal_hsm_init() {
    hsmOutputBufferInit();
    hsmInputBufferInit();
}

/** \brief HAL implementation of HSM post init
 */
void hal_hsm_deinit() {
    //SYS_MESSAGE("HSM Device DeInit\r\n");
    // to do nothing
}

/** \brief send wake up token to CryptoAuth device
 * \param[in] device_addr   device address
 * \return KIT_STATUS_SUCCESS on success, otherwise an error code.
 */
enum kit_protocol_status hal_hsm_wake(uint32_t device_addr) {
    (void) device_addr;
    SYS_MESSAGE("HSM Device Wake\r\n");

    return KIT_STATUS_SUCCESS;
    //NOTE:  This is kind of irrelevant for HSM Mailbox
    //       --Always awake when loaded and in OP mode.
}

/** \brief send idle command to  CryptoAuth device
 * \param[in] device_addr   device address
 * \return KIT_STATUS_SUCCESS on success, otherwise an error code.
 */
enum kit_protocol_status hal_hsm_idle(uint32_t device_addr) {
    (void) device_addr;

    //NOTE:  This is kind of irrelevant for HSM Mailbox
    //       --Always awake when loaded and in OP mode, not sure it can idle.
    return KIT_STATUS_SUCCESS;
}

/** \brief send sleep command to  CryptoAuth device
 * \param[in] device_addr   device address
 * \return KIT_STATUS_SUCCESS on success, otherwise an error code.
 */
enum kit_protocol_status hal_hsm_sleep(uint32_t device_addr) {
    (void) device_addr;
    //NOTE:  This is kind of irrelevant for HSM Mailbox
    //       --Always awake when loaded and in OP mode, not sure it can sleep.
    return KIT_STATUS_SUCCESS;
}


//******************************************************************************
// hal_hsm_parse_kit_cmd(char * data, int dataLength, HalHsmCmd *cmd)
// Parse the T{PDS input data string in the form:
//
// group[##]cmd[##]slot[###)length[#####...]
//
// TODO:  These command tokens are variable with respect to the group and
//        cmd values and are specific to unencrypted raw data of
//        given length being written to the given slot, ie. the VSM_INPUT_DATA
//        HSM command.  The input data follows the HSM Command in hex format.
//        Other commands will be parse different command tokens and values
//        as given by the HSM command API library as currently implemented.
//
//******************************************************************************

int hal_hsm_parse_kit_cmd(char * data, int dataLength, HalHsmCmd *cmd) {

    static char *currentCmdLoc;
    static char *currentParamStart;
    static char *currentParamStop;
    static char cmdStr[MAXCMDSTR] = "";
    static uint16_t cmdLength;
    static bool eoc = false;

    static char hexStr[MAXHEXSTR];
    static uint16_t hexLength = 0;
    //static uint8_t  byteBuffer[MAXHEXSTR/2] = {0};

    cmd->inData = inData;
    cmd->outData = outData;
    cmd->kitCmdRsp = kitCmdRsp;
    data[dataLength] = '\0';
    cmd->group = CMD_INVALID;
    cmd->command = 0xFF;
    cmd->slotNum = 0;
    cmd->dataWords = 0;


    currentCmdLoc = data;
    eoc = false;
    while (!eoc) {
        int numBytes = 0;

        if (strlen(currentCmdLoc) == 0) {
            eoc = true;
            break;
        }

        currentParamStart = strchr(currentCmdLoc, HSM_PARAM_START_DELIMITER);
        currentParamStop = strchr(currentCmdLoc, HSM_PARAM_STOP_DELIMITER);
        cmdLength = currentParamStart - currentCmdLoc; //cccc[
        *currentParamStart = '\0';
        *currentParamStop = '\0';
        hexLength = currentParamStop - currentParamStart - 1; // [xxx]
        strncpy(cmdStr, currentCmdLoc, cmdLength);
        cmdStr[cmdLength] = '\0';
        hexStr[hexLength] = '\0';
        strncpy(hexStr, currentParamStart + 1, hexLength);

        if (*currentCmdLoc != 'd') {
            printf("HSM Token %s = %s\r\n", cmdStr, hexStr);
        }

        switch (*currentCmdLoc) {
                int byteLength;
                int dataLenStrBytes;

            case 'g': //group
            {
                byteLength = kit_protocol_convert_hex_to_binary(hexLength, (uint8_t *) hexStr);

                if (byteLength > 0 && byteLength < 2) {
                    cmd->group = hexStr[0];
                } else {
                    printf("HSM Cmd Group Parse Error (#bytes = %d)\r\n", byteLength);
                    return 1;
                }
            }
                break;

            case 'c': //cmd
            {
                byteLength = kit_protocol_convert_hex_to_binary(hexLength, (uint8_t *) hexStr);

                if (byteLength > 0 && byteLength < 2) {
                    cmd->command = hexStr[0];
                } else {
                    printf("HSM Cmd Spec Parse Error (#bytes = %d)\r\n", byteLength);
                    return 1;
                }
            }
                break;

            case 's': //slot
            {
                byteLength = kit_protocol_convert_hex_to_binary(hexLength, (uint8_t *) hexStr);

                if (byteLength > 0 && byteLength < 2) {
                    cmd->slotNum = hexStr[0];
                } else {
                    printf("HSM Cmd Slot Parse Error (#bytes = %d)\r\n", byteLength);
                    return 1;
                }
            }
                break;

            case 'l': //length in Words
            {
                //Convert hex parameter string to bytes
                //--return #bytes in parameter data string
                byteLength = kit_protocol_convert_hex_to_binary(hexLength, (uint8_t *) hexStr);

                if (byteLength > 0 && byteLength < 5) {
                    cmd->dataWords = hexStr[0];
                } else {
                    printf("HSM Cmd Data Parse Error (#bytes = %d)\r\n", byteLength);
                    return 1;
                }
            }
                break;

            case 'd': //data
            {

                cmd->inData = inData;
                cmd->outData = outData;

                //Convert hex parameter string to bytes
                //--return #bytes in parameter data string
                dataLenStrBytes = kit_protocol_convert_hex_to_binary(
                        hexLength, (uint8_t *) hexStr);

                if ((dataLenStrBytes > 0) && cmd->dataWords > 0 &&
                        (dataLenStrBytes == cmd->dataWords * 4)) {
                    memcpy((uint8_t *) cmd->inData, hexStr, dataLenStrBytes);
                } else {
                    printf("HSM Cmd Data Parse Error (#string %d != #data %d)\r\n",
                            dataLenStrBytes, byteLength);
                    return 1;
                }
            }
                break;

            default:
            {
                printf("Error - %d bytes\r\n", numBytes);
                return 1; //Error
            }
                break;

        } //End switch()

        currentCmdLoc = currentParamStop + 1;

    }

    return 0;

} //End hal_hsm_parse_kit_cmd()


//******************************************************************************
//******************************************************************************

enum kit_protocol_status hal_hsm_execute(HalHsmCmd *cmd,
        uint8_t *rsp,
        uint16_t *rspLength) {
    enum kit_protocol_status status = KIT_STATUS_SUCCESS;
    CmdResultCodes rc;

    if (cmd->group == CMD_VSM) //VSM Commands
    {
        if (cmd->command == CMD_VSM_INPUT_DATA) //VSM_INPUT_DATA
        {
            //static VSInputMetaData vsInputMeta;
            //static CmdVSMDataSpecificMetaData specMetaData;

            printf("\r\nVSM_INPUT_DATA COMMAND\r\n");

            //Specific RAW Meta Data (default))
            //specMetaData.rawMeta.v = 0;
            //specMetaData.rawMeta.s.length = (cmd->dataWords)*BYTES_PER_WORD; 

            //TODO:  Store Sym/Asym/Hash/HashIv 
            //specMetaData.symKeyMeta.keyType   = VSS_SK_AES
            //specMetaData.aesSkMeta.s.keyType  = VSS_SK_AES; 
            //specMetaData.aesSkMeta.s.aesType  = cmd->aesType;
            //specMetaData.aesSkMeta.s.keySize  = cmd->keySize; 
            //specMetaData.aesSkMeta.s.aesType  = VSS_SK_AES_ECB; 

            //Data Words for CMD_VSM_INPUT_DATA
            if (cmd->dataWords > 5) {
                //NOTE:  Any type of unencrypted key data allowed

                //VSS Meta Data Words
                //vsInputMeta.inputLength = (cmd->dataWords)*BYTES_PER_WORD; 
                //vsInputMeta.vsHeader    = 
                //vsInputMeta.validBefore = 0x00000000;
                //vsInputMeta.validAfter  = 0xFFFFFFFF;
                //vsInputMeta.dataSpecificMetaData = specMetaData.v; 

                //Writer the VS Input Metadata (four words) prior to key data
                rc = hal_vsm_input_data_execute(cmd, rsp, rspLength);
            } else {
                rc = KIT_STATUS_INVALID_PARAM;
            }
        } else if (cmd->command == CMD_VSM_OUTPUT_DATA) //VSM_OUTPUT_DATA
        {
            printf("\r\nVSM_OUTPUT_DATA COMMAND\r\n");

            rc = hal_vsm_output_data_execute(cmd, rsp, rspLength);
        } else if (cmd->command == CMD_VSM_DELETE_SLOT) {
            printf("\r\nVSM_DELETE_DATA COMMAND\r\n");
            rc = hal_vsm_delete_data_execute(cmd, rsp, rspLength);
        } else if (cmd->command == CMD_VSM_GET_SLOT_INFO) {
            printf("\r\nVSM_SLOT_INFO COMMAND\r\n");
            rc = hal_vsm_slot_info_execute(cmd, rsp, rspLength);
        } else {
            printf("HSM Command Not Implemented!!!\r\n");
            status = KIT_STATUS_COMMAND_NOT_VALID;
        }
    } else {
        printf("Invalid HSM Command\r\n");
        status = KIT_STATUS_COMMAND_NOT_VALID;
    }

    if (rc != S_OK && status == KIT_STATUS_SUCCESS) {
        status = KIT_STATUS_EXECUTION_ERROR;
    }

    return status;
} //End hal_hsm_execute()


//******************************************************************************
// Execute the VSM Input Data Command
//******************************************************************************

CmdResultCodes hal_vsm_input_data_execute(HalHsmCmd *cmd,
        uint8_t *rsp,
        uint16_t *rspLength) {
    CmdVSMDataSpecificMetaData specMetaData;
    CmdVSMSlotType slotType = CMD_VSS_RAW;
    CmdResultCodes rc;
    RSP_DATA * hsmRsp = &gRspData;

    //Print the Input Data
    //VsmInputDataInfo(cmd->inData, vsmInputParam1);
    //Specific RAW Meta Data (default))
    specMetaData.rawMeta.v = 0;
    specMetaData.rawMeta.s.length = (cmd->dataWords) * BYTES_PER_WORD;

    //TODO:  Store Keys/Hash/HashIv 
    //
    //slotType                          = cmd->slotType;
    //specMetaData.symKeyMeta.keyType   = VSS_SK_AES
    //specMetaData.aesSkMeta.s.keyType  = VSS_SK_AES; 
    //specMetaData.aesSkMeta.s.aesType  = cmd->aesType;
    //specMetaData.aesSkMeta.s.keySize  = cmd->keySize; 
    //specMetaData.aesSkMeta.s.aesType  = VSS_SK_AES_ECB; 

    //VSS Meta Data Words
    //if (cmd->dataWords > 4)
    //{
    //    vsInputMeta.inputLength          = cmd->inData[0];
    //    vsInputMeta.validBefore          = cmd->inData[1];
    //    vsInputMeta.validAfter           = cmd->inData[2];
    //    vsInputMeta.dataSpecificMetaData = cmd->inData[3];
    //}
    //Writer the VS Input Metadata (four words) prior to key data
    //vsmInputDataPtr[0] = (uint32_t) vsInputMeta.inputLength; 
    //vsmInputDataPtr[1] = (uint32_t) vsInputMeta.validBefore; 
    //vsmInputDataPtr[2] = (uint32_t) vsInputMeta.validAfter; 
    //vsmInputDataPtr[3] = (uint32_t) vsInputMeta.dataSpecificMetaData; 

    //Write the Slot with Raw Data Words
    hsmRsp = HsmCmdVsmInputDataUnencrypted(cmd->slotNum,
            cmd->inData,
            cmd->dataWords,
            slotType,
            specMetaData);
    rc = hsmRsp->resultCode;

    ((uint32_t *) rsp)[0] = rc;
    *rspLength = 4; //bytes

    return rc;

} //End hal_vsm_execute()


//******************************************************************************
// Execute the VSM Output Data Command
//******************************************************************************

CmdResultCodes hal_vsm_output_data_execute(HalHsmCmd *cmd,
        uint8_t *rsp,
        uint16_t *rspLength) {
    //CmdVSMDataSpecificMetaData specMetaData;
    CmdResultCodes rc;
    RSP_DATA * hsmRsp = &gRspData;
    VSMetaData vsMetaData;
    uint32_t slotSizeBytes;
    static int dataWords;

    dataWords = cmd->dataWords;

    //Check Slot is NOT Empty
    rc = HsmCmdVsmGetSlotInfo(cmd->slotNum, &vsMetaData, &slotSizeBytes);
    if (rc == E_VSEMPTY) {
        SYS_PRINT(
                "VSM FAIL: !!!CMD_VSM_OUTPUT_DATA TEST ABORT - SLOT #%d EMPTY!!!\r\n",
                cmd->slotNum);
    } else {

        //TODO:  Use the cmd->numDataWords
        if (vsMetaData.vsHeader.s.vsSlotType == VSS_RAW) {
            dataWords = vsMetaData.dataSpecificMetaData / BYTES_PER_WORD +
                    VSS_META_WORDS;
            if (vsMetaData.dataSpecificMetaData != slotSizeBytes) {
                SYS_PRINT("WARNING:  Raw Meta Bytes(0x%02x) != slotSizeBytes(0x%2x)\r\n",
                        vsMetaData.dataSpecificMetaData, (uint16_t) slotSizeBytes);
            }
        } else {
            dataWords = slotSizeBytes / BYTES_PER_WORD;
        }

        //Initialize dmaDataOut
        //uint8_t * dmaDataOutBytePtr = (uint8_t *) cmd->outData;

        //for (int i; i <MAXDATAWORDS*BYTES_PER_WORD; i++)
        //{
        //    dmaDataOutBytePtr[i] = i%0xFF;
        //}
        SYS_PRINT("Slot 15 Data Words = %d (%d Bytes)",
                dataWords, dataWords * BYTES_PER_WORD);

        hsmRsp = HsmCmdVsmOutputDataUnencrypted(cmd->slotNum,
                cmd->outData,
                &dataWords,
                MAXDATAWORDS);

        rc = hsmRsp->resultCode;

        ((uint32_t *) rsp)[0] = rc;
        ((uint32_t *) rsp)[1] = dataWords*BYTES_PER_WORD;
        memcpy((rsp + 8), cmd->outData, dataWords * BYTES_PER_WORD);

        *rspLength = (uint16_t) dataWords * BYTES_PER_WORD + 8;
    }

    return rc;
} //End hal_vsm_output_data_execute()


//******************************************************************************
// Execute the VSM Delete Data Command
//******************************************************************************

CmdResultCodes hal_vsm_delete_data_execute(HalHsmCmd *cmd,
        uint8_t *rsp,
        uint16_t *rspLength) {
    RSP_DATA * hsmRsp = &gRspData;
    CmdResultCodes rc;
    hsmRsp = HsmCmdVsmDeleteSlot(cmd->slotNum);

    rc = hsmRsp->resultCode;

    *(uint32_t *) rsp = rc;
    *rspLength = 4; //bytes

    return rc;
} //End hal_vsm_execute()


//******************************************************************************
// Execute the VSM Slot Info Command
//******************************************************************************

CmdResultCodes hal_vsm_slot_info_execute(HalHsmCmd *cmd,
        uint8_t *rsp,
        uint16_t *rspLength) {
    CmdResultCodes rc;
    static VSMetaData CACHE_ALIGN vsMetaData;
    uint32_t slotSizeBytes;
    //uint32_t          sMeta;
    //char *            bytePtr;
    uint32_t * wordPtr;
    //static VSHeader   vsHeader;

    rc = HsmCmdVsmGetSlotInfo(cmd->slotNum, &vsMetaData, &slotSizeBytes);

    ((uint32_t *) rsp)[0] = rc;
    if (gHsmCmdResp.numResultWords == 1) {
        ((uint32_t *) rsp)[1] = slotSizeBytes;
    } else ((uint32_t *) rsp)[1] = 0x00000000;
    *rspLength = 8; //bytes


    //sMeta   = sizeof(vsMetaData);
    //bytePtr = (char *) &vsMetaData;
    wordPtr = ((uint32_t *) rsp) + 2;

    //Copy dma result to cmd response
    //memcpy(&rsp[2],(uint8_t *)&vsMetaData,sizeof(VSMetaData));
    for (int i = 0; i < VSS_META_WORDS; i++) {
        *(wordPtr + i) = vsmSlotInfoOut[i];
    }
    *rspLength += VSS_META_WORDS*BYTES_PER_WORD;

    return rc;
} //End hal_vsm_slot_info_execute()

/** \brief Implementation of talk command
 * \param[in] device_addr   device address
 * \param[inout] data       As input, reference to txdata (send command)
 *                          As output, reference to rxdata (receive response)
 * \param[inout] length     As input, the size of the txdata buffer.
 *                          As output, the number of bytes received.
 * \return KIT_STATUS_SUCCESS on success, otherwise an error code.
 */
enum kit_protocol_status hal_hsm_talk(uint32_t device_addr,
        uint8_t *data,
        uint16_t *dataLength) {
    static HalHsmCmd cmd;
    enum kit_protocol_status __attribute__((unused)) status = KIT_STATUS_COMM_FAIL;

    //(void)device_addr;
    //(void)*data;
    //(void)*dataLength;

    SYS_PRINT("HSM Device TALK(CMD) \"%s\"\r\n", (char *) data);

    //Parse the HSM command 
    //NOTE:  The data buffer holds the HSM command string with hex parameter
    //       values 
    //       The data[<values>] field values are returned to buffer cmd->inData
    //       with length cmd->dataWords along with other HSM command parameters.
    hal_hsm_parse_kit_cmd((char *) data, *dataLength, &cmd);

    //Execute the HSM command using the HSM MB API Command Library function
    //NOTE:  The data buffer holds the binary response transmitted back to TPDS
    status = hal_hsm_execute(&cmd, data, dataLength);

    return status;

} //End has_hsm_talk())

/** \brief hal implementation of hsm send over harmony
 * \param[in] device_addr   device_address
 * \param[in] txdata        pointer to space to bytes to send
 * \param[in] txlength      number of bytes to send
 * \return KIT_STATUS_SUCCESS on success, otherwise an error code.
 */
enum kit_protocol_status hal_hsm_send(uint32_t device_addr,
        uint8_t *txdata,
        uint16_t* txlength) {
    enum kit_protocol_status status = KIT_STATUS_COMM_FAIL;

    //NOTE:  HSM Mailbox Command function from the hsm_command lib
    status = KIT_STATUS_COMMAND_NOT_SUPPORTED;
    return status;
}

/** \brief HAL implementation of HSM receive function 
 * \param[in]    device_addr   device address
 * \param[out]   rxdata        Data received will be returned here.
 * \param[inout] rxlength      As input, the size of the rxdata buffer.
 *                             As output, the number of bytes received.
 * \return KIT_STATUS_SUCCESS on success, otherwise an error code.
 */
enum kit_protocol_status hal_hsm_receive(uint32_t device_addr,
        uint8_t *rxdata,
        uint16_t *rxlength) {
    enum kit_protocol_status status = KIT_STATUS_COMM_FAIL;
    //uint16_t rxdata_max_size = 1024;
    //uint16_t word_addr_size = 1;
    uint16_t read_length = 2;
    //uint8_t  word_address;


    if ((NULL == rxlength) || (NULL == rxdata)) {
        return KIT_STATUS_INVALID_PARAM;
    }

    //NOTE:  HSM Mailbox Command response function from the hsm_command lib

    *rxlength = 0;

    *rxlength = read_length;

    status = KIT_STATUS_COMMAND_NOT_SUPPORTED;
    return status;
}

/** \brief The function discover devices connected on SPI bus
 * \param[out]   device_list   discovered device info is returned here
 * \param[out]   dev_count     number of devices discovered count is returned here
 * \return KIT_STATUS_SUCCESS on success, otherwise an error code.
 */
void hal_hsm_discover(device_info_t* device_list, uint8_t* dev_count) {
    //enum kit_protocol_status ret_code;

    //if ((KIT_STATUS_SUCCESS == 
    //    (ret_code = 
    //        hsm_discover(0x00, 
    //                     device_list->dev_rev, 
    //                     &(device_list->device_type)))))
    if (KIT_STATUS_SUCCESS == check_hsm_ready()) {
        device_list->address = 0x00;
        device_list->bus_type = DEVKIT_IF_HSM_MB;
        device_list->header = HSM_MB_HEADER;
        device_list->device_type = DEVICE_TYPE_HSM;
        //device_list->dev_rev    = 0; //Revision (8 bytes)
        device_list->is_no_poll = 1; //No polling after command

        (*dev_count)++;
        if (*dev_count == MAX_DISCOVER_DEVICES) {
            return;
        }
    }
}

//******************************************************************************
// check_hsm_ready()
//******************************************************************************

enum kit_protocol_status check_hsm_ready() {
    //int hsmStatus = HSM_REGS->HSM_STATUS; 
    bool busy;

    //Wait for HSM Ready and Operational Mode
    GetHsmStatus(&busy, &ecode, &sbs, &lcs, &ps);
    if (busy == false && ps == HSM_PS_OPERATIONAL) {
        //gPrintStr = messageBuffer[INDX_HSM_MB_READY];   //HSM and HOST Ready
        //SYS_PRINT("%s", gPrintStr); //HSM HOST Ready

        return KIT_STATUS_SUCCESS;
    } else {
        return KIT_STATUS_COMM_FAIL;
    }
} //End check_hsm_ready()

/** \brief Wrapper delay function
 * \param[in]    delay_in_ms  Delay count in millisecond
 */
void kit_delay_ms(uint32_t delay_in_ms) {
    atca_delay_ms(delay_in_ms);
}

static void hsmOutputBufferInit() {
    uint8_t * dmaDataOutBytePtr = (uint8_t *) outData;

    for (int i = 0; i < MAXDATAWORDS * BYTES_PER_WORD; i++) {
        dmaDataOutBytePtr[i] = i % 0xFF;
    }
}

static void hsmInputBufferInit() {
    uint8_t * dmaDataInputPtr = (uint8_t *) inData;

    for (int i = 0; i < MAXDATAWORDS * BYTES_PER_WORD; i++) {
        dmaDataInputPtr[i] = i % 0xFF;
    }
} 
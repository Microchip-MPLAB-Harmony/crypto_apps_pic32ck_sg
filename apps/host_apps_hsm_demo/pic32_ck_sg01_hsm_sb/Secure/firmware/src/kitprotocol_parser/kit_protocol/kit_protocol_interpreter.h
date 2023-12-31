/**
 * \file
 *
 * \brief  KIT protocol intepreter
 *
 * \copyright (c) 2018 Microchip Technology Inc. and its subsidiaries.
 *            You may use this software and any derivatives exclusively with
 *            Microchip products.
 *
 * \page License
 *
 * (c) 2018 Microchip Technology Inc. and its subsidiaries. You may use this
 * software and any derivatives exclusively with Microchip products.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIPS TOTAL LIABILITY ON ALL CLAIMS IN
 * ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
 * TERMS.
 */

#ifndef KIT_PROTOCOL_INTERPRETER_H
#define KIT_PROTOCOL_INTERPRETER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "kit_protocol_api.h"
#include "kit_protocol_status.h"
#include "../kit_device_info.h"

extern device_type_t g_selected_device_type;
// Set the packing alignment for the structure members
#pragma pack(push, 1)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    /**
     * \brief The supported Kit Protocol commands.
     */
    enum kit_protocol_command {
        KIT_COMMAND_UNKNOWN = 0xFF,

        KIT_COMMAND_BOARD = 0x00,
        KIT_COMMAND_BOARD_VERSION = 0x01,
        KIT_COMMAND_BOARD_FIRMWARE = 0x02,
        KIT_COMMAND_BOARD_GET_DEVICE = 0x03,
        KIT_COMMAND_BOARD_GET_DEVICES = 0x04,
        KIT_COMMAND_BOARD_DISCOVER = 0x05,
        KIT_COMMAND_BOARD_GET_LAST_ERROR = 0x06,
        KIT_COMMAND_BOARD_APPLICATION = 0x07,
        KIT_COMMAND_BOARD_POLLING = 0x08,

        KIT_COMMAND_DEVICE = 0x30,
        KIT_COMMAND_DEVICE_IDLE = 0x31,
        KIT_COMMAND_DEVICE_SLEEP = 0x32,
        KIT_COMMAND_DEVICE_WAKE = 0x33,
        KIT_COMMAND_DEVICE_SEND = 0x34,
        KIT_COMMAND_DEVICE_RECEIVE = 0x35,
        KIT_COMMAND_DEVICE_TALK = 0x36,
        KIT_COMMAND_MEMORY_WRITE = 0x37,
        KIT_COMMAND_MEMORY_READ = 0x38,

#ifndef KIT_PROTOCOL_NO_LEGACY_SUPPORT
        KIT_COMMAND_PHYSICAL = 0xF0,
        KIT_COMMAND_PHYSICAL_SELECT = 0xF1
#endif // KIT_PROTOCOL_NO_LEGACY_SUPPORT
    };

#define KIT_DEVICE_HANDLE_SIZE  (8)  //! Size of the device handle ASCII hex string
#define KIT_DEVICE_INDEX_SIZE  (2)   //! Size of the device index ASCII hex string
#define KIT_COMMAND_SIZE_MIN   (3)   //! Minimum size of a command section name (Ex. v())

    /**
     * \brief Kit Protocol Interpreter interface to the application.
     */
    struct kit_interpreter_interface {
        // Board Kit Protocol message functions
        enum kit_protocol_status(*board_get_version)(uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*board_get_firmware)(uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*board_get_device)(uint32_t device_handle, uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*board_get_devices)(uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*board_discover)(bool enabled);
        enum kit_protocol_status(*board_get_last_error)(uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*board_application)(uint32_t device_handle, uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*board_polling)(bool enabled);

        // Device Kit Protocol message functions
        enum kit_protocol_status(*device_idle)(uint32_t device_handle);
        enum kit_protocol_status(*device_sleep)(uint32_t device_handle);
        enum kit_protocol_status(*device_wake)(uint32_t device_id, uint8_t * message, uint16_t * length);
        enum kit_protocol_status(*device_receive)(uint32_t device_handle, uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*device_send)(uint32_t device_handle, uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*device_talk)(uint32_t device_handle, uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*device_mem_write)(uint32_t device_handle, uint8_t *message, uint16_t *message_length);
        enum kit_protocol_status(*device_mem_read)(uint32_t device_handle, uint8_t *message, uint16_t *message_length);
    };

    /** \brief Initialize the Kit Protocol Interpreter library.
     *
     *  \param[in]    interface              references to the Kit interpreter interface
     *
     *  \param[out]   None
     *
     *  \param[inout] None
     *
     *  \return KIT_STATUS_SUCCESS on success, otherwise error code
     */
    enum kit_protocol_status kit_interpreter_init(struct kit_interpreter_interface *interface);

    /** \brief Get the selected device - device address
     *
     *  \param[in]    None
     *
     *  \param[out]   None
     *
     *  \param[inout] None
     *
     *  \return the device address
     */
    uint8_t kit_interpreter_get_selected_device_handle(void);

    /** \brief The function select the hal interface based on device interface selected
     *
     *  \param[in]    handle                 references to device address
     *
     *  \param[out]   None
     *
     *  \param[inout] None
     *
     *  \return None
     */
    void kit_interpreter_set_selected_hsm_device(void);

    /** \brief The function select the HSM MB Interface based on HSM device
     *
     *  \param[in]    none
     *
     *  \param[out]   None
     *
     *  \param[inout] None
     *
     *  \return None
     */
    void kit_interpreter_set_selected_device_handle(const uint32_t handle);

    /** \brief Get the Kit Protocol maximum message length
     *
     *  \param[in]    None
     *
     *  \param[out]   None
     *
     *  \param[inout] None
     *
     *  \return kit protocol maximum message length
     */
    uint16_t kit_interpreter_get_max_message_length(void);

    /** \brief The function checks the message is completely recieved or not
     *
     *  \param[in]    message                references to command message
     *                message_length         references to command message length
     *
     *  \param[out]   None
     *
     *  \param[inout] None
     *
     *  \return True on message received completely or false
     */
    bool kit_interpreter_message_complete(const char *message, uint16_t message_length);

    /** \brief The function interprets the received message and handle it
     *
     *  \param[in]    message                references to command message
     *                message_length         references to command message length
     *
     *  \param[out]   None
     *
     *  \param[inout] None
     *
     *  \return KIT_STATUS_SUCCESS on success, otherwise an error code
     */
    enum kit_protocol_status kit_interpreter_handle_message(char *message, uint16_t *message_length);

#ifdef __cplusplus
}
#endif // __cplusplus

// unset the packing alignment for the structure members
#pragma pack(pop)

#endif // KIT_PROTOCOL_INTERPRETER_H

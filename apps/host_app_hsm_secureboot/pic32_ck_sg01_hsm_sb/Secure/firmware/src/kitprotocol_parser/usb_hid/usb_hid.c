#include "usb_hid.h"


// Use the KIT PROTOCOL message delimiter as the USB message completed delimiter
#define USB_MESSAGE_DELIMITER  KIT_MESSAGE_DELIMITER

#define UDI_HID_REPORT_OUT_SIZE (64)

// Host buffer
uint8_t g_usb_buffer[KIT_MESSAGE_SIZE_MAX];
uint16_t g_usb_buffer_length;
uint8_t g_usb_message_received = false;
uint16_t g_message_length = 0;

bool response_available = false;
uint16_t response_len = 0;

void usb_hid_init(void) {
    //Nothing to do
}

bool usb_hid_report_out_callback(uint8_t *report) {
    bool status;
    //printchar("Report ", report, '\n', UDI_HID_REPORT_OUT_SIZE);

    // Handle incoming USB reports
    for (uint32_t index = 0; index < UDI_HID_REPORT_OUT_SIZE - 1; index++) {
        // Save the incoming USB packet
        g_usb_buffer[g_usb_buffer_length] = report[index];
        g_usb_buffer_length++;

        //Check if the Complete USB Command message was received
        if (report[index] == USB_MESSAGE_DELIMITER) {
            g_usb_message_received = true;
            g_message_length = index; //report length not including \n
            g_usb_buffer[g_usb_buffer_length] = '\0';
            SYS_PRINT("    CMD: %s\r\n", g_usb_buffer);
            break;
        }
        status = true;
    }
    if (g_usb_message_received == false) {
        status = false;

    }
    return (status); /* Place a new read request. */
}

uint8_t usb_send_message_response(uint8_t *response, uint16_t response_length) {
    response_len = response_length; // Update the response length
    response_available = true; //set response available to transmit to host 
    g_usb_message_received = false; //set message_received to false to indicate to kitprotocol

    return 0;
}
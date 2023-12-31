/**
 * \file
 *
 * \brief HAL delay related functionality implementation.
 *
 * Copyright (c) 2014-2022 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include "hal_delay.h"
#include "hal_core_m0plus_base.h"


/**
 * \brief Driver version
 */
#define DRIVER_VERSION 0x00000001u

/**
 * \brief The pointer to a hardware instance used by the driver.
 */
static void *hardware;

/**
 * \brief Initialize Delay driver
 */
void atca_delay_init(void *const hw) {
    _delay_init(hardware = hw);
}

/**
 * \brief Perform delay in us
 */
void atca_delay_us(const uint32_t us) {
    _delay_cycles(hardware, _get_cycles_for_us(us));
}

/**
 * \brief Perform delay in ms
 */
void atca_delay_ms(const uint32_t ms) {
    _delay_cycles(hardware, _get_cycles_for_ms(ms));
}

/**
 * \brief Retrieve the current driver version
 */
uint32_t atca_delay_get_version(void) {
    return DRIVER_VERSION;
}

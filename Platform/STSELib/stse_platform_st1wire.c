/******************************************************************************
 * \file	stse_platform_st1wire_static.c
 * \brief   STSecureElement Services platform (source)
 * \author  STMicroelectronics - CS application team
 *
 ******************************************************************************
 * \attention
 *
 * <h2><center>&copy; COPYRIGHT 2022 STMicroelectronics</center></h2>
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "core/stse_platform.h"
#include "drivers/st1wire/st1wire.h"
#include <stdlib.h>

#ifdef STSE_CONF_USE_ST1WIRE

#define STSE_PLATFORM_ST1WIRE_BUFFER_LENGTH 752U
static PLAT_UI8 st1wire_buffer[STSE_PLATFORM_ST1WIRE_BUFFER_LENGTH];
static PLAT_UI16 st1wire_frame_size;
static volatile PLAT_UI16 st1wire_frame_offset;

stse_ReturnCode_t stse_platform_st1wire_init(PLAT_UI8 busID) {
    st1wire_ReturnCode_t ret;
    (void)busID;

    ret = st1wire_init();
    if (ret == ST1WIRE_OK) {
        return STSE_OK;
    } else {
        return STSE_PLATFORM_SERVICES_INIT_ERROR;
    }
}

stse_ReturnCode_t stse_platform_st1wire_wake(PLAT_UI8 busID,
                                             PLAT_UI8 devAddr,
                                             PLAT_UI16 speed) {
    (void)devAddr;
    (void)speed;

    st1wire_platform_wake(busID);

    return (STSE_OK);
}

stse_ReturnCode_t stse_platform_st1wire_send_start(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI16 FrameLength) {
    (void)busID;
    (void)devAddr;
    (void)speed;

    /* - Check buffer overflow */
    if (FrameLength > sizeof(st1wire_buffer) / sizeof(st1wire_buffer[0])) {
        return STSE_PLATFORM_BUFFER_ERR;
    }

    st1wire_frame_size = FrameLength;
    st1wire_frame_offset = 0;

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_st1wire_send_continue(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pData,
    PLAT_UI16 data_size) {
    (void)busID;
    (void)devAddr;
    (void)speed;

    if (data_size != 0) {
        if (pData == NULL) {
            memset((st1wire_buffer + st1wire_frame_offset), 0x00, data_size);
        } else {
            memcpy((st1wire_buffer + st1wire_frame_offset), pData, data_size);
        }
        st1wire_frame_offset += data_size;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_st1wire_send_stop(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pData,
    PLAT_UI16 data_size) {
    stse_ReturnCode_t ret;

    ret = stse_platform_st1wire_send_continue(
        busID,
        devAddr,
        speed,
        pData,
        data_size);

    /* - Send ST1Wire frame buffer */
    if (ret == STSE_OK) {
        ret = (stse_ReturnCode_t)st1wire_SendFrame(
            busID,
            devAddr,
            speed,
            st1wire_buffer,
            st1wire_frame_size);
    }

    if (ret != STSE_OK) {
        ret = STSE_PLATFORM_BUS_ACK_ERROR;
    }

    return ret;
}

stse_ReturnCode_t stse_platform_st1wire_receive_start(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI16 frameLength) {
    PLAT_I8 ret = 0;

    /* Check read buffer overflow */
    if (frameLength > STSE_PLATFORM_ST1WIRE_BUFFER_LENGTH) {
        return STSE_PLATFORM_BUFFER_ERR;
    }

    /* - Read full Frame */
    ret = st1wire_ReceiveFrame(
        busID,
        devAddr,
        speed,
        st1wire_buffer,
        &st1wire_frame_size);

    if (ret != 0) {
        return STSE_PLATFORM_BUS_ACK_ERROR;
    }

    /* - Reset read offset */
    st1wire_frame_offset = 0;

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_st1wire_receive_continue(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pData,
    PLAT_UI16 data_size) {
    (void)busID;
    (void)devAddr;
    (void)speed;

    if (pData != NULL) {

        /* Check read overflow */
        if ((st1wire_frame_size - st1wire_frame_offset) < data_size) {
            return STSE_PLATFORM_BUFFER_ERR;
        }

        /* Copy buffer content */
        memcpy(pData, (st1wire_buffer + st1wire_frame_offset), data_size);
    }

    st1wire_frame_offset += data_size;

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_st1wire_receive_stop(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pData,
    PLAT_UI16 data_size) {
    /*- Copy last element*/
    stse_platform_st1wire_receive_continue(busID, devAddr, speed, pData, data_size);

    st1wire_frame_offset = 0;

    return (STSE_OK);
}
#endif

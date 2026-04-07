/*!
 * @file      modem_e_hal_impl.c
 *
 * @brief     Hardware Abstraction Layer (HAL) implementation for modem-e
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2024. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdint.h>
#include "modem_e_hal.h"
#include "modem_e_modem_hal.h"
#include "modem_e_system.h"
#include "modem_e_board.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define MODEM_E_RESET_TIMEOUT 3000

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*!
 * @brief modem-e reset timeout flag
 */
static bool modem_e_reset_timeout = false;

/*!
 * @brief Timer to handle the scan timeout
 */
static timer_event_t modem_e_reset_timeout_timer;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*!
 * @brief Function to wait that the modem-e transceiver busy line raise to high
 *
 * @param [in] context Chip implementation context
 * @param [in] timeout_ms timeout in millisec before leave the function
 *
 * @returns modem_e_hal_status_t
 */
static modem_e_hal_status_t modem_e_hal_wait_on_busy( const void* context, uint32_t timeout_ms );

/*!
 * @brief Function to wait that the modem-e busy line fall to low
 *
 * @param [in] context Chip implementation context
 * @param [in] timeout_ms timeout in millisec before leave the function
 *
 * @returns modem_e_hal_status_t
 */
static modem_e_modem_hal_status_t modem_e_modem_hal_wait_on_busy( const void* context, uint32_t timeout_ms );

/*!
 * @brief Function to wait that the modem-e busy line raise to high
 *
 * @param [in] context Chip implementation context
 * @param [in] timeout_ms timeout in millisec before leave the function
 *
 * @returns modem_e_hal_status_t
 */
static modem_e_modem_hal_status_t modem_e_modem_hal_wait_on_unbusy( const void* context, uint32_t timeout_ms );

/*!
 * @brief Function executed on modem-e reset timeout event
 */
static void on_modem_e_reset_timeout_event( void* context );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/*!
 * @brief modem_e_modem_hal.h API implementation
 */

modem_e_modem_hal_status_t modem_e_modem_hal_write( const void* context, const uint8_t* command,
                                                  const uint16_t command_length, const uint8_t* data,
                                                  const uint16_t data_length )
{
    if( modem_e_modem_hal_wakeup( context ) == MODEM_E_MODEM_HAL_STATUS_OK )
    {
        uint8_t                   crc          = 0;
        uint8_t                   crc_received = 0;
        modem_e_modem_hal_status_t status;

        /* NSS low */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        /* Send CMD */
        for( uint16_t i = 0; i < command_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, command[i] );
        }
        /* Send Data */
        for( uint16_t i = 0; i < data_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, data[i] );
        }
        /* Compute and send CRC */
        crc = modem_e_modem_compute_crc( 0xFF, command, command_length );
        crc = modem_e_modem_compute_crc( crc, data, data_length );
        /* Send CRC */
        hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, crc );

        /* NSS high */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        /* Wait on busy pin up to 1000 ms */
        if( modem_e_modem_hal_wait_on_busy( context, 1000 ) != MODEM_E_MODEM_HAL_STATUS_OK )
        {
            return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
        }

        /* Send dummy byte to retrieve RC & CRC */

        /* NSS low */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        /* read RC */
        status       = ( modem_e_modem_hal_status_t ) hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );
        crc_received = hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );
        /* Compute response crc */
        crc = modem_e_modem_compute_crc( 0xFF, ( uint8_t* ) &status, 1 );

        /* NSS high */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        if( crc != crc_received )
        {
            /* change the response code */
            status = MODEM_E_MODEM_HAL_STATUS_BAD_FRAME;
        }

        /* Wait on busy pin up to 1000 ms */
        if( modem_e_modem_hal_wait_on_unbusy( context, 1000 ) != MODEM_E_MODEM_HAL_STATUS_OK )
        {
            return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
        }

        return status;
    }

    return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
}

modem_e_modem_hal_status_t modem_e_modem_hal_write_without_rc( const void* context, const uint8_t* command,
                                                             const uint16_t command_length, const uint8_t* data,
                                                             const uint16_t data_length )
{
    if( modem_e_modem_hal_wakeup( context ) == MODEM_E_MODEM_HAL_STATUS_OK )
    {
        uint8_t                   crc    = 0;
        modem_e_modem_hal_status_t status = MODEM_E_MODEM_HAL_STATUS_OK;

        /* NSS low */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        /* Send CMD */
        for( uint16_t i = 0; i < command_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, command[i] );
        }
        /* Send Data */
        for( uint16_t i = 0; i < data_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, data[i] );
        }
        /* Compute and send CRC */
        crc = modem_e_modem_compute_crc( 0xFF, command, command_length );
        crc = modem_e_modem_compute_crc( crc, data, data_length );
        /* Send CRC */
        hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, crc );

        /* NSS high */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        return status;
    }

    return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
}

modem_e_modem_hal_status_t modem_e_modem_hal_write_read( const void* context, const uint8_t* command, uint8_t* data,
                                                         const uint16_t data_length )
{
    if( modem_e_modem_hal_wakeup( context ) == MODEM_E_MODEM_HAL_STATUS_OK )
    {
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        for( uint16_t i = 0; i < data_length; i++ )
        {
            data[i] = hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, command[i] );
        }

        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        if( modem_e_modem_hal_wait_on_busy( context, 1000 ) != MODEM_E_MODEM_HAL_STATUS_OK )
        {
            return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
        }

        return MODEM_E_MODEM_HAL_STATUS_OK;
    }
    return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
}

modem_e_modem_hal_status_t modem_e_modem_hal_read( const void* context, const uint8_t* command,
                                                 const uint16_t command_length, uint8_t* data,
                                                 const uint16_t data_length )
{
    if( modem_e_modem_hal_wakeup( context ) == MODEM_E_MODEM_HAL_STATUS_OK )
    {
        uint8_t                   crc          = 0;
        uint8_t                   crc_received = 0;
        modem_e_modem_hal_status_t status;

        /* NSS low */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        /* Send CMD */
        for( uint16_t i = 0; i < command_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, command[i] );
        }
        /* Compute and send CRC */
        crc = modem_e_modem_compute_crc( 0xFF, command, command_length );
        /* Send CRC */
        hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, crc );

        /* NSS high */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        /* Wait on busy pin up to 1000 ms */
        if( modem_e_modem_hal_wait_on_busy( context, 1000 ) != MODEM_E_MODEM_HAL_STATUS_OK )
        {
            return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
        }

        /* Send dummy byte to retrieve RC & CRC */

        /* NSS low */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        /* read RC */
        status = ( modem_e_modem_hal_status_t ) hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );
        if( status == MODEM_E_MODEM_HAL_STATUS_OK )
        {
            for( uint16_t i = 0; i < data_length; i++ )
            {
                data[i] = hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );
            }
        }

        crc_received = hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );

        /* NSS high */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        /* Wait on busy pin up to 1000 ms */
        if( modem_e_modem_hal_wait_on_unbusy( context, 1000 ) != MODEM_E_MODEM_HAL_STATUS_OK )
        {
            return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
        }

        /* Compute response crc */
        crc = modem_e_modem_compute_crc( 0xFF, ( uint8_t* ) &status, 1 );
        if( status == MODEM_E_MODEM_HAL_STATUS_OK )
        {
            crc = modem_e_modem_compute_crc( crc, data, data_length );
        }

        if( crc != crc_received )
        {
            /* change the response code */
            status = MODEM_E_MODEM_HAL_STATUS_BAD_FRAME;
        }
        return status;
    }

    return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
}

modem_e_modem_hal_status_t modem_e_modem_hal_reset( const void* context )
{
    modem_e_event_fields_t  events;
    modem_e_response_code_t rc             = MODEM_E_RESPONSE_CODE_OK;
    bool                         event_received = false;
    modem_e_board_set_ready( false );

    /* Start a reset timeout timer */
    timer_init( &modem_e_reset_timeout_timer, on_modem_e_reset_timeout_event );
    timer_set_value( &modem_e_reset_timeout_timer, MODEM_E_RESET_TIMEOUT );
    timer_start( &modem_e_reset_timeout_timer );
    modem_e_reset_timeout = false;

    hal_gpio_set_value( ( ( modem_e_t* ) context )->reset.pin, 0 );
    HAL_Delay( 1 );
    hal_gpio_set_value( ( ( modem_e_t* ) context )->reset.pin, 1 );
    while( !event_received )
    {
        rc = modem_e_get_event( context, &events );
        if( rc == MODEM_E_RESPONSE_CODE_OK )
        {
            if( events.event_type == MODEM_E_LORAWAN_EVENT_RESET )
            {
                event_received = true;
            }
        }
    }
    return MODEM_E_MODEM_HAL_STATUS_OK;
}

void modem_e_modem_hal_enter_dfu( const void* context )
{
    /* Force dio0 to 0 */
    hal_gpio_init_out( ( ( modem_e_t* ) context )->busy.pin, 0 );

    /* reset the chip */
    hal_gpio_set_value( ( ( modem_e_t* ) context )->reset.pin, 0 );
    HAL_Delay( 1 );
    hal_gpio_set_value( ( ( modem_e_t* ) context )->reset.pin, 1 );

    /* wait 250ms */
    HAL_Delay( 250 );

    /* reinit dio0 */
    hal_gpio_init_in( ( ( modem_e_t* ) context )->busy.pin, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_OFF, NULL );
}

modem_e_modem_hal_status_t modem_e_modem_hal_wakeup( const void* context )
{
    if( modem_e_modem_hal_wait_on_busy( context, 10000 ) == MODEM_E_MODEM_HAL_STATUS_OK )
    {
        /* Wakeup radio */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );
    }
    else
    {
        return MODEM_E_MODEM_HAL_STATUS_BUSY_TIMEOUT;
    }

    /* Wait on busy pin for 1000 ms */
    return modem_e_modem_hal_wait_on_unbusy( context, 1000 );
}

/*!
 * @brief Bootstrap bootloader and SPI bootloader API implementation
 */

modem_e_hal_status_t modem_e_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length )
{
    if( modem_e_hal_wakeup( context ) == MODEM_E_HAL_STATUS_OK )
    {
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );
        for( uint16_t i = 0; i < command_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, command[i] );
        }
        for( uint16_t i = 0; i < data_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, data[i] );
        }
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        return modem_e_hal_wait_on_busy( context, 5000 );
    }
    return MODEM_E_HAL_STATUS_ERROR;
}

modem_e_hal_status_t modem_e_hal_read( const void* context, const uint8_t* command, const uint16_t command_length,
                                     uint8_t* data, const uint16_t data_length )
{
    if( modem_e_hal_wakeup( context ) == MODEM_E_HAL_STATUS_OK )
    {
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        for( uint16_t i = 0; i < command_length; i++ )
        {
            hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, command[i] );
        }

        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        if( modem_e_hal_wait_on_busy( context, 5000 ) != MODEM_E_HAL_STATUS_OK )
        {
            return MODEM_E_HAL_STATUS_ERROR;
        }

        /* Send dummy byte */
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );

        for( uint16_t i = 0; i < data_length; i++ )
        {
            data[i] = hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );
        }

        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        return modem_e_hal_wait_on_busy( context, 5000 );
    }
    return MODEM_E_HAL_STATUS_ERROR;
}

modem_e_hal_status_t modem_e_hal_direct_read( const void* context, uint8_t* data, const uint16_t data_length )
{
    hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

    for( uint16_t i = 0; i < data_length; i++ )
    {
        data[i] = hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, 0 );
    }

    hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

    return modem_e_hal_wait_on_busy( context, 5000 );
}

modem_e_hal_status_t modem_e_hal_write_read( const void* context, const uint8_t* command, uint8_t* data,
                                             const uint16_t data_length )
{
    if( modem_e_hal_wakeup( context ) == MODEM_E_HAL_STATUS_OK )
    {
        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );

        for( uint16_t i = 0; i < data_length; i++ )
        {
            data[i] = hal_spi_in_out( ( ( modem_e_t* ) context )->spi_id, command[i] );
        }

        hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

        return modem_e_hal_wait_on_busy( context, 5000 );
    }
    return MODEM_E_HAL_STATUS_ERROR;
}

modem_e_hal_status_t modem_e_hal_wakeup( const void* context )
{
    /* Wakeup radio */
    hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 0 );
    hal_gpio_set_value( ( ( modem_e_t* ) context )->nss.pin, 1 );

    /* Wait on busy pin for 5000 ms */
    return modem_e_hal_wait_on_busy( context, 5000 );
}

modem_e_hal_status_t modem_e_hal_reset( const void* context )
{
    hal_gpio_set_value( ( ( modem_e_t* ) context )->reset.pin, 0 );
    HAL_Delay( 1 );
    hal_gpio_set_value( ( ( modem_e_t* ) context )->reset.pin, 1 );

    return MODEM_E_HAL_STATUS_OK;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void on_modem_e_reset_timeout_event( void* context ) { modem_e_reset_timeout = true; }

static modem_e_hal_status_t modem_e_hal_wait_on_busy( const void* context, uint32_t timeout_ms )
{
#if 0
    while( hal_gpio_get_value( ( ( modem_e_t* ) context )->busy.pin ) == 1 )
    {
        ;
    }
#else
    uint32_t start = hal_rtc_get_time_ms( );
    while( hal_gpio_get_value( ( ( modem_e_t* ) context )->busy.pin ) == 1 )
    {
        if( ( int32_t )( hal_rtc_get_time_ms( ) - start ) > ( int32_t ) timeout_ms )
        {
            return MODEM_E_HAL_STATUS_ERROR;
        }
    }
#endif
    return MODEM_E_HAL_STATUS_OK;
}

static modem_e_modem_hal_status_t modem_e_modem_hal_wait_on_busy( const void* context, uint32_t timeout_ms )
{
#if 0
    while( hal_gpio_get_value( ( ( modem_e_t* ) context )->busy.pin ) == 0 )
    {
        ;
    }
#else
    uint32_t start   = hal_rtc_get_time_ms( );
    uint32_t current = 0;
    while( hal_gpio_get_value( ( ( modem_e_t* ) context )->busy.pin ) == 0 )
    {
        current = hal_rtc_get_time_ms( );
        if( ( int32_t )( current - start ) > ( int32_t ) timeout_ms )
        {
            return MODEM_E_MODEM_HAL_STATUS_ERROR;
        }
    }
#endif
    return MODEM_E_MODEM_HAL_STATUS_OK;
}

static modem_e_modem_hal_status_t modem_e_modem_hal_wait_on_unbusy( const void* context, uint32_t timeout_ms )
{
#if 0
    while( hal_gpio_get_value( ( ( modem_e_t* ) context )->busy.pin ) == 1 )
    {
        ;
    }
#else
    uint32_t start   = hal_rtc_get_time_ms( );
    uint32_t current = 0;
    while( hal_gpio_get_value( ( ( modem_e_t* ) context )->busy.pin ) == 1 )
    {
        current = hal_rtc_get_time_ms( );
        if( ( int32_t )( current - start ) > ( int32_t ) timeout_ms )
        {
            return MODEM_E_MODEM_HAL_STATUS_ERROR;
        }
    }
#endif
    return MODEM_E_MODEM_HAL_STATUS_OK;
}

/* --- EOF ------------------------------------------------------------------ */

/*!
 * @file      lr1121_modem_board.c
 *
 * @brief     Target board lr1121 EVK Modem board driver implementation
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
#include "lr1121_hal.h"
#include "lr1121_modem_hal.h"
#include "lr1121_modem_lorawan.h"
#include "lr1121_modem_board.h"
#include "lr1121_modem_modem_types.h"
#include "leds.h"
#include "smtc_hal_tmr_list.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define GNSS_WEEK_NUMBER_ROLLOVER_2019_2038 2

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*!
 * @brief lr1121 EVK LED context
 */
typedef struct
{
    timer_event_t led_timer;         /*!< @brief Pulse timer */
    bool          timer_initialized; /*!< @brief True if the pulse timer has been initialized, false otherwise */
} lr1121_modem_board_led_ctx_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*!
 * @brief modem ready flag
 */
static bool modem_is_ready = false;

/*!
 * @brief LED1110 EVK LED context array
 */
static lr1121_modem_board_led_ctx_t lr1121_modem_board_leds[LR1121_EVK_LED_COUNT] = { { .timer_initialized = false },
                                                                                      { .timer_initialized = false },
                                                                                      { .timer_initialized = false } };

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*!
 * @brief Pulse timer timeout callback
 *
 * @param context Context used to retrieve the index of the relevant LED.
 */
static void on_led_timer_event( void* context );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC VARIABLES --------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void lr1121_modem_board_init_io_context( void* context )
{
    ( ( lr1121_t* ) context )->reset.pin     = RADIO_RESET;
    ( ( lr1121_t* ) context )->nss.pin       = RADIO_NSS;
    ( ( lr1121_t* ) context )->event.pin     = RADIO_EVENT;
    ( ( lr1121_t* ) context )->event.context = ( ( lr1121_t* ) context );
    ( ( lr1121_t* ) context )->busy.pin      = RADIO_BUSY;
    ( ( lr1121_t* ) context )->spi.pins.miso = RADIO_MISO;
    ( ( lr1121_t* ) context )->spi.pins.mosi = RADIO_MOSI;
    ( ( lr1121_t* ) context )->spi.pins.sclk = RADIO_SCLK;
    ( ( lr1121_t* ) context )->spi_id        = HAL_RADIO_SPI_ID;
}

void lr1121_modem_board_init_io( const void* context )
{
    hal_gpio_init_out( ( ( lr1121_t* ) context )->reset.pin, 1 );
    hal_gpio_init_out( ( ( lr1121_t* ) context )->nss.pin, 1 );
    hal_gpio_init_in( ( ( lr1121_t* ) context )->busy.pin, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_OFF, NULL );
    hal_gpio_init_in( ( ( lr1121_t* ) context )->event.pin, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_RISING,
                      &( ( lr1121_t* ) context )->event );
}

void lr1121_modem_board_deinit_io( const void* context )
{
    hal_gpio_init_out( ( ( lr1121_t* ) context )->spi.pins.mosi, 0 );
    hal_gpio_init_out( ( ( lr1121_t* ) context )->spi.pins.miso, 0 );
    hal_gpio_init_out( ( ( lr1121_t* ) context )->spi.pins.sclk, 0 );
    hal_gpio_init_out( ( ( lr1121_t* ) context )->nss.pin, 1 );
    hal_gpio_init_out( ( ( lr1121_t* ) context )->reset.pin, 1 );
    hal_gpio_init_in( ( ( lr1121_t* ) context )->busy.pin, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_OFF, NULL );
    hal_gpio_init_in( ( ( lr1121_t* ) context )->event.pin, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_RISING, NULL );
}

void lr1121_modem_board_analog_deinit_io( const void* context )
{
    hal_gpio_deinit( ( ( lr1121_t* ) context )->event.pin );
    hal_gpio_deinit( ( ( lr1121_t* ) context )->busy.pin );
}

uint32_t lr1121_modem_board_get_tcxo_wakeup_time( void ) { return BOARD_TCXO_WAKEUP_TIME; }

lr1121_modem_response_code_t lr1121_modem_board_init( const void* context )
{
    lr1121_modem_response_code_t    modem_response_code = LR1121_MODEM_RESPONSE_CODE_OK;
    lr1121_modem_hal_status_t       modem_hal_status    = LR1121_MODEM_HAL_STATUS_OK;
    lr1121_modem_system_lfclk_cfg_t lfclk_cfg           = LR1121_MODEM_SYSTEM_LFCLK_XTAL;

    modem_hal_status = lr1121_modem_hal_reset( context );
    if( modem_hal_status != LR1121_MODEM_HAL_STATUS_OK )
    {
        /* Something goes wrong with the lr1121 modem-e */
        return LR1121_MODEM_RESPONSE_CODE_FAIL;
    }
    modem_response_code |= lr1121_modem_system_cfg_lfclk( context, lfclk_cfg, true );
    modem_response_code |= lr1121_modem_set_crystal_error( context, 10 );

    return modem_response_code;
}

void lr1121_modem_board_lna_on( void ) { lna_on( ); }

void lr1121_modem_board_lna_off( void ) { lna_off( ); }

lr1121_modem_response_code_t lr1121_modem_board_event_flush( const void* context )
{
    lr1121_modem_response_code_t modem_response_code = LR1121_MODEM_RESPONSE_CODE_OK;
    lr1121_modem_event_fields_t  event_fields;

    do
    {
        modem_response_code = lr1121_modem_get_event( context, &event_fields );
    } while( modem_response_code != LR1121_MODEM_RESPONSE_CODE_NO_EVENT );

    return modem_response_code;
}

bool lr1121_modem_board_read_event_line( const void* context )
{
    return hal_gpio_get_value( ( ( lr1121_t* ) context )->event.pin );
}

bool lr1121_modem_board_is_ready( void ) { return modem_is_ready; }

void lr1121_modem_board_set_ready( bool ready ) { modem_is_ready = ready; }

lr1121_modem_response_code_t lr1121_modem_board_measure_battery_drop( const void* context, int32_t* drop,
                                                                      uint32_t* time_recovery )
{
    lr1121_modem_response_code_t modem_response_code = LR1121_MODEM_RESPONSE_CODE_OK;
    lr1121_modem_regions_t       region;
    uint32_t                     relaxed_voltage = 0;
    uint32_t                     tick_vdrop      = 0;

    relaxed_voltage = hal_mcu_get_vref_level( );

    modem_response_code |= lr1121_modem_get_region( context, &region );

    /* Enter in test mode */
    modem_response_code |= lr1121_modem_test_mode_start( context );

    switch( region )
    {
    case LR1121_LORAWAN_REGION_EU868:
    case LR1121_LORAWAN_REGION_IN865:
    case LR1121_LORAWAN_REGION_RU864:
    {
        modem_response_code |= lr1121_modem_test_tx_cw( context, 865500000, 14 );
        break;
    }
    case LR1121_LORAWAN_REGION_US915:
    case LR1121_LORAWAN_REGION_AU915:
    case LR1121_LORAWAN_REGION_AS923_GRP1:
    case LR1121_LORAWAN_REGION_AS923_GRP2:
    case LR1121_LORAWAN_REGION_AS923_GRP3:
    case LR1121_LORAWAN_REGION_KR920:
    {
        modem_response_code |= lr1121_modem_test_tx_cw( context, 920900000, 14 );
        break;
    }
    default:
    {
        HAL_DBG_TRACE_ERROR( "This region is not covered by this test\n\r" );
        break;
    }
    }

    /* Wait the drop */
    HAL_Delay( 2000 );

    /* Measure the drop */
    *drop = relaxed_voltage - hal_mcu_get_vref_level( );

    /* Leave the test mode */
    modem_response_code |= lr1121_modem_test_nop( context );
    modem_response_code |= lr1121_modem_test_exit( context );

    HAL_DBG_TRACE_PRINTF( "Battery voltage drop = %d mV\n\r", *drop );

    if( *drop > 0 )
    {
        *time_recovery = 0;
        /* Get Start Tick*/
        tick_vdrop = HAL_GetTick( );
        /* Wait 66% of drop recovery */
        while( ( hal_mcu_get_vref_level( ) < relaxed_voltage - ( *drop / 3 ) ) && ( *time_recovery < 10000 ) )
        {
            *time_recovery = HAL_GetTick( ) - tick_vdrop;
        }

        HAL_DBG_TRACE_PRINTF( "Voltage recovery time = %d ms\n\r", *time_recovery );
    }

    return modem_response_code;
}

void lr1121_modem_board_led_set( uint32_t led_mask, bool turn_on )
{
    /* If a pulse timer is running on one of the requested LEDs, it
     *  must be stopped to avoid conflicting with the requested LED state. */
    lr1121_evk_led_t led = LR1121_EVK_LED_TX;
    for( led = LR1121_EVK_LED_TX; led < LR1121_EVK_LED_COUNT; led++ )
    {
        if( led_mask & ( 1 << led ) )
        {
            if( ( lr1121_modem_board_leds[led].timer_initialized ) &&
                ( timer_is_started( &lr1121_modem_board_leds[led].led_timer ) ) )
            {
                timer_stop( &lr1121_modem_board_leds[led].led_timer );
            }
        }
    }
    if( turn_on )
    {
        leds_on( led_mask );
    }
    else
    {
        leds_off( led_mask );
    }
}

void lr1121_modem_board_led_pulse( uint32_t led_mask, bool turn_on, uint32_t duration_ms )
{
    lr1121_evk_led_t led = LR1121_EVK_LED_TX;
    for( led = LR1121_EVK_LED_TX; led < LR1121_EVK_LED_COUNT; led++ )
    {
        if( led_mask & ( 1 << led ) )
        {
            if( lr1121_modem_board_leds[led].timer_initialized )
            {
                if( timer_is_started( &lr1121_modem_board_leds[led].led_timer ) )
                {
                    timer_stop( &lr1121_modem_board_leds[led].led_timer );
                }
            }
            else
            {
                timer_init( &lr1121_modem_board_leds[led].led_timer, on_led_timer_event );
                timer_set_context( &lr1121_modem_board_leds[led].led_timer, ( void* ) led );
                lr1121_modem_board_leds[led].timer_initialized = true;
            }
            timer_set_value( &lr1121_modem_board_leds[led].led_timer, duration_ms );
            timer_start( &lr1121_modem_board_leds[led].led_timer );
        }
    }
    if( turn_on )
    {
        leds_on( led_mask );
    }
    else
    {
        leds_off( led_mask );
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

// static lr1121_modem_response_code_t lr1121_modem_board_init_tcxo_io( const void* context )
// {
//     return lr1121_modem_system_set_tcxo_mode( context, LR1121_MODEM_SYSTEM_TCXO_CTRL_1_8V,
//                                               ( lr1121_modem_board_get_tcxo_wakeup_time( ) * 1000 ) / 30.52 );
// }

void on_led_timer_event( void* context )
{
    lr1121_evk_led_t led      = ( lr1121_evk_led_t ) context;
    uint32_t         led_mask = 1 << led;
    leds_toggle( led_mask );
    timer_stop( &lr1121_modem_board_leds[led].led_timer );
}

/* --- EOF ------------------------------------------------------------------ */

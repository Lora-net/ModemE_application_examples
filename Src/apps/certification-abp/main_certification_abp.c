/**
 * @ingroup   apps_certification_abp
 * @file      main_certification_abp.c
 *
 * @brief     modem_e Modem-E certification ABP implementation
 *
 * @copyright
 * @parblock
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
 * @endparblock
 */

/**
 * @addtogroup apps_certification_abp
 * modem_e Modem-E certification ABP device implementation
 * @{
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include "lorawan_commissioning.h"
#include "modem_e_board.h"
#include "smtc_utilities.h"
#include "apps_utilities.h"
#include "modem_e_system_types.h"
#include "modem_e_helper.h"
#include "modem_e_modem.h"
#include "modem_e_lorawan.h"
#include "common_app_configuration.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/**
 * @brief Stringify constants
 */
#define xstr( a ) str( a )
#define str( a ) #a

/**
 * @brief Helper macro that returned a human-friendly message if a command does not return MODEM_E_RESPONSE_CODE_OK
 *
 * @remark The macro is implemented to be used with functions returning a @ref modem_e_response_code_t
 *
 * @param[in] rc  Return code
 */

#define ASSERT_SMTC_MODEM_RC( rc_func )                                                        \
    do                                                                                         \
    {                                                                                          \
        modem_e_response_code_t rc = rc_func;                                             \
        if( rc == MODEM_E_RESPONSE_CODE_NOT_INITIALIZED )                                 \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( MODEM_E_RESPONSE_CODE_NOT_INITIALIZED ) );         \
        }                                                                                      \
        else if( rc == MODEM_E_RESPONSE_CODE_INVALID )                                    \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( MODEM_E_RESPONSE_CODE_INVALID ) );                 \
        }                                                                                      \
        else if( rc == MODEM_E_RESPONSE_CODE_BUSY )                                       \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( MODEM_E_RESPONSE_CODE_BUSY ) );                    \
        }                                                                                      \
        else if( rc == MODEM_E_RESPONSE_CODE_FAIL )                                       \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( MODEM_E_RESPONSE_CODE_FAIL ) );                    \
        }                                                                                      \
        else if( rc == MODEM_E_RESPONSE_CODE_NO_TIME )                                    \
        {                                                                                      \
            HAL_DBG_TRACE_WARNING( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__, \
                                   xstr( MODEM_E_RESPONSE_CODE_NO_TIME ) );               \
        }                                                                                      \
        else if( rc == MODEM_E_RESPONSE_CODE_NO_EVENT )                                   \
        {                                                                                      \
            HAL_DBG_TRACE_INFO( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,    \
                                xstr( MODEM_E_RESPONSE_CODE_NO_EVENT ) );                 \
        }                                                                                      \
    } while( 0 )

#define EXTI_BUTTON PC_13

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

static uint8_t user_dev_addr[MODEM_E_ABP_DEV_ADDR_LEGNTH] = LORAWAN_ABP_DEV_ADDR;
static uint8_t user_nwk_skey[MODEM_E_ABP_SKEYS_LEGNTH]    = LORAWAN_ABP_NWK_SKEY;
static uint8_t user_app_skey[MODEM_E_ABP_SKEYS_LEGNTH]    = LORAWAN_ABP_APP_SKEY;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

extern modem_e_t modem_e;
extern hal_rtc_t hal_rtc;

static uint8_t                          rx_payload[LORAWAN_APP_DATA_MAX_SIZE] = { 0 };
static uint8_t                          rx_payload_size = 0;
static modem_e_downlink_metadata_t rx_metadata     = { 0 };
static uint8_t                          rx_remaining    = 0;

static volatile bool                              user_button_is_press = false;
static volatile modem_e_certification_mode_t certif_running       = false;
static uint32_t                                   uplink_counter       = 0;
static uint32_t                                   confirmed_counter    = 0;
static bool                                       nvm_restored         = false;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void user_button_callback( void* context );
static void main_handle_button_pushed( void* context );
static void send_uplinks_counter_on_port( uint8_t port );
static modem_e_response_code_t send_frame( const uint8_t* tx_frame_buffer, const uint8_t tx_frame_buffer_size,
                                                uint8_t port, const modem_e_uplink_type_t tx_confirmed );
static void event_process( void* context );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

int main( void )
{
    hal_mcu_init( );
    hal_mcu_init_periph( );

    leds_blink( LED_ALL_MASK, 250, 4, true );

    HAL_DBG_TRACE_MSG( "\n\n" );
    HAL_DBG_TRACE_INFO( "###### ===== Certification ABP example is starting ==== ######\n\n\n" );

    hal_mcu_disable_irq( );

    hal_gpio_irq_t nucleo_blue_button = {
        .pin      = EXTI_BUTTON,
        .context  = NULL,
        .callback = user_button_callback,
    };
    hal_gpio_init_in( EXTI_BUTTON, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_FALLING, &nucleo_blue_button );

    hal_gpio_irq_t event_callback = {
        .pin      = modem_e.event.pin,
        .context  = &modem_e,
        .callback = event_process,
    };
    hal_gpio_init_in( modem_e.event.pin, HAL_GPIO_PULL_MODE_DOWN, HAL_GPIO_IRQ_MODE_RISING, &event_callback );

    modem_e_system_reboot( &modem_e, false );
    hal_mcu_enable_irq( );
    HAL_DBG_TRACE_MSG( "Initialization done\n\n" );

    leds_blink( LED_TX_MASK, 100, 20, true );
    while( 1 )
    {
        if( user_button_is_press == true )
        {
            user_button_is_press = false;
            main_handle_button_pushed( &modem_e );
        }

        hal_mcu_disable_irq( );
        if( ( user_button_is_press == false ) )
        {
            hal_watchdog_reload( );
            hal_mcu_set_sleep_for_ms( WATCHDOG_RELOAD_PERIOD_MS );
        }
        hal_watchdog_reload( );
        hal_mcu_enable_irq( );
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void event_process( void* context )
{
    modem_e_response_code_t rc_event = MODEM_E_RESPONSE_CODE_OK;
    do
    {
        modem_e_event_fields_t current_event;
        rc_event = modem_e_get_event( context, &current_event );
        if( rc_event == MODEM_E_RESPONSE_CODE_OK )
        {
            switch( current_event.event_type )
            {
            case MODEM_E_LORAWAN_EVENT_RESET:
            {
                HAL_DBG_TRACE_MSG_COLOR( "Event received: RESET\n\n", HAL_DBG_TRACE_COLOR_BLUE );

                ASSERT_SMTC_MODEM_RC( modem_e_board_init( context ) );
                get_and_print_crashlog( context );

                if( nvm_restored )
                {
                    HAL_DBG_TRACE_INFO( "Session restored from NVM, skipping ABP connect\n\n" );
                    ASSERT_SMTC_MODEM_RC( modem_e_get_certification_mode(
                        context, ( modem_e_certification_mode_t* ) &certif_running ) );
                    print_certification( certif_running );
                }
                else
                {
                    HAL_DBG_TRACE_INFO( "First boot, performing ABP connect\n\n" );

                    ASSERT_SMTC_MODEM_RC( modem_e_set_region( context, LORAWAN_REGION_USED ) );
                    print_lorawan_region( LORAWAN_REGION_USED );

                    ASSERT_SMTC_MODEM_RC(
                        modem_e_connect_with_abp( context, user_dev_addr, user_nwk_skey, user_app_skey ) );
                   
                    HAL_DBG_TRACE_INFO( "###### ===== ABP CONNECTED ==== ######\n\n\n" );
                }
                break;
            }

            case MODEM_E_LORAWAN_EVENT_ALARM:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: ALARM\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                if( certif_running == MODEM_E_CERTIFICATION_MODE_ENABLE )
                {
                    modem_e_clear_alarm_timer( context );
                }
                else
                {
                    send_uplinks_counter_on_port( 101 );
                    ASSERT_SMTC_MODEM_RC( modem_e_set_alarm_timer( context, PERIODICAL_UPLINK_DELAY_S ) );
                }
                break;

            case MODEM_E_LORAWAN_EVENT_JOINED:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: JOINED\n", HAL_DBG_TRACE_COLOR_BLUE );
                HAL_DBG_TRACE_INFO( "Modem is now joined \n\n" );

                ASSERT_SMTC_MODEM_RC( modem_e_set_certification_mode(
                    context, MODEM_E_CERTIFICATION_MODE_ENABLE ) );
                certif_running = MODEM_E_CERTIFICATION_MODE_ENABLE;
                
                uint8_t adr_custom_list[16] = { 0 };
                ASSERT_SMTC_MODEM_RC( modem_e_set_adr_profile(
                    context, MODEM_E_ADR_PROFILE_NETWORK_SERVER_CONTROLLED, adr_custom_list ) );
                
                if( (LORAWAN_REGION_USED == MODEM_E_LORAWAN_REGION_US915) || (LORAWAN_REGION_USED == MODEM_E_LORAWAN_REGION_AU915))
                {
                    /* The following SetChannelMask command allows forcing the device to use one or more sub-bands of channels for the US915 and AU915 frequency plans, 
                    before receiving a LinkADRRequest from the Network Server. 
                    By default, the 64 channels at 125 kHz are enabled after an ABP join, but in practice we usually use 8- or 16-channel gateways. */
                    
                    modem_e_channel_mask_configuration_t channel_configuration;
                    channel_configuration.channel_mask_control = 5;
                    channel_configuration.channel_mask[0] = 0x01;
                    channel_configuration.channel_mask[1] = 0x00;
                    modem_e_connect_set_channel_mask( context, &channel_configuration, 1 );
                }
                if( certif_running == MODEM_E_CERTIFICATION_MODE_DISABLE )
                {
                    // Send first periodical uplink on port 101
                    send_uplinks_counter_on_port( 101 );
                    // start periodical uplink alarm
                    ASSERT_SMTC_MODEM_RC( modem_e_set_alarm_timer( context, PERIODICAL_UPLINK_DELAY_S ) );
                }
                break;

            case MODEM_E_LORAWAN_EVENT_TX_DONE:
            {
                const modem_e_tx_done_event_t tx_done_event_data =
                    ( modem_e_tx_done_event_t )( current_event.data >> 8 );
                HAL_DBG_TRACE_MSG_COLOR( "Event received: TXDONE\n\n", HAL_DBG_TRACE_COLOR_BLUE );

                HAL_DBG_TRACE_MSG( "TX DATA     : " );

                switch( tx_done_event_data )
                {
                case MODEM_E_TX_NOT_SENT:
                {
                    HAL_DBG_TRACE_PRINTF( " NOT SENT" );
                    uplink_counter--;
                    break;
                }
                case MODEM_E_CONFIRMED_TX:
                {
                    HAL_DBG_TRACE_PRINTF( " CONFIRMED - ACK" );
                    confirmed_counter++;
                    break;
                }
                case MODEM_E_UNCONFIRMED_TX:
                {
                    HAL_DBG_TRACE_MSG( " UNCONFIRMED\n\n" );
                    break;
                }
                default:
                {
                    HAL_DBG_TRACE_PRINTF( " unknown value (%02x)\n\n", tx_done_event_data );
                }
                }
                HAL_DBG_TRACE_MSG( "\n\n" );

                HAL_DBG_TRACE_INFO( "Transmission done \n" );
                break;
            }

            case MODEM_E_LORAWAN_EVENT_DOWN_DATA:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: DOWNDATA\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                ASSERT_SMTC_MODEM_RC( modem_e_get_downlink_data_size( context, &rx_payload_size, &rx_remaining ) );
                ASSERT_SMTC_MODEM_RC( modem_e_get_downlink_data( context, rx_payload, rx_payload_size ) );
                ASSERT_SMTC_MODEM_RC( modem_e_get_downlink_metadata( context, &rx_metadata ) );
                HAL_DBG_TRACE_PRINTF( "Data received on port %u\n", rx_metadata.fport );
                HAL_DBG_TRACE_ARRAY( "Received payload", rx_payload, rx_payload_size );
                break;

            case MODEM_E_LORAWAN_EVENT_JOIN_FAIL:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: JOINFAIL\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_LINK_CHECK:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: LINK_CHECK\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_CLASS_B_PING_SLOT_INFO:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: CLASS_B_PING_SLOT_INFO\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_CLASS_B_STATUS:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: CLASS_B_STATUS\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_LORAWAN_MAC_TIME:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: LORAWAN MAC TIME\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_NEW_MULTICAST_SESSION_CLASS_C:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: MULTICAST CLASS_C STOP\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_NEW_MULTICAST_SESSION_CLASS_B:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: MULTICAST CLASS_B STOP\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_C:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: New MULTICAST CLASS_C\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_B:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: New MULTICAST CLASS_B\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_REGIONAL_DUTY_CYCLE:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: REGIONAL DUTY CYCLE\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_RELAY_TX_DYNAMIC:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: RELAY_TX_DYNAMIC\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_RELAY_TX_MODE:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: RELAY_TX_MODE\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_RELAY_TX_SYNC:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: RELAY_TX_SYNC\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_ALC_SYNC_TIME:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: ALC_SYNC_TIME\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_FUOTA_DONE:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: FUOTA_DONE\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_TEST_MODE:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: TEST_MODE\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;
            
            case MODEM_E_LORAWAN_EVENT_DR_BACKOFF_LIMIT:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: DR_BACKOFF_LIMIT\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case MODEM_E_LORAWAN_EVENT_RESET_REQUEST:
            {
                HAL_DBG_TRACE_MSG_COLOR( "Event received: RESET_REQUEST\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                uint32_t nvm_write_counter;
                modem_e_response_code_t rc = modem_e_store_state_snapshot_to_nvm( context, &nvm_write_counter );
                if( rc == MODEM_E_RESPONSE_CODE_OK )
                {
                    HAL_DBG_TRACE_INFO( "State snapshot saved to NVM (write counter: %lu)\n",
                                        ( unsigned long ) nvm_write_counter );
                }
                else
                {
                    HAL_DBG_TRACE_ERROR( "Failed to save state snapshot to NVM\n" );
                }
                uint32_t timestamp_ms = hal_rtc_get_time_ms( );
                HAL_DBG_TRACE_INFO( "Timestamp saved: %lu ms, resetting MCU...\n",
                                    ( unsigned long ) timestamp_ms );
                
                modem_e_system_reboot( context, false );

                hal_rtc_delay_in_ms( 2000 );
                
                uint32_t current_time = hal_rtc_get_time_ms( );
                uint64_t elapsed_time = ( uint64_t ) current_time - timestamp_ms;
                HAL_DBG_TRACE_INFO( "Current time: %lu, Elapsed time: %d ms\n",
                                    ( unsigned long ) current_time, elapsed_time );
                rc = modem_e_restore_state_snapshot_from_nvm( context, elapsed_time );
                if( rc == MODEM_E_RESPONSE_CODE_OK )
                {
                    HAL_DBG_TRACE_INFO( "State snapshot restored from NVM successfully\n\n" );
                    nvm_restored = true;
                }
                else
                {
                    HAL_DBG_TRACE_ERROR( "Failed to restore state snapshot from NVM (rc=%d)\n\n", rc );
                }

                break;
            }

            default:
                HAL_DBG_TRACE_INFO( "Event not handled 0x%02x\n", current_event.event_type );
                break;
            }
        }
    } while( rc_event != MODEM_E_RESPONSE_CODE_NO_EVENT );
}

static void user_button_callback( void* context )
{
    HAL_DBG_TRACE_INFO( "Button pushed\n" );

    ( void ) context;

    static uint32_t last_press_timestamp_ms = 0;

    if( ( int32_t )( hal_rtc_get_time_ms( ) - last_press_timestamp_ms ) > 500 )
    {
        last_press_timestamp_ms = hal_rtc_get_time_ms( );
        user_button_is_press    = true;
    }
}

static void main_handle_button_pushed( void* context )
{
    if( certif_running == MODEM_E_CERTIFICATION_MODE_ENABLE )
    {
        ASSERT_SMTC_MODEM_RC( modem_e_set_certification_mode( context, MODEM_E_CERTIFICATION_MODE_DISABLE ) );
        ASSERT_SMTC_MODEM_RC( modem_e_leave_network( context ) );
        certif_running = MODEM_E_CERTIFICATION_MODE_DISABLE;
    }
    else
    {
        ASSERT_SMTC_MODEM_RC( modem_e_set_certification_mode( context, MODEM_E_CERTIFICATION_MODE_ENABLE ) );
        certif_running = MODEM_E_CERTIFICATION_MODE_ENABLE;
    }
    print_certification( certif_running );
}

static void send_uplinks_counter_on_port( uint8_t port )
{
    uint8_t buff[8] = { 0 };
    buff[0]         = ( uplink_counter >> 24 ) & 0xFF;
    buff[1]         = ( uplink_counter >> 16 ) & 0xFF;
    buff[2]         = ( uplink_counter >> 8 ) & 0xFF;
    buff[3]         = ( uplink_counter & 0xFF );
    buff[0]         = ( confirmed_counter >> 24 ) & 0xFF;
    buff[1]         = ( confirmed_counter >> 16 ) & 0xFF;
    buff[2]         = ( confirmed_counter >> 8 ) & 0xFF;
    buff[3]         = ( confirmed_counter & 0xFF );
    ASSERT_SMTC_MODEM_RC( send_frame( buff, 8, port, true ) );
    uplink_counter++;
}

static modem_e_response_code_t send_frame( const uint8_t* tx_frame_buffer, const uint8_t tx_frame_buffer_size,
                                                uint8_t port, const modem_e_uplink_type_t tx_confirmed )
{
    modem_e_response_code_t modem_response_code = MODEM_E_RESPONSE_CODE_OK;
    uint8_t                      tx_max_payload;
    int32_t                      duty_cycle;

    modem_e_get_duty_cycle_status( &modem_e, &duty_cycle );

    if( duty_cycle < 0 )
    {
        HAL_DBG_TRACE_INFO( "DUTY CYCLE, NEXT UPLINK AVAILABLE in %d milliseconds \n\n\n", -duty_cycle );
        return modem_response_code;
    }

    modem_response_code = modem_e_get_next_tx_max_payload( &modem_e, &tx_max_payload );
    if( modem_response_code != MODEM_E_RESPONSE_CODE_OK )
    {
        HAL_DBG_TRACE_ERROR( "\n\n modem_e_get_next_tx_max_payload RC : %d \n\n", modem_response_code );
    }

    if( tx_frame_buffer_size > tx_max_payload )
    {
        HAL_DBG_TRACE_PRINTF( "\n\n APP DATA > MAX PAYLOAD AVAILABLE (%d bytes) \n\n", tx_max_payload );
        modem_response_code = modem_e_request_tx( &modem_e, port, tx_confirmed, NULL, 0 );
    }
    else
    {
        modem_response_code =
            modem_e_request_tx( &modem_e, port, tx_confirmed, tx_frame_buffer, tx_frame_buffer_size );
    }

    if( modem_response_code == MODEM_E_RESPONSE_CODE_OK )
    {
        HAL_DBG_TRACE_INFO( "modem_e MODEM-E REQUEST TX \n\n" );
        HAL_DBG_TRACE_MSG( "TX DATA     : " );
        print_hex_buffer( tx_frame_buffer, tx_frame_buffer_size );
        HAL_DBG_TRACE_MSG( "\n\n\n" )
    }
    else
    {
        HAL_DBG_TRACE_ERROR( "modem_e MODEM-E REQUEST TX ERROR CMD, modem_response_code : %d \n\n\n",
                             modem_response_code );
    }
    return modem_response_code;
}

/* --- EOF ------------------------------------------------------------------ */

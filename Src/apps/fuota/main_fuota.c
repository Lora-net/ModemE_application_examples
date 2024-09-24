/*!
 * @ingroup   apps_fuota
 * @file      main_fuota.c
 *
 * @brief     lr1121 Modem-E FUOTA example implementation
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

/*!
 * @addtogroup apps_fuota
 * lr1121 Modem-E FUOTA example implementation
 * @{
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include "lorawan_commissioning.h"
#include "lr1121_modem_board.h"
#include "apps_utilities.h"
#include "lr1121_modem_helper.h"
#include "lr1121_modem_system_types.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*!
 * @brief Stringify constants
 */
#define xstr( a ) str( a )
#define str( a ) #a

/*!
 * @brief Helper macro that returned a human-friendly message if a command does not return LR1121_MODEM_RESPONSE_CODE_OK
 *
 * @remark The macro is implemented to be used with functions returning a @ref lr1121_modem_return_code_t
 *
 * @param[in] rc  Return code
 */

#define ASSERT_SMTC_MODEM_RC( rc_func )                                                        \
    do                                                                                         \
    {                                                                                          \
        lr1121_modem_response_code_t rc = rc_func;                                             \
        if( rc == LR1121_MODEM_RESPONSE_CODE_NOT_INITIALIZED )                                 \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( LR1121_MODEM_RESPONSE_CODE_NOT_INITIALIZED ) );         \
        }                                                                                      \
        else if( rc == LR1121_MODEM_RESPONSE_CODE_INVALID )                                    \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( LR1121_MODEM_RESPONSE_CODE_INVALID ) );                 \
        }                                                                                      \
        else if( rc == LR1121_MODEM_RESPONSE_CODE_BUSY )                                       \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( LR1121_MODEM_RESPONSE_CODE_BUSY ) );                    \
        }                                                                                      \
        else if( rc == LR1121_MODEM_RESPONSE_CODE_FAIL )                                       \
        {                                                                                      \
            HAL_DBG_TRACE_ERROR( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,   \
                                 xstr( LR1121_MODEM_RESPONSE_CODE_FAIL ) );                    \
        }                                                                                      \
        else if( rc == LR1121_MODEM_RESPONSE_CODE_NO_TIME )                                    \
        {                                                                                      \
            HAL_DBG_TRACE_WARNING( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__, \
                                   xstr( LR1121_MODEM_RESPONSE_CODE_NO_TIME ) );               \
        }                                                                                      \
        else if( rc == LR1121_MODEM_RESPONSE_CODE_NO_EVENT )                                   \
        {                                                                                      \
            HAL_DBG_TRACE_INFO( "In %s - %s (line %d): %s\n", __FILE__, __func__, __LINE__,    \
                                xstr( LR1121_MODEM_RESPONSE_CODE_NO_EVENT ) );                 \
        }                                                                                      \
    } while( 0 )

/**
 * @brief Watchdog counter reload value during sleep (The period must be lower than MCU watchdog period (here 20s))
 */
#define WATCHDOG_RELOAD_PERIOD_MS 20000

/**
 * @brief Periodical uplink alarm delay in seconds
 */
#define PERIODICAL_UPLINK_DELAY_S 50

#define EXTI_BUTTON PC_13

/*!
 * @brief User application data buffer size
 */
#define LORAWAN_APP_DATA_MAX_SIZE 242

/*!
 * @brief LoRaWAN regulatory region.
 * One of:
 * LR1121_LORAWAN_REGION_EU868
 * LR1121_LORAWAN_REGION_US915
 * LR1121_LORAWAN_REGION_AU915
 * LR1121_LORAWAN_REGION_AS923_GRP1
 * LR1121_LORAWAN_REGION_CN470
 * LR1121_LORAWAN_REGION_AS923_GRP2
 * LR1121_LORAWAN_REGION_AS923_GRP3
 * LR1121_LORAWAN_REGION_AS923_GRP4
 * LR1121_LORAWAN_REGION_IN865
 * LR1121_LORAWAN_REGION_KR920
 * LR1121_LORAWAN_REGION_RU864
 */
#define LORAWAN_REGION_USED LR1121_LORAWAN_REGION_EU868

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/**
 * @brief Stack credentials
 */

#if( !USE_LR11XX_CREDENTIALS )
static const uint8_t user_join_eui[8] = LORAWAN_JOIN_EUI;
static const uint8_t user_dev_eui[8]  = LORAWAN_DEVICE_EUI;
static const uint8_t user_nwk_key[16] = LORAWAN_NWK_KEY;
static const uint8_t user_app_key[16] = LORAWAN_APP_KEY;
#endif

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

extern lr1121_t lr1121;

static volatile bool user_button_is_press = false;  // Flag for button status

/**
 * @brief Internal credentials
 */
#if defined( USE_LR11XX_CREDENTIALS )
static uint8_t chip_eui[8] = { 0 };
static uint8_t chip_pin[4] = { 0 };
#endif
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/**
 * @brief User callback for button EXTI
 *
 * @param context Defined by the user at the init
 */
static void user_button_callback( void* context );

/**
 * @brief Send empty uplink
 */
static lr1121_modem_response_code_t send_empty_uplink( const lr1121_modem_uplink_type_t tx_confirmed );

/**
 * @brief Process received events
 *
 */
static void event_process( void* context );

/**
 * @brief Get the and print multicast class B group information
 *
 * @param context Defined by the user at the init
 * @param group_id The multicast group id to print information
 */
static void get_and_print_multicast_class_b_group_information( void* context, uint8_t group_id );

/**
 * @brief Get the and print multicast class C group information
 *
 * @param context Defined by the user at the init
 * @param group_id The multicast group id to print information
 */
static void get_and_print_multicast_class_c_group_information( void* context, uint8_t group_id );
/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

int main( void )
{
    // Configure all the µC periph (clock, gpio, timer, ...)
    hal_mcu_init( );
    hal_mcu_init_periph( );

    leds_blink( LED_ALL_MASK, 250, 4, true );

    HAL_DBG_TRACE_MSG( "\r\n" );
    HAL_DBG_TRACE_INFO( "###### ===== FUOTA example is starting (with uplink every %d sec) ==== ######\r\n\r\n",
                        PERIODICAL_UPLINK_DELAY_S );

    // Disable IRQ to avoid unwanted behavior during init
    hal_mcu_disable_irq( );

    // Configure Nucleo blue button as EXTI
    hal_gpio_irq_t nucleo_blue_button = {
        .pin      = EXTI_BUTTON,
        .context  = NULL,                  // context pass to the callback - not used in this example
        .callback = user_button_callback,  // callback called when EXTI is triggered
    };
    hal_gpio_init_in( EXTI_BUTTON, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_FALLING, &nucleo_blue_button );

    // Configure event callback on interrupt
    hal_gpio_irq_t event_callback = {
        .pin      = lr1121.event.pin,
        .context  = &lr1121,        // context pass to the callback - not used in this example
        .callback = event_process,  // callback called when event pin is triggered
    };
    hal_gpio_init_in( lr1121.event.pin, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_RISING, &event_callback );

    // Flush events before enabling irq
    lr1121_modem_board_event_flush( &lr1121 );

    // Init done: enable interruption
    hal_mcu_enable_irq( );

    /* Board is initialized */
    leds_blink( LED_TX_MASK, 100, 20, true );
    HAL_DBG_TRACE_MSG( "Initialization done\r\n" );

    lr1121_modem_system_reboot( &lr1121, false );

    while( 1 )
    {
        // Check button
        if( user_button_is_press == true )
        {
            user_button_is_press = false;
            HAL_DBG_TRACE_MSG( "Button pushed\r\n" );

            lr1121_modem_lorawan_status_t modem_status;
            lr1121_modem_get_status( &lr1121, &modem_status );
            // Check if the device has already joined a network
            if( ( modem_status & LR1121_LORAWAN_JOINED ) == LR1121_LORAWAN_JOINED )
            {
                // Send the uplink counter on port 102
                send_empty_uplink( LR1121_MODEM_UPLINK_UNCONFIRMED );
            }
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
    // Continue to read modem event until all event has been processed
    lr1121_modem_response_code_t rc_event = LR1121_MODEM_RESPONSE_CODE_OK;
    do
    {
        // Read modem event
        lr1121_modem_event_fields_t current_event;
        rc_event = lr1121_modem_get_event( context, &current_event );
        if( rc_event == LR1121_MODEM_RESPONSE_CODE_OK )
        {
            switch( current_event.event_type )
            {
            case LR1121_MODEM_LORAWAN_EVENT_RESET:

                HAL_DBG_TRACE_MSG_COLOR( "Event received: RESET\r\n", HAL_DBG_TRACE_COLOR_BLUE );

                ASSERT_SMTC_MODEM_RC( lr1121_modem_system_cfg_lfclk( context, LR1121_MODEM_SYSTEM_LFCLK_XTAL, true ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_crystal_error( context, 50 ) );
                get_and_print_crashlog( context );
#if( !USE_LR11XX_CREDENTIALS )
                // Set user credentials
                HAL_DBG_TRACE_INFO( "###### ===== LR1121 SET EUI and KEYS ==== ######\r\n" );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_dev_eui( context, user_dev_eui ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_join_eui( context, user_join_eui ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_app_key( context, user_app_key ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_nwk_key( context, user_nwk_key ) );
                uint8_t tmp_pin[4] = { 0 };  // The chip_pin is not used if we use custom credentials
                print_lorawan_credentials( user_dev_eui, user_join_eui, tmp_pin, USE_LR11XX_CREDENTIALS );
#else
                // Get internal credentials
                uint8_t tmp_join_eui[8] = { 0 };
                uint8_t tmp_app_key[16] = { 0 };  // The app_key cannot be accessed in case of internal credentials use
                ASSERT_SMTC_MODEM_RC( lr1121_modem_system_read_uid( context, chip_eui ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_system_read_pin( context, chip_pin ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_join_eui( context, tmp_join_eui ) );
                print_lorawan_keys( chip_eui, tmp_join_eui, tmp_app_key, tmp_app_key, chip_pin,
                                    USE_LR11XX_CREDENTIALS );
#endif

                // Set user region
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_region( context, LORAWAN_REGION_USED ) );
                print_lorawan_region( LORAWAN_REGION_USED );

                // Force certification mode
                lr1121_modem_certification_mode_t actual_cerification_mode = LR1121_MODEM_CERTIFICATION_MODE_DISABLE;
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_certification_mode( context, &actual_cerification_mode ) );
                if( actual_cerification_mode != LR1121_MODEM_CERTIFICATION_MODE_ENABLE )
                {
                    ASSERT_SMTC_MODEM_RC(
                        lr1121_modem_set_certification_mode( context, LR1121_MODEM_CERTIFICATION_MODE_ENABLE ) );
                }

                // Schedule a Join LoRaWAN network
                ASSERT_SMTC_MODEM_RC( lr1121_modem_join( context ) );
                HAL_DBG_TRACE_INFO( "###### ===== JOINING ==== ######\r\n\r\n" );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_ALARM:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: ALARM\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                // Send periodical empty uplink
                send_empty_uplink( LR1121_MODEM_UPLINK_UNCONFIRMED );
                // Restart periodical uplink alarm
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_alarm_timer( context, PERIODICAL_UPLINK_DELAY_S ) );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_JOINED:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: JOINED\n", HAL_DBG_TRACE_COLOR_BLUE );
                HAL_DBG_TRACE_INFO( "Modem is now joined \r\n" );

                uint8_t adr_custom_list[16] = { 0 };
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_adr_profile(
                    context, LR1121_MODEM_ADR_PROFILE_NETWORK_SERVER_CONTROLLED, adr_custom_list ) );
                // Send first empty periodical uplink
                send_empty_uplink( LR1121_MODEM_UPLINK_UNCONFIRMED );

                // start ALC sync service
                ASSERT_SMTC_MODEM_RC( lr1121_modem_alc_sync_start_service( context ) );

                // start periodical uplink alarm
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_alarm_timer( context, PERIODICAL_UPLINK_DELAY_S ) );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_TX_DONE:
            {
                const lr1121_modem_tx_done_event_t tx_done_event_data =
                    ( lr1121_modem_tx_done_event_t )( current_event.data >> 8 );
                HAL_DBG_TRACE_MSG_COLOR( "Event received: TXDONE\n\n", HAL_DBG_TRACE_COLOR_BLUE );

                HAL_DBG_TRACE_MSG( "TX DATA     : " );

                switch( tx_done_event_data )
                {
                case LR1121_MODEM_TX_NOT_SENT:
                {
                    HAL_DBG_TRACE_PRINTF( " NOT SENT" );
                    break;
                }
                case LR1121_MODEM_CONFIRMED_TX:
                {
                    HAL_DBG_TRACE_PRINTF( " CONFIRMED - ACK" );
                    break;
                }
                case LR1121_MODEM_UNCONFIRMED_TX:
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

            case LR1121_MODEM_LORAWAN_EVENT_DOWN_DATA:
            {
                uint8_t rx_payload[LORAWAN_APP_DATA_MAX_SIZE] = { 0 };  // Buffer for rx payload
                uint8_t rx_payload_size                       = 0;      // Size of the payload in the rx_payload buffer
                lr1121_modem_downlink_metadata_t rx_metadata  = { 0 };  // Metadata of downlink
                uint8_t                          rx_remaining = 0;      // Remaining downlink payload in modem

                HAL_DBG_TRACE_MSG_COLOR( "Event received: DOWNDATA\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                // Get downlink data
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_downlink_data_size( context, &rx_payload_size, &rx_remaining ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_downlink_data( context, rx_payload, rx_payload_size ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_downlink_metadata( context, &rx_metadata ) );
                HAL_DBG_TRACE_PRINTF( "Data received on port %u\n", rx_metadata.fport );
                HAL_DBG_TRACE_ARRAY( "Received payload", rx_payload, rx_payload_size );
            }
            break;

            case LR1121_MODEM_LORAWAN_EVENT_JOIN_FAIL:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: JOINFAIL\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_LINK_CHECK:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: LINK_CHECK\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_CLASS_B_PING_SLOT_INFO:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: CLASS_B_PING_SLOT_INFO\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_CLASS_B_STATUS:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: CLASS_B_STATUS\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_LORAWAN_MAC_TIME:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: LORAWAN MAC TIME\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_NEW_MULTICAST_SESSION_CLASS_C:
            {
                HAL_DBG_TRACE_MSG_COLOR( "Event received: NEW MULTICAST CLASS_C\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                const uint8_t mc_group_id = ( uint8_t )( current_event.data >> 1 );
                get_and_print_multicast_class_c_group_information( context, mc_group_id );
                break;
            }

            case LR1121_MODEM_LORAWAN_EVENT_NEW_MULTICAST_SESSION_CLASS_B:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: NEW MULTICAST CLASS_B\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                const uint8_t mc_group_id = ( uint8_t )( current_event.data >> 1 );
                get_and_print_multicast_class_b_group_information( context, mc_group_id );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_C:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: STOP MULTICAST CLASS_C\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_B:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: STOP MULTICAST CLASS_B\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;
            case LR1121_MODEM_LORAWAN_EVENT_ALC_SYNC_TIME:
            {
                HAL_DBG_TRACE_MSG_COLOR( "Event received: ALC SYNC TIME\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;
            }
            case LR1121_MODEM_LORAWAN_EVENT_FUOTA_DONE:
            {
                HAL_DBG_TRACE_MSG_COLOR( "Event received: FUOTA DONE\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                HAL_DBG_TRACE_PRINTF( "  --> FUOTA status %02x\n", ( uint8_t )( current_event.data >> 8 ) );
                break;
            }
            case LR1121_MODEM_LORAWAN_EVENT_REGIONAL_DUTY_CYCLE:
            {
                HAL_DBG_TRACE_MSG_COLOR( "Event received: REGIONAL DUTY CYCLE\r\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;
            }
            default:
                HAL_DBG_TRACE_INFO( "Event not handled 0x%02x\n", current_event.event_type );
                break;
            }
        }
    } while( rc_event != LR1121_MODEM_RESPONSE_CODE_NO_EVENT );
}

static void user_button_callback( void* context )
{
    ( void ) context;  // Not used in the example - avoid warning

    static uint32_t last_press_timestamp_ms = 0;

    // Debounce the button press, avoid multiple triggers
    if( ( int32_t )( hal_rtc_get_time_ms( ) - last_press_timestamp_ms ) > 500 )
    {
        last_press_timestamp_ms = hal_rtc_get_time_ms( );
        user_button_is_press    = true;
    }
}

static lr1121_modem_response_code_t send_empty_uplink( const lr1121_modem_uplink_type_t tx_confirmed )
{
    lr1121_modem_response_code_t modem_response_code = LR1121_MODEM_RESPONSE_CODE_OK;
    int32_t                      duty_cycle;

    lr1121_modem_get_duty_cycle_status( &lr1121, &duty_cycle );

    if( duty_cycle < 0 )
    {
        HAL_DBG_TRACE_INFO( "DUTY CYCLE, NEXT UPLINK AVAILABLE in %d milliseconds \r\n\r\n", -duty_cycle );
        return modem_response_code;
    }

    /* Send empty frame in order to offer a downlink opportunity to the network */
    modem_response_code = lr1121_modem_request_empty_tx( &lr1121, false, 1, tx_confirmed );

    if( modem_response_code == LR1121_MODEM_RESPONSE_CODE_OK )
    {
        HAL_DBG_TRACE_INFO( "lr1121 MODEM-E REQUEST EMPTY TX \r\n" );
    }
    else
    {
        HAL_DBG_TRACE_ERROR( "lr1121 MODEM-E REQUEST EMPTY TX ERROR CMD, modem_response_code : %d \r\n\r\n",
                             modem_response_code );
    }
    return modem_response_code;
}

void get_and_print_multicast_class_b_group_information( void* context, uint8_t group_id )
{
    lr1121_modem_multicast_class_b_status_t mc_b_status = { 0 };
    lr1121_modem_get_multicast_class_b_session_status( context, group_id, &mc_b_status );

    HAL_DBG_TRACE_PRINTF( "-> multicast group ID: %u\n", group_id );
    HAL_DBG_TRACE_PRINTF( "-> is_session_started: %u\n", mc_b_status.is_session_started );
    HAL_DBG_TRACE_PRINTF( "-> downlink_frequency: %u\n", mc_b_status.downlink_frequency );
    HAL_DBG_TRACE_PRINTF( "-> downlink_datarate: %u\n", mc_b_status.downlink_datarate );
    HAL_DBG_TRACE_PRINTF( "-> is_session_waiting_for_beacon: %u\n", mc_b_status.is_session_waiting_for_beacon );
    HAL_DBG_TRACE_PRINTF( "-> ping_slot_periodicity: %u\n", mc_b_status.ping_slot_periodicity );
}

void get_and_print_multicast_class_c_group_information( void* context, uint8_t group_id )
{
    lr1121_modem_multicast_class_c_status_t mc_c_status = { 0 };
    lr1121_modem_get_multicast_class_c_session_status( context, group_id, &mc_c_status );

    HAL_DBG_TRACE_PRINTF( "-> multicast group ID: %u\n", group_id );
    HAL_DBG_TRACE_PRINTF( "-> is_session_started: %u\n", mc_c_status.is_session_started );
    HAL_DBG_TRACE_PRINTF( "-> downlink_frequency: %u\n", mc_c_status.downlink_frequency );
    HAL_DBG_TRACE_PRINTF( "-> downlink_datarate: %u\n", mc_c_status.downlink_datarate );
}

/* --- EOF ------------------------------------------------------------------ */

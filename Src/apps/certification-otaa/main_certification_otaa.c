/**
 * @ingroup   apps_certification
 * @file      main_certification.c
 *
 * @brief     modem_e Modem-E certification implementation
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
 * @addtogroup apps_certification
 * modem_e Modem-E certification device implementation
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

#if( !USE_LR11XX_CREDENTIALS )
/**
 * @brief Stack credentials
 */
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

#if( USE_LR11XX_CREDENTIALS )
/**
 * @brief Internal credentials
 */
static uint8_t chip_eui[8] = { 0 };
static uint8_t chip_pin[4] = { 0 };
#endif

extern modem_e_t modem_e;

static uint8_t                          rx_payload[LORAWAN_APP_DATA_MAX_SIZE] = { 0 };  // Buffer for rx payload
static uint8_t                          rx_payload_size = 0;      // Size of the payload in the rx_payload buffer
static modem_e_downlink_metadata_t rx_metadata     = { 0 };  // Metadata of downlink
static uint8_t                          rx_remaining    = 0;      // Remaining downlink payload in modem

static volatile bool                              user_button_is_press = false;  // Flag for button status
static volatile modem_e_certification_mode_t certif_running       = false;  // Certification mode enabled
static uint32_t                                   uplink_counter       = 0;      // uplink sent counter
static uint32_t                                   confirmed_counter    = 0;      // confirmed uplink counter

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/**
 * @brief User callback for button EXTI
 *
 * @param context Define by the user at the init
 */
static void user_button_callback( void* context );

/**
 * @brief Enable or disable certification mode
 *
 * @param context Define by the user at the init
 */
static void main_handle_button_pushed( void* context );

/**
 * @brief Send the 32bits uplink counter and 32bits confirmed counter on chosen port
 */
static void send_uplinks_counter_on_port( uint8_t port );

/**
 * @brief Send tx_frame_buffer on choosen port
 *
 */
static modem_e_response_code_t send_frame( const uint8_t* tx_frame_buffer, const uint8_t tx_frame_buffer_size,
                                                uint8_t port, const modem_e_uplink_type_t tx_confirmed );

/**
 * @brief Process received events
 *
 */
static void event_process( void* context );

/**
 * @brief Set credentials and region
 * 
*/
static void set_credentials_and_region(void* context);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

int main( void )
{
    // Configure all the microprocessor peripherals (clock, gpio, timer, ...)
    hal_mcu_init( );
    hal_mcu_init_periph( );

    leds_blink( LED_ALL_MASK, 250, 4, true );

    HAL_DBG_TRACE_MSG( "\n\n" );
    HAL_DBG_TRACE_INFO( "###### ===== Certification example is starting ==== ######\n\n\n" );

    // Disable IRQ to avoid unwanted behavior during init
    hal_mcu_disable_irq( );

    // Configure Nucleo blue button as EXTI
    hal_gpio_irq_t nucleo_blue_button = {
        .pin      = EXTI_BUTTON,
        .context  = NULL,                  // context passed to the callback - not used in this example
        .callback = user_button_callback,  // callback called when EXTI is triggered
    };
    hal_gpio_init_in( EXTI_BUTTON, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_FALLING, &nucleo_blue_button );

    // Configure event callback on interrupt
    hal_gpio_irq_t event_callback = {
        .pin      = modem_e.event.pin,
        .context  = &modem_e,        // context passed to the callback
        .callback = event_process,  // callback called when event pin is triggered
    };
    hal_gpio_init_in( modem_e.event.pin, HAL_GPIO_PULL_MODE_DOWN, HAL_GPIO_IRQ_MODE_RISING, &event_callback );

    modem_e_system_reboot( &modem_e, false );

    // Init done: enable interruption
    hal_mcu_enable_irq( );
    HAL_DBG_TRACE_MSG( "Initialization done\n\n" );

    /* Board is initialized */
    leds_blink( LED_TX_MASK, 100, 20, true );
    while( 1 )
    {
        // Check button
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
    // Continue to read modem events until all of them have been processed.
    modem_e_response_code_t rc_event = MODEM_E_RESPONSE_CODE_OK;
    do
    {
        // Read modem event
        modem_e_event_fields_t current_event;
        rc_event = modem_e_get_event( context, &current_event );
        if( rc_event == MODEM_E_RESPONSE_CODE_OK )
        {
            switch( current_event.event_type )
            {
            case MODEM_E_LORAWAN_EVENT_RESET:

                HAL_DBG_TRACE_MSG_COLOR( "Event received: RESET\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                ASSERT_SMTC_MODEM_RC( modem_e_board_init(context));
                get_and_print_crashlog( context );

                ASSERT_SMTC_MODEM_RC( modem_e_get_certification_mode(
                    context, ( modem_e_certification_mode_t* ) &certif_running ) );
                print_certification( certif_running );
                /* If certification mode is disabled,
                set the credentials if needed, print them, and launch the join procedure */
                if( certif_running == MODEM_E_CERTIFICATION_MODE_DISABLE )
                {
                    set_credentials_and_region(context);
                    // Schedule a LoRaWAN network JoinRequest.
                    ASSERT_SMTC_MODEM_RC( modem_e_join( context ) );
                    HAL_DBG_TRACE_INFO( "###### ===== JOINING ==== ######\n\n\n" );
                }
                // Otherwise, just print the credentials
                else
                {
                    uint8_t tmp_join_eui[8] = { 0 };
#if( !USE_LR11XX_CREDENTIALS )
                    uint8_t tmp_pin[4] = { 0 };  // The chip_pin is not used if we use custom credentials
                    uint8_t tmp_dev_eui[8] = { 0 };
                    ASSERT_SMTC_MODEM_RC( modem_e_get_otaa_dev_eui( context, tmp_dev_eui ) );
                    ASSERT_SMTC_MODEM_RC( modem_e_get_otaa_join_eui( context, tmp_join_eui ) );
                    print_lorawan_credentials( tmp_dev_eui, tmp_join_eui, tmp_pin, USE_LR11XX_CREDENTIALS );
#else
                    ASSERT_SMTC_MODEM_RC( modem_e_system_read_uid( context, chip_eui ) );
                    ASSERT_SMTC_MODEM_RC( modem_e_system_read_pin( context, chip_pin ) );
                    ASSERT_SMTC_MODEM_RC( modem_e_get_otaa_join_eui( context, tmp_join_eui ) );
                    print_lorawan_credentials( chip_eui, tmp_join_eui, chip_pin, USE_LR11XX_CREDENTIALS );
#endif
                    modem_e_regions_t modem_region = MODEM_E_LORAWAN_REGION_EU868;  // Init to EU868
                    get_and_print_lorawan_region_from_modem( context, &modem_region );

                    // Check for alignment between the region and JoinEUI configured in the Modem-E and those expected by the application.
                    // There are two possible mismatches:
                    // 1. The region configured in the Modem-E differs from the one used by the application (LORAWAN_REGION_USED).
                    // 2. The JoinEUI stored in the Modem-E differs from the one expected by the application,
                    //    and the application is not using LR11XX default credentials (USE_LR11XX_CREDENTIALS == false).
                    // This typically indicates that the certification mode from a different region was not properly stopped
                    // before flashing a new firmware.
                    // In such cases, the ongoing join process must be stopped, the correct region and credentials must be set,
                    // and the certification mode must be restarted.
                    if( (modem_region != LORAWAN_REGION_USED) || 
                        ((memcmp(user_join_eui, tmp_join_eui, sizeof(user_join_eui)) != 0) && (!USE_LR11XX_CREDENTIALS)) )
                    {
                        HAL_DBG_TRACE_ERROR(
                            "Configuration mismatch:\n"
                            "- Region Modem-E: 0x%02x vs App: 0x%02x\n"
                            "- JoinEUI match: %s\n"
                            "- Using LR11XX credentials: %s\n",
                            modem_region, LORAWAN_REGION_USED,
                            (memcmp(user_join_eui, tmp_join_eui, sizeof(user_join_eui)) == 0) ? "YES" : "NO",
                            USE_LR11XX_CREDENTIALS ? "YES" : "NO" );

                        HAL_DBG_TRACE_MSG("Mismatch detected, stopping join process...\n");
                        modem_e_leave_network( context );

                        HAL_DBG_TRACE_MSG("Disabling certification mode...\n");
                        modem_e_set_certification_mode( context, MODEM_E_CERTIFICATION_MODE_DISABLE );

                        HAL_DBG_TRACE_MSG("Reconfiguring credentials and region...\n\n");
                        set_credentials_and_region( context );

                        HAL_DBG_TRACE_MSG("Re-enabling certification mode...\n");
                        modem_e_set_certification_mode( context, MODEM_E_CERTIFICATION_MODE_ENABLE );
                    }
                }
                break;

            case MODEM_E_LORAWAN_EVENT_ALARM:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: ALARM\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                if( certif_running == MODEM_E_CERTIFICATION_MODE_ENABLE )
                {
                    modem_e_clear_alarm_timer( context );
                }
                else
                {
                    // Send periodical uplink on port 101
                    send_uplinks_counter_on_port( 101 );
                    // Restart periodical uplink alarm
                    ASSERT_SMTC_MODEM_RC( modem_e_set_alarm_timer( context, PERIODICAL_UPLINK_DELAY_S ) );
                }
                break;

            case MODEM_E_LORAWAN_EVENT_JOINED:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: JOINED\n", HAL_DBG_TRACE_COLOR_BLUE );
                HAL_DBG_TRACE_INFO( "Modem is now joined \n\n" );


                uint8_t adr_custom_list[16] = { 0 };
                ASSERT_SMTC_MODEM_RC( modem_e_set_adr_profile(
                    context, MODEM_E_ADR_PROFILE_NETWORK_SERVER_CONTROLLED, adr_custom_list ) );

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
                // Get downlink data
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
                HAL_DBG_TRACE_MSG_COLOR( "Event received: REGIONAL DUTY CYCLE\r\n", HAL_DBG_TRACE_COLOR_BLUE );
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
            
            case MODEM_E_LORAWAN_EVENT_RESET_REQUEST:  // MODEM_E_LORAWAN_EVENT_RESET_REQUEST
                HAL_DBG_TRACE_MSG_COLOR( "Event received: RESET_REQUEST\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                modem_e_system_reboot( context, false );
                break;

            default:
                HAL_DBG_TRACE_INFO( "Event not handled 0x%02x\n", current_event.event_type );
                break;
            }
        }
    } while( rc_event != MODEM_E_RESPONSE_CODE_NO_EVENT );
}

static void set_credentials_and_region(void* context)
{
#if( !USE_LR11XX_CREDENTIALS )
    // Set user credentials
    HAL_DBG_TRACE_INFO( "###### ===== LR1121 SET EUI and KEYS ==== ######\n\n" );
    ASSERT_SMTC_MODEM_RC( modem_e_set_otaa_dev_eui( context, user_dev_eui ) );
    ASSERT_SMTC_MODEM_RC( modem_e_set_otaa_join_eui( context, user_join_eui ) );
    ASSERT_SMTC_MODEM_RC( modem_e_set_otaa_app_key( context, user_app_key ) );
    ASSERT_SMTC_MODEM_RC( modem_e_set_otaa_nwk_key( context, user_nwk_key ) );
    uint8_t tmp_pin[4] = { 0 };  // The chip_pin is not used if we use custom credentials
    print_lorawan_credentials( user_dev_eui, user_join_eui, tmp_pin, USE_LR11XX_CREDENTIALS );
#else
    // Get internal credentials
    uint8_t tmp_join_eui[8] = { 0 };
    ASSERT_SMTC_MODEM_RC( modem_e_system_read_uid( context, chip_eui ) );
    ASSERT_SMTC_MODEM_RC( modem_e_system_read_pin( context, chip_pin ) );
    ASSERT_SMTC_MODEM_RC( modem_e_get_otaa_join_eui( context, tmp_join_eui ) );
    print_lorawan_credentials( chip_eui, tmp_join_eui, chip_pin, USE_LR11XX_CREDENTIALS );
#endif
    // Set user region
    ASSERT_SMTC_MODEM_RC( modem_e_set_region( context, LORAWAN_REGION_USED ) );
    print_lorawan_region( LORAWAN_REGION_USED );

}

static void user_button_callback( void* context )
{
    HAL_DBG_TRACE_INFO( "Button pushed\n" );

    ( void ) context;  // Not used in the example - avoid warning

    static uint32_t last_press_timestamp_ms = 0;

    // Debounce the button press, avoid multiple triggers
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
        ASSERT_SMTC_MODEM_RC( modem_e_join( context ) );
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
    // Send uplink and confirmed counter
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
    // Increment uplink counter
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
        /* Send empty frame in order to flush MAC commands */
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

/*!
 * @ingroup   apps_multicast
 * @file      main_multicast.c
 *
 * @brief     lr1121 Modem-E multicast implementation
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
 * @addtogroup apps_multicast
 * lr1121 Modem-E multicast device implementation
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
#include "smtc_utilities.h"
#include "apps_utilities.h"
#include "lr1121_modem_system_types.h"
#include "lr1121_modem_helper.h"

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

/*!
 * @brief Multicast session class definition
 */
#define LORAWAN_CLASS_B 0x01
#define LORAWAN_CLASS_C 0x02

/**
 * @brief Watchdog counter reload value during sleep (The period must be lower than MCU watchdog period (here 20s))
 */
#define WATCHDOG_RELOAD_PERIOD_MS 20000

/**
 * @brief Pin of the nucleo button
 */
#define EXTI_BUTTON PC_13

/*!
 * @brief User application data buffer size
 */
#define LORAWAN_APP_DATA_MAX_SIZE 242

/*!
 * @brief LoRaWAN regulatory region.
 * One of:
 * LR1121_LORAWAN_REGION_AS923_GRP1
 * LR1121_LORAWAN_REGION_AS923_GRP2
 * LR1121_LORAWAN_REGION_AS923_GRP3
 * LR1121_LORAWAN_REGION_AS923_GRP4
 * LR1121_LORAWAN_REGION_AU915
 * LR1121_LORAWAN_REGION_CN470
 * LR1121_LORAWAN_REGION_EU868
 * LR1121_LORAWAN_REGION_IN865
 * LR1121_LORAWAN_REGION_KR920
 * LR1121_LORAWAN_REGION_RU864
 * LR1121_LORAWAN_REGION_US915
 */
#define LORAWAN_REGION_USED LR1121_LORAWAN_REGION_EU868

/*!
 * @brief Multicast class
 * One of:
 * LORAWAN_CLASS_B
 * LORAWAN_CLASS_C
 */
#define MULTICAST_SESSION_CLASS LORAWAN_CLASS_B

/*!
 * @brief Number of multicast session (1 or 2).
 */
#define NUMBER_MULTICAST_SESSION 2

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/**
 * @brief Stack credentials
 */
#if( USE_LR11XX_CREDENTIALS == 0 )
static const uint8_t user_dev_eui[8]  = LORAWAN_DEVICE_EUI;
static const uint8_t user_join_eui[8] = LORAWAN_JOIN_EUI;
static const uint8_t user_nwk_key[16] = LORAWAN_NWK_KEY;
static const uint8_t user_app_key[16] = LORAWAN_APP_KEY;
#endif

/*!
 * @brief Multicast session keys.
 * Each element in the array represents a multicast session and contains a 3-dimensional array for the session keys:
 * - grp_addr: 4 bytes
 * - nwk_skey: 16 bytes
 * - app_skey: 16 bytes
 */
static const uint8_t MULTICAST_KEYS[NUMBER_MULTICAST_SESSION][3][16] = {
    {
        /* Session 1 */
        { 0x01, 0x02, 0x03, 0x04 }, /* grp_addr (4 bytes) */
        { 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
          0x14 }, /* nwk_skey (16 bytes) */
        { 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
          0x24 } /* app_skey (16 bytes) */
    },
    {
        /* Session 2 */
        { 0x25, 0x26, 0x27, 0x28 }, /* grp_addr (4 bytes) */
        { 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
          0x38 }, /* nwk_skey (16 bytes) */
        { 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
          0x49 } /* app_skey (16 bytes) */
    }
};

/**
 * @brief Ping slot periodicity for multicast class B session
 * One of:
 *  LR1121_MODEM_CLASS_B_PING_SLOT_1_S
 *  LR1121_MODEM_CLASS_B_PING_SLOT_2_S
 *  LR1121_MODEM_CLASS_B_PING_SLOT_4_S
 *  LR1121_MODEM_CLASS_B_PING_SLOT_8_S
 *  LR1121_MODEM_CLASS_B_PING_SLOT_16_S
 *  LR1121_MODEM_CLASS_B_PING_SLOT_32_S
 *  LR1121_MODEM_CLASS_B_PING_SLOT_64_S
 *  LR1121_MODEM_CLASS_B_PING_SLOT_128_S
 */
static const uint8_t MULTICAST_PING_SLOT_PERIODICITY[NUMBER_MULTICAST_SESSION] = {
    LR1121_MODEM_CLASS_B_PING_SLOT_8_S, LR1121_MODEM_CLASS_B_PING_SLOT_16_S
};

#if( MULTICAST_SESSION_CLASS == LORAWAN_CLASS_B )
/**
 * @brief Default multicast class b frequency per region opcode
 * The value 0 indicates that the multicast frequency should hop according to the beacon frequency.
 * The value -1 indicates that this configuration does not make sense for these frequency plans because they are not
 * specified.
 */
static const uint32_t DEFAULT_MULTICAST_FREQ[13] = { 869525000, 923400000, 0,         0,         494900000,
                                                     -1,        921600000, 916800000, 866550000, 923100000,
                                                     868900000, -1,        917500000 };

/**
 * @brief Default multicast class b datarate per region opcode
 * The value -1 indicates that this configuration does not make sense for these frequency plans because they are not
 * specified.
 */
static const uint8_t DEFAULT_MULTICAST_DR[13] = { 3, 3, 8, 8, 2, -1, 3, 3, 4, 3, 3, -1, 3 };
#else
/**
 * @brief Default multicast class c frequency per region opcode
 */
static const uint32_t DEFAULT_MULTICAST_FREQ[13] = { 869525000,  923200000, 923300000, 923300000, 492500000,
                                                     2423000000, 921400000, 916600000, 866550000, 921900000,
                                                     869100000,  505300000, 917300000 };
/**
 * @brief Default multicast class c datarate per region opcode
 */
static const uint32_t DEFAULT_MULTICAST_DR[13] = { 0, 2, 8, 8, 1, 0, 2, 2, 4, 0, 0, 0, 2 };
#endif

static const uint32_t MULTICAST_FREQUENCY = DEFAULT_MULTICAST_FREQ[LORAWAN_REGION_USED - 1];

static const uint8_t MULTICAST_DATARATE = DEFAULT_MULTICAST_DR[LORAWAN_REGION_USED - 1];

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */
extern lr1121_t      lr1121;
static volatile bool user_button_is_press = false;  // Flag for button status
static volatile bool unicast_ready        = false;  // Flag for class applied
static volatile bool multicast_started    = false;  // Flag for multicast started
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
 * @brief Send tx_frame_buffer on choosen port
 *
 */
static lr1121_modem_response_code_t send_frame( const uint8_t* tx_frame_buffer, const uint8_t tx_frame_buffer_size,
                                                uint8_t port, const lr1121_modem_uplink_type_t tx_confirmed );

/**
 * @brief Process received events
 *
 */
static void event_process( void* context );

/**
 * @brief Convert lr1121_modem_downlink_window_t to window name
 *
 */
const char* get_downlink_window_name( lr1121_modem_downlink_window_t window );
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
    HAL_DBG_TRACE_INFO( "###### ===== Multicast example is starting ==== ######\n\n\n" );

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
        .pin      = lr1121.event.pin,
        .context  = &lr1121,        // context passed to the callback
        .callback = event_process,  // callback called when event pin is triggered
    };
    hal_gpio_init_in( lr1121.event.pin, HAL_GPIO_PULL_MODE_NONE, HAL_GPIO_IRQ_MODE_RISING, &event_callback );

    // Flush events before enabling irq
    lr1121_modem_board_event_flush( &lr1121 );

    // Init done: enable interruption
    hal_mcu_enable_irq( );

    /* Board is initialized */
    leds_blink( LED_TX_MASK, 100, 20, true );
    HAL_DBG_TRACE_MSG( "Initialization done\n\n" );
    lr1121_modem_system_reboot( &lr1121, false );
    while( 1 )
    {
        // Check button
        if( user_button_is_press == true )
        {
            user_button_is_press = false;
            main_handle_button_pushed( &lr1121 );
        }

        if( MULTICAST_SESSION_CLASS == LR1121_LORAWAN_CLASS_B )
        {
            lr1121_modem_multicast_class_b_status_t multicast_status;
            ASSERT_SMTC_MODEM_RC( lr1121_modem_get_multicast_class_b_session_status( &lr1121, 0, &multicast_status ) );

            if( ( multicast_status.is_session_waiting_for_beacon == 0 ) &&
                ( multicast_status.is_session_started == 1 ) )
            {
                multicast_started = true;
                HAL_DBG_TRACE_INFO( "###### ===== BEACON RECEIVED ==== ######\n\n\n" );
                HAL_DBG_TRACE_PRINTF( "You can now send multicast downlinks\n\n" );
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
    // Continue to read modem events until all of them have been processed.
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

                HAL_DBG_TRACE_MSG_COLOR( "Event received: RESET\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_system_cfg_lfclk( context, LR1121_MODEM_SYSTEM_LFCLK_XTAL, true ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_crystal_error( context, 50 ) );
                get_and_print_crashlog( context );
                unicast_ready     = false;
                multicast_started = false;
#if( !USE_LR11XX_CREDENTIALS )
                // Set user credentials
                HAL_DBG_TRACE_INFO( "###### ===== LR1121 SET EUI and KEYS ==== ######\n\n" );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_dev_eui( context, user_dev_eui ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_join_eui( context, user_join_eui ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_app_key( context, user_app_key ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_nwk_key( context, user_nwk_key ) );
                uint8_t tmp_pin[4] = { 0 };  // The chip_pin is not used if we use custom credentials
                print_lorawan_credentials( user_dev_eui, user_join_eui, tmp_pin, USE_LR11XX_CREDENTIALS );
#else
                // Get internal credentials
                uint8_t tmp_join_eui[8] = { 0 };
                ASSERT_SMTC_MODEM_RC( lr1121_modem_system_read_uid( context, chip_eui ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_system_read_pin( context, chip_pin ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_join_eui( context, tmp_join_eui ) );
                print_lorawan_credentials( chip_eui, tmp_join_eui, chip_pin, USE_LR11XX_CREDENTIALS );
#endif

                // Set user region
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_region( context, LORAWAN_REGION_USED ) );
                print_lorawan_region( LORAWAN_REGION_USED );

                // Schedule a LoRaWAN network JoinRequest.
                ASSERT_SMTC_MODEM_RC( lr1121_modem_join( context ) );
                HAL_DBG_TRACE_INFO( "###### ===== JOINING ==== ######\n\n\n" );

                break;

            case LR1121_MODEM_LORAWAN_EVENT_ALARM:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: ALARM\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_JOINED:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: JOINED\n", HAL_DBG_TRACE_COLOR_BLUE );
                HAL_DBG_TRACE_INFO( "Modem is now joined \n\n" );

                uint8_t adr_custom_list[16] = { 0 };
                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_adr_profile(
                    context, LR1121_MODEM_ADR_PROFILE_NETWORK_SERVER_CONTROLLED, adr_custom_list ) );

                ASSERT_SMTC_MODEM_RC( lr1121_modem_set_class( context, MULTICAST_SESSION_CLASS ) );
                for( uint8_t i = 0; i < NUMBER_MULTICAST_SESSION; i++ )
                {
                    uint32_t grp_addr = ( MULTICAST_KEYS[i][0][0] << 24 ) | ( MULTICAST_KEYS[i][0][1] << 16 ) |
                                        ( MULTICAST_KEYS[i][0][2] << 8 ) | MULTICAST_KEYS[i][0][3];
                    ASSERT_SMTC_MODEM_RC( lr1121_modem_set_multicast_group_config(
                        context, i, grp_addr, MULTICAST_KEYS[i][1], MULTICAST_KEYS[i][2] ) );
                }
                if( MULTICAST_SESSION_CLASS == LR1121_LORAWAN_CLASS_C )
                {
                    unicast_ready = true;
                    // Send an uplink to enable the unicast class C session on NS
                    uint8_t buff[8] = { 0 };
                    ASSERT_SMTC_MODEM_RC( send_frame( buff, 8, 10, LR1121_MODEM_UNCONFIRMED_TX ) );
                }
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
                if( unicast_ready )
                {
                    HAL_DBG_TRACE_INFO(
                        "Device unicast session setup - You can push the blue button to start the multicast "
                        "session\n\n\n" );
                }

                break;
            }

            case LR1121_MODEM_LORAWAN_EVENT_DOWN_DATA:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: DOWNDATA\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                // Get downlink data
                uint8_t rx_payload[LORAWAN_APP_DATA_MAX_SIZE] = { 0 };  // Buffer for rx payload
                uint8_t rx_payload_size                       = 0;      // Size of the payload in the rx_payload buffer
                lr1121_modem_downlink_metadata_t rx_metadata  = { 0 };  // Metadata of downlink
                uint8_t                          rx_remaining = 0;      // Remaining downlink payload in modem
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_downlink_data_size( context, &rx_payload_size, &rx_remaining ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_downlink_data( context, rx_payload, rx_payload_size ) );
                ASSERT_SMTC_MODEM_RC( lr1121_modem_get_downlink_metadata( context, &rx_metadata ) );
                HAL_DBG_TRACE_PRINTF( "Data received on windows %s\n", get_downlink_window_name( rx_metadata.window ) );
                HAL_DBG_TRACE_ARRAY( "Received payload", rx_payload, rx_payload_size );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_JOIN_FAIL:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: JOINFAIL\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_LINK_CHECK:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: LINK_CHECK\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_CLASS_B_PING_SLOT_INFO:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: CLASS_B_PING_SLOT_INFO\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_CLASS_B_STATUS:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: CLASS_B_STATUS\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                if( ( current_event.data ) && ( MULTICAST_SESSION_CLASS == LR1121_LORAWAN_CLASS_B ) )
                {
                    unicast_ready = true;
                    // Send an uplink to enable the unicast class C session on NS
                    uint8_t buff[8] = { 0 };
                    ASSERT_SMTC_MODEM_RC( send_frame( buff, 8, 10, LR1121_MODEM_UNCONFIRMED_TX ) );
                }
                break;

            case LR1121_MODEM_LORAWAN_EVENT_LORAWAN_MAC_TIME:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: LORAWAN MAC TIME\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_NEW_MULTICAST_SESSION_CLASS_C:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: MULTICAST CLASS_C STOP\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_NEW_MULTICAST_SESSION_CLASS_B:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: MULTICAST CLASS_B STOP\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_C:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: New MULTICAST CLASS_C\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;

            case LR1121_MODEM_LORAWAN_EVENT_NO_MORE_MULTICAST_SESSION_CLASS_B:
                HAL_DBG_TRACE_MSG_COLOR( "Event received: New MULTICAST CLASS_B\n\n", HAL_DBG_TRACE_COLOR_BLUE );
                break;
            default:
                HAL_DBG_TRACE_INFO( "Event not handled 0x%02x\n", current_event.event_type );
                break;
            }
        }
    } while( rc_event != LR1121_MODEM_RESPONSE_CODE_NO_EVENT );
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
    if( unicast_ready )
    {
        // The device class is setup, we can start or stop the multicast session
        if( multicast_started )
        {
            // The multicast session is started, the action is to stop the session
            if( MULTICAST_SESSION_CLASS == LR1121_LORAWAN_CLASS_B )
            {
                ASSERT_SMTC_MODEM_RC( lr1121_modem_stop_all_session_multicast_class_b( context ) );
            }
            else
            {
                ASSERT_SMTC_MODEM_RC( lr1121_modem_stop_all_session_multicast_class_c( context ) );
            }
            multicast_started = false;
            HAL_DBG_TRACE_INFO( "###### ===== STOP MULTICAST SESSION(S) ==== ######\n\n\n" );
        }
        else
        {
            // The multicast session is not started, the action is to start the session
            for( uint8_t i = 0; i < NUMBER_MULTICAST_SESSION; i++ )
            {
                if( MULTICAST_SESSION_CLASS == LR1121_LORAWAN_CLASS_B )
                {
                    ASSERT_SMTC_MODEM_RC( lr1121_modem_start_session_multicast_class_b(
                        context, i, MULTICAST_FREQUENCY, MULTICAST_DATARATE, MULTICAST_PING_SLOT_PERIODICITY[i] ) );
                    HAL_DBG_TRACE_INFO( "###### ===== START MULTICAST SESSION n°%d ==== ######\n\n\n", i + 1 );
                    HAL_DBG_TRACE_PRINTF( "Wait for beacon reception...\n\n" );
                }
                else
                {
                    ASSERT_SMTC_MODEM_RC( lr1121_modem_start_session_multicast_class_c( context, i, MULTICAST_FREQUENCY,
                                                                                        MULTICAST_DATARATE ) );

                    HAL_DBG_TRACE_INFO( "###### ===== START MULTICAST SESSION n°%d ==== ######\n\n\n", i + 1 );
                    HAL_DBG_TRACE_PRINTF( "You can now send multicast downlinks\n\n" );
                }
            }
            multicast_started = true;
        }
    }
    else
    {
        multicast_started = false;
        HAL_DBG_TRACE_PRINTF( "UNICAST SESSION NOT READY\n\n" );
    }
}

static lr1121_modem_response_code_t send_frame( const uint8_t* tx_frame_buffer, const uint8_t tx_frame_buffer_size,
                                                uint8_t port, const lr1121_modem_uplink_type_t tx_confirmed )
{
    lr1121_modem_response_code_t modem_response_code = LR1121_MODEM_RESPONSE_CODE_OK;
    uint8_t                      tx_max_payload;
    int32_t                      duty_cycle;

    lr1121_modem_get_duty_cycle_status( &lr1121, &duty_cycle );

    if( duty_cycle < 0 )
    {
        HAL_DBG_TRACE_INFO( "DUTY CYCLE, NEXT UPLINK AVAILABLE in %d milliseconds \n\n\n", -duty_cycle );
        return modem_response_code;
    }

    modem_response_code = lr1121_modem_get_next_tx_max_payload( &lr1121, &tx_max_payload );
    if( modem_response_code != LR1121_MODEM_RESPONSE_CODE_OK )
    {
        HAL_DBG_TRACE_ERROR( "\n\n lr1121_modem_get_next_tx_max_payload RC : %d \n\n", modem_response_code );
    }

    if( tx_frame_buffer_size > tx_max_payload )
    {
        /* Send empty frame in order to flush MAC commands */
        HAL_DBG_TRACE_PRINTF( "\n\n APP DATA > MAX PAYLOAD AVAILABLE (%d bytes) \n\n", tx_max_payload );
        modem_response_code = lr1121_modem_request_tx( &lr1121, port, tx_confirmed, NULL, 0 );
    }
    else
    {
        modem_response_code =
            lr1121_modem_request_tx( &lr1121, port, tx_confirmed, tx_frame_buffer, tx_frame_buffer_size );
    }

    if( modem_response_code == LR1121_MODEM_RESPONSE_CODE_OK )
    {
        HAL_DBG_TRACE_INFO( "lr1121 MODEM-E REQUEST TX \n\n" );
        HAL_DBG_TRACE_MSG( "TX DATA     : " );
        print_hex_buffer( tx_frame_buffer, tx_frame_buffer_size );
        HAL_DBG_TRACE_MSG( "\n\n\n" )
    }
    else
    {
        HAL_DBG_TRACE_ERROR( "lr1121 MODEM-E REQUEST TX ERROR CMD, modem_response_code : %d \n\n\n",
                             modem_response_code );
    }
    return modem_response_code;
}

const char* get_downlink_window_name( lr1121_modem_downlink_window_t window )
{
    switch( window )
    {
    case LR1121_MODEM_DOWNLINK_WINDOW_RX1:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RX1";
    case LR1121_MODEM_DOWNLINK_WINDOW_RX2:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RX2";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXC:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXC";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP0:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP0";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP1:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP1";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP2:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP2";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP3:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXC_MULTICAST_GROUP3";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXB:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXB";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP0:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP0";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP1:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP1";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP2:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP2";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP3:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXB_MULTICAST_GROUP3";
    case LR1121_MODEM_DOWNLINK_WINDOW_RXBEACON:
        return "LR1121_MODEM_DOWNLINK_WINDOW_RXBEACON";
    default:
        return "UNKNOWN_WINDOW";
    }
}

/* --- EOF ------------------------------------------------------------------ */

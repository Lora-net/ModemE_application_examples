# Certification ABP Application

This example allows enabling or disabling certification mode using ABP (Activation By Personalization).

It should be noted that the ABP test **TP_A_XXX_MAC_104_BV_001_B** cannot pass with this example for two reasons:
*In step 4, LCTT expects the device to use the minimum allowed data rate (DR) after a reset.
*In step 6, LCTT expects the device to restore and reuse all default channels after a reset.
These steps imply that LCTT assumes a device using ABP will clear any previously received **LinkADRRequest** following a reset. However, in our implementation, this information is preserved. When a reset occurs with context save and restore enabled, the device continues to apply the **LinkADRRequest** values received prior to the reset.
From the network’s perspective, in ABP there is no distinction between a device reset and a wake‑up from deep sleep. From the Modem‑E point of view, both behaviors are effectively identical.

## 1. Description

At startup, this example configures the modem in certification mode and connects to the network using ABP with pre-provisioned session keys (DevAddr, NwkSKey, AppSKey).

When the LCTT sends a reset request (event `RESET_REQUEST`), the application saves the modem state to NVM before resetting the device. After reboot, the modem state is restored from NVM, preserving frame counters and session context.

Pressing the blue button on the Nucleo board toggles the certification mode on or off.

## 2. Configuration

### 2.1. LoRaWAN configuration

Several constants are defined at the top of `main_certification_abp.c`, `common_app_configuration.h`, or `lorawan_commissioning.h`. Their values can be set to define the LoRaWAN configuration of the application.

| Constant              | Comments |
| --------------------- | -------- |
| `PERIODICAL_UPLINK_DELAY_S`  | Periodical uplink alarm delay in seconds. |
| `EXTI_BUTTON` | Pin name of the button. |
| `LORAWAN_APP_DATA_MAX_SIZE` | User application data buffer size. |
| `LORAWAN_REGION_USED` | LoRaWAN regulatory region. |

Supported values for `LORAWAN_REGION_USED`:

* `MODEM_E_LORAWAN_REGION_AS923_GRP1`
* `MODEM_E_LORAWAN_REGION_AS923_GRP2`
* `MODEM_E_LORAWAN_REGION_AS923_GRP3`
* `MODEM_E_LORAWAN_REGION_AS923_GRP4`
* `MODEM_E_LORAWAN_REGION_AU915`
* `MODEM_E_LORAWAN_REGION_CN470`
* `MODEM_E_LORAWAN_REGION_EU868 (default)`
* `MODEM_E_LORAWAN_REGION_IN865`
* `MODEM_E_LORAWAN_REGION_KR920`
* `MODEM_E_LORAWAN_REGION_RU864`
* `MODEM_E_LORAWAN_REGION_US915`
* `MODEM_E_LORAWAN_REGION_WW2G4`

### 2.2. ABP credentials

ABP session credentials must be configured in `Inc/apps/lorawan_commissioning/lorawan_commissioning.h`:

| Constant              | Comments |
| --------------------- | -------- |
| `LORAWAN_ABP_DEV_ADDR` | ABP device address (8 bytes). |
| `LORAWAN_ABP_NWK_SKEY` | ABP network session key (16 bytes). |
| `LORAWAN_ABP_APP_SKEY` | ABP application session key (16 bytes). |

These credentials must match the ones configured in LCTT.

## 2.3. Channel mask configuration (US915/AU915)

For the MODEM_E_LORAWAN_REGION_US915 and MODEM_E_LORAWAN_REGION_AU915 regions, it is possible to configure the channel mask before receiving any LinkADRRequest from the network server by using the SetChannelMask command.

By default, after an ABP activation, the device enables all 64 uplink channels at 125 kHz. However, in real deployments, gateways typically operate on a reduced number of channels (commonly 8 or 16). In such cases, restricting the active channels on the device side improves compatibility and avoids unnecessary transmissions on unused frequencies.

The SetChannelMask command allows selecting specific sub-bands of channels to be used by the device. This configuration is applied locally and takes effect immediately, even before any network-driven reconfiguration.

Example:
``` c
if( (LORAWAN_REGION_USED == MODEM_E_LORAWAN_REGION_US915) || (LORAWAN_REGION_USED == MODEM_E_LORAWAN_REGION_AU915))
{
    modem_e_channel_mask_configuration_t channel_configuration;
    channel_configuration.channel_mask_control = 5;
    channel_configuration.channel_mask[0] = 0x01;
    channel_configuration.channel_mask[1] = 0x00;
    modem_e_connect_set_channel_mask( context, &channel_configuration, 1 );
}
```
In this example:

channel_mask_control = 5 selects a specific sub-band (depending on the LoRaWAN regional specification).
channel_mask[] defines which channels are enabled within that sub-band.

## 3. Usage

### 3.1. Serial console

Information messages are displayed on the serial console.
Once the certification mode is enabled, no more information messages are displayed, because events are managed internally.

### 3.2. Reset request handling

When the LCTT sends a reset request, the application:
1. Saves the modem state snapshot to NVM
2. Resets the device
3. After reboot, restores the state from NVM (preserving frame counters and session)

This ensures ABP session continuity across resets during certification testing.

## 4. Miscellaneous

### 4.1. Application main loop

The application follows a state machine based on the reception of events:

- Reset event: Tries to restore state from NVM. If no snapshot is available, performs ABP connection with configured credentials and enables certification mode.
- Reset request event: Saves state to NVM, then resets the device.

Pressing the blue button toggles the certification mode.

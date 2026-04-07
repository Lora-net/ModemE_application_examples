# Certification OTAA Application

This example allows enabling or disabling certification mode using OTAA (Over-The-Air Activation), allowing the launch of the certification procedure with LCTT (LoRaWAN Certification Test Tool).

## 1. Description

At startup, this example functions like the periodic uplinks example, configuring the keys and then automatically sending join requests.  

After pressing the blue button on the Nucleo board, the alarm is disabled, the modem's certification mode is activated, and events are then managed internally by the modem.

Pressing the blue button again disables the certification mode, leaves the network, and restarts the join procedure.  

This example is intended to be used with the LCTT. We recommend using unique credentials configured in LCTT (to avoid joining a real Network Server) and activating the certification mode (by pressing the blue button on the Nucleo board) before running the tests on LCTT.

## 2. Configuration

### 2.1. LoRaWAN configuration

Several constants are defined at the top of `main_certification.c`, `common_app_configuration.h`, or `lorawan_commissioning.h`. Their values can be set to define the LoRaWAN configuration of the application.

| Constant              | Comments |
| --------------------- | -------- |
| `USE_LR11XX_CREDENTIALS` | Select if you want to use custom credentials (false) or internal credentials (true). It is recommended to use custom credentials in this example. |
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


### 2.2. Join configuration

You can provide your own EUIs in `Inc/apps/lorawan_commissioning/lorawan_commissioning.h` by setting `USE_PRODUCTION_KEYS` to any other value and by changing the values of `LORAWAN_DEVICE_EUI`, `LORAWAN_JOIN_EUI`, `LORAWAN_NWK_KEY` and `LORAWAN_APP_KEY`.

## 3. Usage

### 3.1. Serial console

Information messages are displayed on the serial console.  
Once the certification mode is enabled, no more information messages are displayed, because events are managed internally.

## 4. Miscellaneous

### 4.1. Application main loop

The application follows a relatively simple state machine based on the reception of events:

- Reset event: Configures the keys, the region, and starts the join procedure.
- Joined event: Immediately sends the number of uplinks sent and the number of uplinks confirmed in an uplink on port 101 and then sets the alarm.
- TxDone event: Increments the confirmed uplinks counter, if applicable.
- Alarm event: Sends the number of uplinks sent and the number of uplinks confirmed in an uplink on port 101 and reconfigures the alarm.

Pressing the blue button disables the alarm and enables the certification mode.  

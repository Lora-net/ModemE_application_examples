# Certification Application 

This example allows enabling or disabling certification mode, allowing the launch of the certification procedure with LCTT (LoRaWAN Certification Test Tool).

## 1. Description

At startup, this example functions like the periodic uplinks example, configuring the keys and then automatically sending join requests.  

After pressing the blue button on the Nucleo board, the alarm is disabled, the modem's certification mode is activated, and events are then managed internally by the modem.

Pressing the blue button again disables the certification mode, leaves the network, and restarts the join procedure.  

This example is intended to be used with the LCTT. We recommend using unique credentials configured in LCTT (to avoid joining a real Network Server) and activating the certification mode (by pressing the blue button on the Nucleo board) before running the tests on LCTT.

## 2. Configuration 

### 2.1. LoRaWAN configuration

Several constants are defined at the top of `main_certification.c`, their values can be set to define the LoRaWAN configuration of the application.

| Constant              | Comments |
| --------------------- | -------- |
| `USE_LR11XX_CREDENTIALS` | Select if you want to use custom credentials (false) or internal credentials (true). It is recommended to use custom credentials in this example. |
| `PERIODICAL_UPLINK_DELAY_S`  | Periodical uplink alarm delay in seconds. |
| `EXTI_BUTTON` | Pin name of the button. |
| `LORAWAN_APP_DATA_MAX_SIZE` | User application data buffer size. |
| `LORAWAN_REGION_USED` | LoRaWAN regulatory region. |

Supported values for `LORAWAN_REGION_USED`:

* `LR1121_LORAWAN_REGION_AS923_GRP1`
* `LR1121_LORAWAN_REGION_AS923_GRP2`
* `LR1121_LORAWAN_REGION_AS923_GRP3`
* `LR1121_LORAWAN_REGION_AS923_GRP4`
* `LR1121_LORAWAN_REGION_AU915`
* `LR1121_LORAWAN_REGION_CN470`
* `LR1121_LORAWAN_REGION_EU868 (default)`
* `LR1121_LORAWAN_REGION_IN865`
* `LR1121_LORAWAN_REGION_KR920`
* `LR1121_LORAWAN_REGION_RU864`
* `LR1121_LORAWAN_REGION_US915`


### 2.2. Join configuration

You can provide your own EUIs in `Inc/apps/lorawan_commissioning/lorawan_commissioning.h` by setting `USE_PRODUCTION_KEYS` to any other value and by changing the values of `LORAWAN_DEVICE_EUI`, `LORAWAN_JOIN_EUI`, `LORAWAN_NWK_KEY` and `LORAWAN_APP_KEY`.

## 3. Usage

### 3.1. Serial console

Information messages are displayed on the serial console.  
Once the certification mode is enabled, no more information messages are displayed, because events are managed internally.

### 3.2. Disable of certification mode

**IMPORTANT NOTE** The certification mode **MUST** be disabled explicitely when certification tests are terminated.

This is partcularly important when testing several configurations (like seeral LoRaWAN regions) by using several binaries, one after the others.
In this case, the certification mode **MUST** be explicitely disabled before flashing the next binary.
Indeed Modem-E maintains the configuration and will directly re-start the certification if not disabled, but using the previous configuration (and not the one from the newly flashed binary).

The explicit disabling of certification mode can be done by pressing the *User button* of the Nucleo board, and observing the message printed on the serial output indicating status of the certification mode.

## 4. Miscellaneous

### 4.1. Application main loop

The application follows a relatively simple state machine based on the reception of events:

- Reset event: Configures the keys, the region, and starts the join procedure.
- Joined event: Immediately sends the number of uplinks sent and the number of uplinks confirmed in an uplink on port 101 and then sets the alarm.
- TxDone event: Increments the confirmed uplinks counter, if applicable.
- Alarm event: Sends the number of uplinks sent and the number of uplinks confirmed in an uplink on port 101 and reconfigures the alarm.

Pressing the blue button disables the alarm and enables the certification mode.  



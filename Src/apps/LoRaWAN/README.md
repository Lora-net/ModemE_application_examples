# Simple LoRaWAN Class A Application 

This example demonstrates how to periodically send uplinks in a LoRaWAN Class A application and manually trigger uplinks by pressing a button.

## 1. Description

This application automatically submits a Join-Request to the LoRa Network Server.

Uplinks are sent periodically once the Join-Accept is received.

Pressing the NUCLEO blue button immediately sends an uplink on port 102.

## 2. Configuration 

### 2.1. LoRaWAN configuration

Several constants are defined at the top of `main_lorawan.c`, their values can be set to define the LoRaWAN configuration of the application.

| Constant              | Comments |
| --------------------- | -------- |
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

The LR1121 is pre-provisioned with a ChipEUI/DevEUI and a JoinEUI. The application will use these identifiers if the `USE_LR11XX_CREDENTIALS` from [lorawan_comissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h) is set to true. 

Alternatively, you can provide your own EUIs in `Inc/apps/lorawan_commissioning/lorawan_commissioning.h` by setting `USE_LR11XX_CREDENTIALS` to false and by changing the values of `LORAWAN_DEVICE_EUI`, `LORAWAN_JOIN_EUI`, `LORAWAN_NWK_KEY` and `LORAWAN_APP_KEY`.

## 3. Usage

### 3.1. Serial console

The application requires no user intervention after the static configuration options are set.

Information messages are displayed on the serial console, starting with the DevEUI, AppEUI/JoinEUI, and PIN, which you may need to register your device with the LoRa Cloud Device Join service.

## 4. Miscellaneous

### 4.1. Application main loop

The application implements a relatively simple state machine based on the reception of events:

- Reset event: Configures the keys, sets the region, and starts the join procedure.
- Joined event: Immediately sends the number of uplinks sent and the number of uplinks confirmed in an uplink on port 101 and then sets the alarm.
- TxDone event: Increments the confirmed uplinks counter, if applicable.
- Alarm event: Sends the number of uplinks sent and the number of uplinks confirmed in an uplink on port 101 and reconfigures the alarm.  

Pressing the blue button allows for the immediate sending of an uplink on port 102.
# LoRaWAN Class B Application 

This example demonstrates how to switch between Class A and Class B modes in a LoRaWAN application, allowing the device to communicate using scheduled ping slots in Class B mode.

## 1. Description

This application automatically submits a Join-Request to the LoRa Network Server.

Once the join accept is received, the application waits for the blue button to be pressed.

Pressing the NUCLEO blue button switches the device between Class A and Class B.

Once Class B is set, the modem will automatically send a DevTimeRequest and a PingSlotInfoRequest to the Network Server, then enable Class B after receiving a beacon.

## 2. Configuration 

### 2.1. LoRaWAN configuration

Several constants are defined at the top of `main_lorawan.c`, their values can be set to define the LoRaWAN configuration of the application.

| Constant              | Comments |
| --------------------- | -------- |
| `EXTI_BUTTON` | Pin name of the button. |
| `LORAWAN_APP_DATA_MAX_SIZE` | User application data buffer size. |
| `LORAWAN_REGION_USED` | LoRaWAN regulatory region. |
| `PING_SLOT_PERIODICITY` | Ping slot periodicity for class B unicast. |

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

Supported values for `PING_SLOT_PERIODICITY`:

* `LR1121_MODEM_CLASS_B_PING_SLOT_1_S`
* `LR1121_MODEM_CLASS_B_PING_SLOT_2_S`
* `LR1121_MODEM_CLASS_B_PING_SLOT_4_S`
* `LR1121_MODEM_CLASS_B_PING_SLOT_8_S`
* `LR1121_MODEM_CLASS_B_PING_SLOT_16_S`
* `LR1121_MODEM_CLASS_B_PING_SLOT_32_S`
* `LR1121_MODEM_CLASS_B_PING_SLOT_64_S`
* `LR1121_MODEM_CLASS_B_PING_SLOT_128_S`

### 2.2. Join configuration

The LR1121 is pre-provisioned with a ChipEUI/DevEUI and a JoinEUI. The application will use these identifiers if the `USE_LR11XX_CREDENTIALS` from [lorawan_comissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h) is set to true. 

Alternatively, you can provide your own EUIs in `Inc/apps/lorawan_commissioning/lorawan_commissioning.h` by setting `USE_LR11XX_CREDENTIALS` to false and changing the values of `LORAWAN_DEVICE_EUI`, `LORAWAN_JOIN_EUI`, `LORAWAN_NWK_KEY` and `LORAWAN_APP_KEY`.

## 3. Usage

### 3.1. Serial console

Information messages are displayed on the serial console, starting with the DevEUI, AppEUI/JoinEUI, and PIN that you might need to register your device with the LoRa Cloud Device Join service.

Once Class B is set and enabled, a message is displayed to inform you that Class B downlinks can now be received.

## 4. Miscellaneous

### 4.1. Application main loop

The application implements a state machine based on the reception of events:

- Reset event: Configures the keys, the region, and starts the join procedure.
- Joined event: Displays a message to inform you that you can push the blue button to switch to Class B.
- Class B status event: Immediately sends an uplink to inform the Network Server that Class B is running.
- TxDone event: Checks if Class B is enabled; if yes, displays a message to inform you that you can send Class B downlinks.

Pressing the blue button switches the device between Class A and Class B.  
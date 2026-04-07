# LR1121 modem Application Example

## 1. Description

The LR1121 modem application example project contains several simple examples highlighting LoRa Basic Modem-E features.  
This version of the examples supports LoRa Basics™ Modem-E **v2.1.0**.


### 1.1. Simple LoRaWAN Class A application

This application automatically joins the LoRa Network server and then sends uplinks periodically with the interval defined by `PERIODICAL_UPLINK_DELAY_S`.

Please read the [lorawan application documentation](Src/apps/LoRaWAN/README.md) for more details.

### 1.2. Certification OTAA application

This application switches the modem to certification mode once the NUCLEO blue button is pressed. It uses OTAA (Over-The-Air Activation) for joining the network.

Please read the [certification OTAA application documentation](Src/apps/certification-otaa/README.md) for more details.

### 1.2b. Certification ABP application

This application is similar to the OTAA certification example but uses ABP (Activation By Personalization) for network connection. On a reset request from LCTT, it saves the modem state to NVM before resetting and restores it afterwards.

Please read the [certification ABP application documentation](Src/apps/certification-abp/README.md) for more details.

### 1.3. LoRaWAN Class B application

This application switches the modem between class A and class B configurations once the NUCLEO blue button is pressed.

Please read the [class b application documentation](Src/apps/class_b/README.md) for more details.

### 1.4. Multicast Class B/C application

This application allows you to configure and launch one or more multicast sessions in Class B or Class C.

Please read the [multicast application documentation](Src/apps/multicast/README.md) for more details.

### 1.5. FUOTA (Firmware Update Over The Air) application

This example of Firmware Upgrade Over the Air (FUOTA) demonstrates how to run pre-certification suite with LoRaWAN Certification Test Tool (LCTT) for FUOTA

Please read the [fuota application documentation](Src/apps/fuota/README.md) for more details.

## 2. Hardware

The example applications are designed to run with the LR1121 Evaluation Kit hardware, which includes:

* NUCLEO-L476RG development board
* LR1121 shield

This project supports both versions of the LR1121 shield **with TCXO or without TCXO**

### 2.1. TCXO Configuration

To enable support for TCXO-based shields, you must modify the TCXO configuration macros in the [board-config.h](Inc/boards/board-config.h) file:

**Basic TCXO Enable/Disable:**
- Set `TCXO_ON_BOARD` to `1` if your shield includes a TCXO, or leave it as `0` (default) for non-TCXO boards.

**Advanced TCXO Parameters (configurable when `TCXO_ON_BOARD = 1`):**
- `BOARD_TCXO_SUPPLY_VOLTAGE`: Configure the TCXO supply voltage. Available options:
  - `MODEM_E_SYSTEM_TCXO_CTRL_1_6V` (1.6V)
  - `MODEM_E_SYSTEM_TCXO_CTRL_1_7V` (1.7V)
  - `MODEM_E_SYSTEM_TCXO_CTRL_1_8V` (1.8V) - **Default**
  - `MODEM_E_SYSTEM_TCXO_CTRL_2_2V` (2.2V)
  - `MODEM_E_SYSTEM_TCXO_CTRL_2_4V` (2.4V)
  - `MODEM_E_SYSTEM_TCXO_CTRL_2_7V` (2.7V)
  - `MODEM_E_SYSTEM_TCXO_CTRL_3_0V` (3.0V)
  - `MODEM_E_SYSTEM_TCXO_CTRL_3_3V` (3.3V)

- `BOARD_TCXO_WAKEUP_TIME`: Configure the TCXO wakeup time in milliseconds (default: 8ms)

**Example configuration for a 3.3V TCXO with 10ms wakeup time:**
```c
#define TCXO_ON_BOARD 1
#define BOARD_TCXO_SUPPLY_VOLTAGE MODEM_E_SYSTEM_TCXO_CTRL_3_3V
#define BOARD_TCXO_WAKEUP_TIME 10
```

## 3. Usage

Connect the NUCLEO board to a host PC via a USB cable. The USB connection will provide power for the LR1121 Evaluation Kit as well as serve as a serial communication link for the example application.

Use a terminal application configured with the following settings:

- Speed: 921600 baud
- Data bits: 8s
- Stop bits: 1
- Parity: None

Applications use the serial link to display information messages.  
Modem keys are defined in [lorawan_commissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h).
To use the LR1121 modem production keys, update the USE_LR11XX_CREDENTIALS definition

## 4. Build & Install

To build the example application for the STM32L476RG controller of the NUCLEO development board, the ARM GCC tool chain must be set up under your development environment. These examples were developed using GNU Arm Embedded Toolchain 10-2020-q4-major 10.2.1 20201103 (release)

To build the example applications with the GNU Arm Embedded Toolchain:

1. Run `make` from the `gcc` directory with the target application name as an argument:

```
$ make APP=lorawan
```

It is possible to provide additional compile-time definition through usage of `EXTRAFLAGS` make argument as follows:

```
$ make APP=lorawan EXTRAFLAGS='-D<TOKEN>=<VALUE>'
```

Where `<TOKEN>` is a macro name to configure with `<VALUE>`, for instance:

```bash
$ make APP=lorawan EXTRAFLAGS='-DLORAWAN_REGION_USED=MODEM_E_LORAWAN_REGION_EU868'
```

**Example: Building with TCXO configuration:**
```bash
$ make APP=lorawan EXTRAFLAGS='-DTCXO_ON_BOARD=1 -DBOARD_TCXO_SUPPLY_VOLTAGE=MODEM_E_SYSTEM_TCXO_CTRL_3_3V -DBOARD_TCXO_WAKEUP_TIME=10'
```

Here is the list of configurable tokens:

**Application Configuration:**
* `WATCHDOG_RELOAD_PERIOD_MS`
* `LORAWAN_APP_DATA_MAX_SIZE`
* `LORAWAN_REGION_USED`
* `PERIODICAL_UPLINK_DELAY_S`

**LoRaWAN Credentials:**
* `USE_LR11XX_CREDENTIALS`
* `LORAWAN_DEVICE_EUI`
* `LORAWAN_JOIN_EUI`
* `LORAWAN_NWK_KEY`
* `LORAWAN_APP_KEY`

**TCXO Configuration:**
* `TCXO_ON_BOARD` (set to 1 to enable TCXO support)
* `BOARD_TCXO_SUPPLY_VOLTAGE` (e.g., `MODEM_E_SYSTEM_TCXO_CTRL_3_3V`)
* `BOARD_TCXO_WAKEUP_TIME` (wakeup time in milliseconds)

Note: The supported application names are `lorawan`, `certification-otaa`, `certification-abp`, `class_b`, `multicast` and `fuota`.

2. The application binary file, for example `lorawan.bin`, is created in the `gcc/build` directory.
3. Copy the binary file to the STM32 microcontroller, either using the host OS copy facility or a dedicated tool like the STM32 ST-Link utility.

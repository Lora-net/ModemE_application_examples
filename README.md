# LR1121 modem Application Example

## 1. Description

The LR1121 modem application example project contains several simple examples highlighting LoRa Basic Modem-E features.

### 1.1. Simple LoRaWAN Class A application

This application automatically joins the LoRa Network server and then sends uplinks periodically with the interval defined by `PERIODICAL_UPLINK_DELAY_S`.

Please read the [lorawan application documentation](Src/apps/LoRaWAN/README.md) for more details.

### 1.2. Certification application

This application switches the modem to certification mode once the NUCLEO blue button is pressed.

Please read the [certification application documentation](Src/apps/certification/README.md) for more details.

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

## 3. Usage

Connect the NUCLEO board to a host PC via a USB cable. The USB connection will provide power for the LR1121 Evaluation Kit as well as serve as a serial communication link for the example application.

Use a terminal application configured with the following settings:
- Speed: 921600 baud
- Data bits: 8s
- Stop bits: 1
- Parity: None

Applications use the serial link to display information messages.  
Modem keys are defined in [lorawan_comissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h).  
To use the LR1121 modem production keys, update the USE_LR11XX_CREDENTIALS definition

## 4. Build & Install

To build the example application for the STM32L476RG controller of the NUCLEO development board, the ARM GCC tool chain must be set up under your development environment. These examples were developed using GNU Arm Embedded Toolchain 10-2020-q4-major 10.2.1 20201103 (release)

To build the example applications with the GNU Arm Embedded Toolchain:


1. Run `make` from the `gcc` directory with the target application name as an argument:

```
$ make APP=lorawan
```

Note: The supported application names are `lorawan`, `certification`, `class_b`, `multicast` and `fuota`.

2. The application binary file, for example `lorawan.bin`, is created in the `gcc/build` directory.
3. Copy the binary file to the STM32 microcontroller, either using the host OS copy facility or a dedicated tool like the STM32 ST-Link utility.

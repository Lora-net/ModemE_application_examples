# FUOTA example

This example of Firmware Upgrade Over the Air (FUOTA) demonstrates how to run pre-certification suite with LoRaWAN Certification Test Tool (LCTT) for FUOTA with LR1121 Modem-E v2.

## Scope

The FUOTA tested here consists only on the following aspects:

- Remote multicast setup
- Time synchronization
- Fragmentation transport

And it does not cover actual firmware management.

### References to LoRaWAN packages

The LCTT usage here allows to confirm the compatibility of LR1121 Modem-E v2 behavior against the following LoraWAN packages:

- `TS005-2.0.0 Remote Multicast Setup`
- `TS003-2.0.0 Application Layer Clock Synchronization`
- `TS004-2.0.0 Fragmented Data Block Transport`

## Requirements

Concerning LCTT, the following is required:

- LCTT v3.0.0
- Technology package v3.14.0_R1

Concerning the hardware:

- a Nucleo board STM32L476RG
- an LR1121 shield flashed with Modem-E v2
- a gateway
- a computer that can run LCTT

## Configure and build

### Embedded

Before building the embedded code, modify the commissioning information in file [lorawan_commissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h).

Then build the code with the following commands:

```bash
$ cd gcc/
$ make APP=fuota
# Building logs
```

This produces file *gcc/build/fuota.bin* that can be flashed into the nucleo board.

### Gateway

The gateway must be configured to communicate with the machine that runs LCTT tool.
Particularly the server address and port must refer to the machine running LCTT tool.

Refer to your gateway documentation for details.

### LCTT

The following configuration steps are valid for LCTT v3.0.0 with technology package v3.14.0_R1.

- The gateway configuration must be set to `FUOTA testing` (as on [this screenshot](Src/apps/fuota/doc/lctt_gw_config.PNG)).
- The device configuration must enables class A and class C
- The commissioning information must match between the LCTT configuration and the one from [lorawan_commissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h) file
- The LCTT I_ED_GenAppKey must be set to the LoRawWAN AppKey configured in file [lorawan_commissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h)
- Ensure the selected region match the one of the gateway and of the device under test
- Ensure the LoRaWAN version selected is 1.0.4
- Ensure the device configuration has Over-The-Air Activation (OTAA) enabled
- Ensure the device configuration has the following package enabled:
  - Applicative Layer Clock synchronization
  - Fragmentation transport
  - Remote multicast setup

## Usage

After configuring the embedded code, gateway and LCTT, prepare the test plan with the following tests (here for EU868):

- `TP_FUOTA_EU868_ED_MAC_104_BV_000`
- `TP_FUOTA_EU868_ED_MAC_104_BV_001`
- `TP_FUOTA_EU868_ED_MAC_104_BV_002`

Then press the `Run` button, and reset the device. This will force the device to join the LCTT LNS, and the tests will executes.

### Behavior

After reset, the device attempts to join a LoRaWAN network with its configured commissioning information.
It configures automatically the modem to run in certification mode.

After joining successfully a LoRaWAN network, the device sends empty uplinks at regularly.
The periodicity of uplinks is configured through macro `PERIODICAL_UPLINK_DELAY_S` (by default: every 50 seconds).

Note that the device is expected to join the LCTT LoRaWAN network.
The periodical uplinks offers downlinks opportunities to LCTT in order to run the FUOTA related tests.

## Issues and workarounds

This section provides some points to investigate in case of test failures.
It does not claim to be complete nor to substitute to LCTT documentation.
Refer to the LCTT documentation, bugzilla, or contact your LCTT representative for support.

For all situations, the logs from device, gateway and LCTT are an important source of information to diagnose the behavior of the device and of the test.

### Gateway issue

- Ensure the *HW checks* are passing (as on [this screenshot](Src/apps/fuota/doc/lctt_hw_check.PNG)).
- The machine running LCTT and the gateway must be capable to communicate on the network.
- The IP address and port the gateway is reporting to must be the one of the machine running LCTT (and not the one of another LNS)
- The LCTT configuration must refer the correct gateway IP, port, and Host IP

### RF issue

- Ensure only the DUT is under range of the gateway
- Ensure the RF power reported by the gateway for received packet is not too high nor too low
  - Typically the RSSI of reported packets should be in the range -90dBm to -100dBm

A possible way to leverage RF issues is to run the tests in conducted environment with enough attenuation to keep RF power received by the gateway in the indicated range.

### Failure on device activation step

- Ensure the device commissioning configured in LCTT matches the embedded software one
- Diagnose with LCTT logs for reasons of failure

### Failure on `TP_FUOTA_EU868_ED_MAC_104_BV_001` step `McClassCSessionReq`

If this test is failing on the check of received packet count during the multicast window, it is probably an issue with an incorrect GenAppKey.
Ensure the LCTT parameter `I_ED_GenAppKey` matches the LoRaWAN AppKey of the commissioning file [lorawan_commissioning.h](Inc/apps/lorawan_commissioning/lorawan_commissioning.h).

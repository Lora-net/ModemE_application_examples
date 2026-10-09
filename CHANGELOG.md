# Modem-E examples changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v1.3.1] - 2026-09-01

### Changed

- Upgrade Modem-E driver to v2.1.1

## [v1.3.0] - 2026-07-01

### Added

- Support for Modem-E driver v2.1.0

### Changed

- Updated all documentation to reflect LR1121/LR1110 dual support
- Multicast example: compile-time `#if` for class selection instead of runtime `if` (eliminates dead code)

### Fixed

- Coverity: unchecked return value in `smtc_hal_rng.c` (`HAL_RCCEx_PeriphCLKConfig`)
- Coverity: null pointer dereference in `apps_utilities.c` (`get_and_print_lorawan_region_from_modem`)
- Coverity: potential null dereference of linked list node in `smtc_hal_tmr_list.c`
- Coverity: out-of-bounds read on `MSIRangeTable` in `system_stm32l4xx.c`
- Coverity: uninitialized `EraseInitStruct.Banks` field in `smtc_hal_flash.c`
- Coverity: uninitialized `hal_status` variables in `smtc_hal_flash.c`
- Coverity: uninitialized `temperature` in `lis2de12.c`
- Coverity: unused `status` variable in `flash_write_buffer`
- Coverity: unused initial value for `pllr`/`pllm` in `system_stm32l4xx.c`
- Coverity: unused initial value for `duty_cycle` in all application examples
- Coverity: unused return value in `lis2de12_temperature_raw_get`
- Coverity: dead `flash_operation_retry = 0` assignment in flash erase functions
- Coverity: infinite loops replaced by `hal_mcu_panic()` in `smtc_hal_flash.c`
- Coverity: potential infinite loop in timer IRQ handler (`smtc_hal_tmr_list.c`)
- Coverity: printf format mismatch in `main_certification_abp.c`
- Coverity: mixed enum types in multiple application files (uplink type, certification mode, class, status bitmask)
- Coverity: operator precedence issue with enum cast in `modem_e_helper.c`
- Coverity: set-but-not-used timeout flag in `modem_e_hal_impl.c` (now used as reset timeout guard)
- Coverity: constant expression result in multicast class B checks

## [v1.2.0] - 2026-03-27

### Added

- Support for both LR1121 and LR1110 transceivers
- Support for Modem-E v2.1.0 and the Modem-E driver v2.0.0
- Split of the certification example into certification-otaa and certification-abp examples
- Certification ABP: NVM state save/restore on reset request for session continuity
- Certification ABP: channel mask configuration for US915/AU915 regions
- LED feedback: TX/RX LEDs blink 5 times on reset, TX LED pulses on transmission, RX LED pulses on downlink reception
- Handling of new LoRaWAN events: RELAY_TX_DYNAMIC, RELAY_TX_MODE, RELAY_TX_SYNC, ALC_SYNC_TIME, FUOTA_DONE, TEST_MODE
- ISM2400 region support
- Build script (`build_all.py`) for compiling all applications across all regions

### Changed

- Renamed all API calls, types, and constants from `lr1121_*` / `LR1121_*` prefix to `modem_e_*` / `MODEM_E_*`
- Renamed HAL implementation file to `modem_e_hal_impl.c`
- Renamed board files to `modem_e_board.c/h`
- Updated LoRaWAN region macros from `LR1121_LORAWAN_REGION_*` to `MODEM_E_LORAWAN_REGION_*`

## [v1.1.0] - 2025-10-15

### Added

- The toolchain supports building with compile-time parameters given on command-line

## [v1.0.0] - 2024-09-19

First release

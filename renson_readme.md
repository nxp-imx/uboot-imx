# Renson U-Boot Customization Overview

This document summarizes Renson-specific board enablement and customization in this `uboot-imx` tree.

## Board Support Added by Renson

- Added support for `imx91-9x9-flux`.
- Added support for `imx91-9x9-flux-v3`.
- Added/customized support for FRDM-based platforms used by Renson (for example `imx93_frdm` and related boot environment updates).

## Flux Family Layout

The `flux` and `flux-v3` boards currently reuse the same board implementation directory:

- `board/freescale/imx91_flux`

The `flux-v3` board has its own target and board description files:

- Defconfig: `configs/imx91_9x9_flux_v3_defconfig`
- Kconfig target: `CONFIG_TARGET_IMX91_9X9_FLUX_V3` in `arch/arm/mach-imx/imx9/Kconfig`
- DTS: `arch/arm/dts/imx91-9x9-flux-v3.dts`
- DTB registration: `arch/arm/dts/Makefile`

## FRDM Customizations

Renson has also customized FRDM-oriented flows, including boot environment improvements such as explicit FDT addresses in board env files.

Examples:

- `board/freescale/imx91_flux/imx91_flux.env`
- `board/freescale/imx93_frdm/imx93_frdm.env`

## DDR Timing Note for Flux V3

At this stage, `imx91-9x9-flux-v3` reuses existing Flux DDR timing objects.

If V3 requires different memory timing, update:

- Timing source files under `board/freescale/imx91_flux/` (for example create V3-specific LPDDR timing files).
- `board/freescale/imx91_flux/Makefile` to select V3 timing objects for:
  - `CONFIG_TARGET_IMX91_9X9_FLUX_V3`
- If needed, board init/SPL logic in `board/freescale/imx91_flux/spl.c` to reference the correct timing structures.

In short, switching memory timing from existing Flux settings to dedicated V3 settings is mainly controlled by the board Makefile object selection and the SPL timing references.

## Build Quick Start

Use GNU make (`gmake`) on macOS.

- Flux:
  - `gmake imx91_9x9_flux_defconfig`
  - `gmake -j8`

- Flux V3:
  - `gmake imx91_9x9_flux_v3_defconfig`
  - `gmake -j8`

## Environment Note

Some host setups require OpenSSL development headers for U-Boot host tools (`openssl/evp.h`).

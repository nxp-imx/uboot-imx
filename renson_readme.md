# Renson U-Boot Change Notes

This file tracks the main custom changes integrated in this `uboot-imx` tree for Renson up to now.

## Latest Board Addition

### `imx91-9x9-flux-v3` support

Integrated in commit:
- `c6bf53b03f0` - Add support for `imx91-9x9-flux-v3` device tree and related configurations

Implemented items:
- New defconfig: `configs/imx91_9x9_flux_v3_defconfig`
- New target symbol: `CONFIG_TARGET_IMX91_9X9_FLUX_V3`
- Shared board implementation reuse under `board/freescale/imx91_flux`
- New DTS: `arch/arm/dts/imx91-9x9-flux-v3.dts`
- DTB registration in `arch/arm/dts/Makefile`

Current decision:
- LPDDR timing is reused from existing flux board for now.
- V3-specific timing can be split later if hardware characterization requires it.

## Other Recent Renson-Related Changes

- `82623f10480` - Added `fdt_addr` and `fdt_addr_r` to `imx91_flux` and `imx93_frdm` environment files.
- `ad8c9d5c5b2` - Updated DTS Makefile mapping so `imx91-9x9-flux.dtb` is tied to `CONFIG_TARGET_IMX91_9X9_FLUX`.
- `6079880dc24` - Merged Renson patch set into this U-Boot branch.

## Merge/Integration Milestones

- `56f7e9fe874` - Merge pull request #3 from `lf_v2025.04`
- `99376713491` - Merge pull request #2 from `lf_v2025.04_rpre`
- `b4b9d1b7105` - Synced remote-tracking branch into `lf_v2025.04_rpre`
- `fc0745ed281` - Merge from upstream U-Boot/NXP line into Renson integration branch
- `1a30c555d74` - Merge pull request #1 for patch integration

## Quick Build Targets

Use GNU make (`gmake`) on macOS:

- Build existing flux:
  - `gmake imx91_9x9_flux_defconfig`
  - `gmake -j8`

- Build new flux-v3:
  - `gmake imx91_9x9_flux_v3_defconfig`
  - `gmake -j8`

## Notes

- Some host environments require OpenSSL development headers for U-Boot host tools (`openssl/evp.h`).
- If full build fails on host tool dependencies, defconfig generation can still be used to validate Kconfig wiring.

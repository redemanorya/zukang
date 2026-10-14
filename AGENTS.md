# Repository Guidelines

## Project Structure & Module Organization

`app_src/` contains product behavior, mode implementations, hardware-facing components, and the main configuration. Mode-specific work belongs under directories such as `app_src/app_mode_linein/`; reusable device logic belongs under `app_src/components/`. `app_framework/` provides the mode, audio, Bluetooth, and device engines. Low-level peripherals are in `driver/`, while protocol stacks, FreeRTOS, audio libraries, and boot support are in `middleware/`. Audio-effect and prompt resources live in `audioeffect_res/` and `remind_res/`. Design notes and change records are in `docs/`.

`Release/` is AndeSight-generated build output; do not hand-edit its `subdir.mk`, `sources.mk`, or object files. `SI4/` is Source Insight project metadata. Make production changes only in `B6_Audio_SDK`; treat sibling `B6_Audio_SDK_bak` as recovery material and `S037_SDK` as reference-only.

## Build, Test, and Development Commands

Build from an AndeSight/Cygwin shell with the Andes NDS32 toolchain configured:

```sh
make -C Release all
make -C Release clean
```

The first command creates `Release/B6_Audio_SDK.adx` and secondary images; the second removes generated build products. The preferred workflow is AndeSight **Clean Project**, then **Build Project**, followed by board programming through the existing debug configuration.

## Coding Style & Naming Conventions

Follow the surrounding C style: tabs for indentation, braces on their own line for functions, `snake_case` filenames, module-prefixed public functions, and uppercase configuration macros such as `CFG_LINEIN_INPUT_SOURCE`. Keep hardware constants close to the owning module and guard alternative implementations with compile-time macros.

Preserve existing C/H files as GB2312/CP936 with their original line endings. New Markdown files use UTF-8. Never perform an incidental whole-file encoding conversion; check for replacement characters after every edit.

## Testing Guidelines

No standalone unit-test framework is present. Every change must compile in all affected macro branches. For audio or peripheral work, validate on hardware: boot prompts, speaker/headphone output, UART framing, USB enumeration, and measured I2S clocks/data as applicable. Record expected and observed results in the PR or `docs/改动记录.md`.

## Commit & Pull Request Guidelines

This workspace snapshot has no Git history from which to infer an established convention. Use concise, imperative commits with a scope, for example `linein: correct GC1808 MCLK divider`. Keep unrelated changes separate. Pull requests should explain the hardware configuration, changed macros, build result, flashing target, bench-test evidence, and any compatibility or rollback considerations. Include logs or waveform screenshots when behavior depends on UART, USB, ADC, or I2S timing.

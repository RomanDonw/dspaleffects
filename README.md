# OpenAL-based DSP-filters for PipeWire

## Description
PipeWire real-time DSP-filters for [my own DSP-modules loader (loaddsp)](https://github.com/RomanDonw/loaddsp) with usage OpenAL Soft as sound postprocessing backend.

## Dependencies
- [loaddsp](https://github.com/RomanDonw/loaddsp).
- OpenAL Soft with supported extensions:
- - EFX (`AL_EXT_EFX`).
- - loopback device (`ALC_SOFT_loopback`).
- - output limiter (`ALC_SOFT_output_limiter`).
- - HRTF (`ALC_SOFT_HRTF`).
- - effect target (`AL_SOFT_effect_target`).
- json-c.

## Usage
Example command line to start EAX Reverb. module with config 3 (3.json):
```bash
loaddsp [...] ./dspeaxreverb.so -f presets/eaxreverb/3.json
```
Additional parameters are specified in the source code of modules. Also you can use all `loaddsp` parametres supported by them. See [page about loaddsp](https://github.com/RomanDonw/loaddsp) for more info.
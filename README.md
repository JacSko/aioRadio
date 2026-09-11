# aioRadio

## Project description
aioRadio (all-in-one Radio) is the ESP32-based system designed for FM, DAB and Online radio reception. It is based on Silicon Labs Si4684 chipset capable 
of decoding analog and digital radio. System consist of:
- Main custom-designed PCB with antenna input and 3,5mm jack audio output,
- st7796 480x320 LCD
- Rotary encoder with integrated push button

## Key features
- FM and DAB radio support
- Station list
- Band scan

## Building
System is based on esp-idf framework and works under freeRTOS control.

### Preconditions:
- download esp-idf toolchain
- source export.sh
### Build target
- go into project root directory and run:
```
./build_app.sh
```

### Build unit tests 
- go into project root directory and run:
```
./build_ut.sh
```

<img width="882" height="573" alt="image" src="https://github.com/user-attachments/assets/5d5096f8-eddb-4e4e-9163-700f9f375bca" />


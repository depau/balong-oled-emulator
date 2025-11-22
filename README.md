# Huawei E5785/E5885 display emulator

Emulator for the Huawei E5785/E5885 display, which can be used to test changed to
the [display hijacker](https://github.com/alexbers/huawei_oled_hijack_ng).

## Dependencies

The program must be run on a Linux system with the following packages installed. Other operating systems will most
likely not work.

- `sdl2`
- `sdl2-ttf`
- `fontconfig`

Build dependencies:

- `cmake`
- `pkg-config`

## How to use

[My fork](https://github.com/depau-forks/huawei_oled_hijack_ng) of the hijacker has a few changes that are useful to
build the library locally.

```bash
cd huawei_oled_hijack_ng
make -j$(nproc) CC=gcc oled_hijack_debug.so CFLAGS="-DHIJACK_DIR='\"$PWD\"'"
```

This builds the library for the current system, setting the `/app/hijack` directory to the hijack project root, so it
can find the scripts.

You may want to add the following to the `sms_and_ussd.sh` script after the shebang since it appears to break the
hijacker:

```bash
#!/bin/sh
echo "text:Stub"
exit 0
```

Then, build and run the emulator with the library attached:

```bash
# Build the emulator
cd balong-oled-emulator
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Emulate E5785 128x128 LCD display
LD_PRELOAD=../huawei_oled_hijack_ng/oled_hijack_debug.so ./balong-oled-emulator

# Emulate E5885 128x64 OLED display
LD_PRELOAD=../huawei_oled_hijack_ng/oled_hijack_debug.so ./balong-oled-emulator --short
```

## Controls

- Space: menu button
- Enter: power button

## Gotchas

Hold the space bar to enter the secret menu. It will be displayed after a few seconds since it attempts to perform a
bunch of stuff that doesn't make sense outside the actual modem, taking a few seconds to complete.

If the menu still doesn't appear, shortly press the space bar again to force the hijacker to redraw the screen.

# Project Template

This is the bare-metal STM32F411 project the rest of the repository is built on:
a minimal but complete skeleton that boots the chip, brings up the clocks and
drivers, links CMSIS-DSP in, and hands control to a single `main`. Copy it to
start a new experiment, or read it to see how the pieces below fit together.

## Layout

- [`app/`](app) - the applications: `main.c` and `board_test.c`, the `config`
  layer that initializes the drivers, describes the board wiring and retargets
  `printf` to UART2, and `signals.c` with the sample waveforms the examples
  feed their algorithms.
- [`drivers/`](drivers/README.md) - the bare-metal STM32F411 drivers the
  application runs on.
- [`linkers/`](linkers) - the linker script and startup code that place the image
  in flash and bring the C runtime up before `main`.
- [workspace files](workspaces) - editor and debugger configuration for the
  project.
- `Makefile` - builds the image from the project root, so `app/`, `drivers/`
  and `linkers/` all stay plain source folders.

The CMSIS-DSP archive this links against lives at the repository root, in
[`lib/`](../lib), and is shared by every module rather than copied into each one.

## Building

```sh
make          # compile and link the firmware image
make load     # flash the image to the board over J-Link
make monitor  # open the serial console with picocom
make clean    # remove build artifacts
```

`monitor` defaults to `/dev/ttyUSB0` at 115200 baud. Override it when the board
enumerates elsewhere:

```sh
make monitor PORT=/dev/ttyACM0
```

The build lands in `Build/`, with `flash.elf` for the debugger and `flash.bin`
for the flasher. `make` also prints the flash and SRAM footprint on every link,
which is worth watching once the DSP routines start pulling in lookup tables.

Building requires `libCMSISDSP.a`. If it is missing, build it once from the
repository root:

```sh
make -C ../lib
```

### Selecting which example to build

`main.c` is the default, but the Makefile takes an `APP` variable naming any
file in [`app/Src/`](app/Src), with one target per example:

```sh
make main         # the CMSIS-DSP starting point
make board_test   # exercises the board's peripherals
```

Modules that carry several examples add one target per concept, so each can be
built and flashed on its own.

### `board_test`

Worth running first on a fresh setup, and worth coming back to whenever a
result looks wrong, to tell a bad algorithm apart from bad wiring. It blinks
the three LEDs on `PB3`/`PB4`/`PB5` together every 500 ms and prints a line on
the same period with the potentiometer's raw ADC count and whether the button
was pressed since the last report:

```
pot:  2047   button: -
pot:  3120   button: pressed
```

The button is polled on every pass through the loop rather than once per
report, so a tap shorter than 500 ms still shows up.

## Debugging

The [`workspaces/`](workspaces) folder holds both editor and debugger setups:

- `app.code-workspace` opens `app/`, `drivers/`, `linkers/` and `lib/` as one
  VS Code workspace, with the CMSIS and driver include paths wired up so
  IntelliSense resolves `arm_math.h` and the register headers.
- `app.jdebug` is the Ozone project. It targets the STM32F411RE over SWD and
  opens `Build/flash.elf`, so a `make` followed by opening the project is enough
  to start a session.

Ozone is also how the signals get inspected: add a global to **View -> Data
Sampling** and show it under **View -> Timeline** to watch a buffer evolve while
the code runs.

For a quick look without the debugger, `printf` is retargeted to UART2 at 115200
baud, so the Arduino IDE's **Tools -> Serial Plotter** will graph whatever the
application prints.

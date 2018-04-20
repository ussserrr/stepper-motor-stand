# stepper-motor-stand
Arduino-based (ATmega328P MCU) stepper motor driver with remote control via USART (57600, 8N1). Motor is actuated via any switches. To see quick help, send `help`+ENTER command. It may be used as some sort of stand where alternate direction motion is needed.


## Features
  - 2 drive modes: wave and full-step (see [Wiki](https://en.wikipedia.org/wiki/Stepper_motor#Phase_current_waveforms)):
    - wave - basic mode (one phase per period)
    - full-step - increased torque (2 phases per period are driven simultaneously)
  - Pulse duration adjustment (change speed and torque)
  - 2 endstops for movement direction change
  - Save parameters to the EEPROM non-volatile memory
  - Built-in help for commands system
  - Rotate on the specified number of steps


## Connection
Driver uses Timer2 to manually switches corresponding phases of 4-wire bipolar stepper motors. Plug in your motor to any switches (relays, discrete transistors, array of transistors, Darlington transistors, driver IC and so on). Correct order of phases' connection depends on your motor but generally is A-C-B-D, considering AB and CD as 2 coils.
  - PC2, PC3, PC4, PC5: motor' phases
  - PB2, PB3: endstops (active-low, should be pulled up to Vcc)
  - USART serial connection: 57600 baud, 8N1, NL&CR.


## Build and flash
It's recommended to use [PlatformIO](https://platformio.org) for building and flashing as all-in-one solution.

### CLI
Change settings in `platformio.ini` file if needed (for example, specify whether use programming unit or not). Then in the same directory run:
```bash
$ pio run  # build

$ pio run -t program  # flash using usbasp
or
$ pio run -t upload  # flash using on-board programmer

$ pio run -t uploadeep  # write EEPROM
```

### IDE (Atom or VSCode)
  1. Import project: `File` -> `Add Project Folder`;
  2. Change settings in `platformio.ini` file if needed (for example, specify whether use programming unit or not);
  3. Build: `PlatformIO` -> `Build`;
  4. Open built-in terminal and run:
     ```bash
     $ pio run -t program; pio run -t uploadeep
     ```
     or for Arduino board:
     ```bash
     $ pio run -t upload; pio run -t uploadeep
     ```


## Commands system
Such help you can also get by sending `help` command to the driver itself.
```
pulse [value]
  Set the duration of the high-level pulse in microseconds (from 64us to 16384us, integer number). Values like '12345678' will be interpreted as 12345

fullstep
  Set full-step drive mode of stepper motor

wave
  Set wave drive mode of stepper motor

move/movenb [±steps]
  Make [±steps] steps in blocking/non-blocking mode and stop (regardless of the previous state). Sign defines the direction

stop
  Stop the motor in the current position

info
  Get an information about a content of timer's registers (compare value and prescaler), current mode (full-step or wave) and movement direction

save
  Save current configuration in EEPROM
```

///////////////////////////////////////////////////////////
//////////////////////// Version 1 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Microblaze with 128kB memory
- AXI Timer 0 on Int(0)
- Buttons 5-bit GPIO (input only) on Int(1)
- SW 16-bit GPIO (input only)
- LED 16-bit GPIO (output only)
- UART @ 19200
- 100MHz & 6.144MHz clock outputs
- 3-stage synchronizer for micData

SOFTWARE:

- Basic Xilkernel up and running
- LEDs working in leds_thread
- Pushbuttons & button_handler working
- Switches working in switches_thread
- No watchdog timer running

///////////////////////////////////////////////////////////
//////////////////////// Version 2 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Added PWM TIMER to generate timer_pwm signal
- AXI Timer 0 will just keep the kernel ticking
- Connected timer_pwm and micData to AUD_PWM --> use rotary switch to toggle

SOFTWARE:

- Changed switches_thread to rotary_thread
- Read rotary count values successfully
- Adjust PWM based on rotary count

///////////////////////////////////////////////////////////
//////////////////////// Version 3 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Added FIT Timer with clock count of 5000
- FIT Timer connected to Int(0)
- Added AXI SPI with data width 16-bits (modes: standard, master)
- AXI SPI connected to Int(4)
- Divided 6MHz clock to 3MHz for on-board mic
- Added SPI pins to constraints .XDC file

SOFTWARE:

- Got FIT interrupt handler working
- Initialized SPI device
- Test PWM on jack output with rotary-controlled sine wave
- Receiving SPI mic data

///////////////////////////////////////////////////////////
//////////////////////// Version 4 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Created ChorusBuffer IP from BlockRAM (16-bit wide, 65k deep)
- Added IP to embedded system with AXI (read/write)

SOFTWARE:

- Wrote ChorusBuffer drivers
- Tested drivers in application

///////////////////////////////////////////////////////////
//////////////////////// Version 5 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Created DelayBuffer IP from BlockRAM (16-bit wide, 65k deep)
- Added IP to embedded system with AXI (write-only)
- Removed PWM timer + interrupt
- Changed FIT timer to 16kKhz
- Created AudioOutput module to generate PDM stream
- Tested AudioOutput with 262kHz, 1MHz, 3MHz clock inputs

SOFTWARE:

- Wrote DelayBuffer drivers
- Tested drivers in application (grainy audio output using SPI mic input)
- Removed xilkernel and replaced with standalone OS
- Added FIT handler + added switch handler

///////////////////////////////////////////////////////////
//////////////////////// Version 6 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Created InputBuffer IP from BlockRAM (16-bit wide, 65k deep)
- Added IP to embedded system with AXI (read-only)
- Created AudioInput module to read & process PDM stream
- Fixed cross-clock domain issue in AudioInput

SOFTWARE:

- Wrote InputBuffer drivers
- Fixed bugs with InputBuffer base address
- Tested AudioInput --> InputBuffer --> DelayBuffer --> AudioOutput

///////////////////////////////////////////////////////////
//////////////////////// Version 6 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Removed SPI references from XDC and EMBSYS
- Removed 1MHz clock generator

SOFTWARE:

- Merged Chad + Neil's software with final_project.c
- Got DSP working successfully on PDM audio input

///////////////////////////////////////////////////////////
//////////////////////// Version 7 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Final working version of the hardware
- Tied AudioOutput to 4'b1101 read cycle
- Minor cleanup

SOFTWARE:

- Added commenting
- Fixed issue with float <--> int conversion
- Added FIT Handler back in to increment unused variable
- Added executables folder for BIT and ELF files
///////////////////////////////////////////////////////////
//////////////////////// Version 1 ////////////////////////
///////////////////////////////////////////////////////////

HARDWARE:

- Microblaze
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
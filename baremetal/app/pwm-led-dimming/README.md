# pwm-led-dimming

This is a basic application which demonstrates the use of PWM. This application can operate in one of two mode: `DEMO_MODE` and `TESTING_MODE`.

## DEMO_MODE
The `DEMO_MODE` is intended to be used for demonstrating how the PWM affects the dimming of a real LED.

When the application is in `DEMO_MODE`, the PWM will be generated on a single pre-determined pin, and the duty cycle will go through a set of predefined values. 

## TESTING_MODE (mostly for developers)
The `TESTING_MODE` is intended to be used to test the PWM on all supported pins, using a logic analyzer.

When the application is in `TESTING_MODE`, the PWM will be generated on every supported pin, going through a list of predefined duty cycle values for each pin.

## Changing the application mode
By default, the application runs in `DEMO_MODE`. Change the operation mode of the application via the preprocessor define in the `pwm_led_dimming.c` file.

```c
#define TESTING_MODE 0 // Keep at 0 for DEMO MODE, set to 1 for TESTING MODE
```
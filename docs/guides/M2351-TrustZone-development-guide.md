# TrustZone Development Guide #


This guide contains descriptions of how to achieve common tasks when working
on a TrustZone binary. Some familiarity with the CMake build system is
assumed, but it includes descriptions of what the build system is actually
instructed to do. Therefore, this guide should also be useful if you are not
using CMake.

## TrustZone Overview ##

### What is TrustZone? ###

The purpose of TrustZone is to separate secure (trusted) code and data from
non-secure (untrusted) code. This is accomplished by hardware within the Arm
core, which makes the separation very robust. The benefit is to ensure the
security of trusted code and data by forbidding untrusted code from reading
or modifying it.

TrustZone also allows the secure firmware developer to mark MCU peripherals
as trusted or untrusted. Thus, trusted hardware can only be accessed from
secure code (inaccessible to the non-secure world), while non-trusted
hardware can be accessed by both.

Overall, secure (trusted) code has access to the entire memory space & CPU
peripherals, while non-secure (untrusted) code only has access to peripherals
and regions marked as non-secure and non-secure callable by the secure
bootloader.

### What does TrustZone software look like? ##

The trust separation is reflected in the way TrustZone programs are compiled and
linked. Two different binaries are produced by the build system:

- A secure binary, compiled with the `-mcmse` flag of `arm-none-eabi-gcc`.
The secure binary usually contains code for booting the chip, as well as
secure APIs that are accessed by untrusted code.

- A non-secure binary, which contains application code. The non-secure binary
cannot talk to the secure world except through non-secure callable functions
defined in secure code.

These binaries must be flashed separately, at different addresses. This
preserves the separation between trusted and untrusted code, in that
untrusted software is entirely separate from the secure binary, and knows as
little as possible about it.

The start address of non-secure firmware must be known by:

- The secure bootloader, in order to successfully boot into the non-secure
application code

- The non-secure linker script, so that the absolute address of non-secure
symbols is known

- The tool that flashes the two binaries onto the microcontroller, in order
to position the non-secure code at the correct place in flash

### Communicating between worlds ###

By default, the secure binary has access to the entire system, while the
non-secure program only has access to the parts deemed non-secure by the
secure bootloader.

The secure binary can choose to provide an API to non-secure code through
entry or non-secure callable functions. This allows non-secure code to
access secure resources in a way that is safe, without breaking the security
guarantees set in place by the secure software developer.

Secure code can access non-secure code through callbacks. The secure code
defines a non-secure callable (NSC) function that takes a function pointer
argument. The secure world can then call the callback (a non-secure function)
in the body of the NSC function, which lets the secure binary access the
non-secure world.

Here is an example that demonstrates this concept:

The secure binary defines a NSC function called `call_the_callback`. Its
argument `callback` is a pointer to a function that takes an integer argument
and returns an integer. The secure code uses `callback` to do some work.

```
// in secure.h
#include <arm_cmse.h>

// Type for non-secure callback. The attribute instructs the compiler to safely
// call the non-secure function
typedef int (*nonsecure_callback_t)(int) __attribute__((cmse_nonsecure_call));

void call_the_callback(int (*callback)(int));
void do_secure_stuff_with(int);

// in secure.c
__attribute__((cmse_nonsecure_entry)) void call_the_callback(int (*callback)(int))
{
	int result = ((nonsecure_callback_t) cmse_nsfptr_create(callback)) (5);
    // Use the result for some secure work that the non-secure
    // binary must not know about
    do_secure_stuff_with(result);
}


int function_for_secure(int i)
{
    return (i+1);
}

int main()
{
	call_the_callback(function_for_secure);
    return 0;
}
```

The non-secure binary defines `function_for_secure`, the callback for the
secure binary, which is then passed to the secure code within `main`.

```
// in the non-secure main.c

int function_for_secure(int i)
{
    return (i+1);
}

void main()
{
	call_the_callback(function_for_secure);
}
```

### Implementation detail: Secure Gateway Veneers ###

Changing between the secure and non-secure worlds is potentially dangerous.
If it is not done carefully, untrusted software may gain access to the entire
system.

To mitigate this problem, TrustZone chips allow the developer to define
non-secure callable regions. These regions are the only place where
non-secure code can call the `SG` instruction (Secure Gateway) and switch to
the secure world.

The non-secure callable region must only contain secure gateway veneers: very
short functions consisting of the SG instruction and a jump to secure code.
The secure binary defines its non-secure API by creating these secure gateway
veneers.

Therefore, non-secure entry functions are composed of two parts:

- The main body of the function, which lies somewhere in the secure flash and
contains the actual code of the function.

- The secure gateway veneer, which lies in the non-secure callable region.

You don't generally have to worry about the distinction between SG veneers
and the actual code of the functions, since this is all handled by the
compiler & linker. However, you will likely come across the veneers when
using a debugger to step through non-secure callable functions.

### Cascoda and TrustZone ##

Cascoda defines a number of TrustZone targets. The most important one is
`freertos-default-secure`: this is a secure binary that contains FreeRTOS
secure code, as well as the Cascoda BSP. This secure binary is what powers
most of the TrustZone demos. You may use it as a reference for developing
your own secure binaries that use the CA821x and provide extra functionality.

The non-secure callable API is defined through the use of the
`__NONSECURE_ENTRY` macro, defined in `system_M2351.h`. This macro marks a
function as non-secure callable, which generates a SG veneer and places it in
the non-secure callable region.

The macro is used by invoking it just before the function definition:

```
__NONSECURE_ENTRY void BSP_PowerDown(
    u32_t sleeptime_ms,
    u8_t use_timer0,
    u8_t dpd,
    struct ca821x_dev *pDeviceRef
    )
{
	u8_t  lxt_connected = 0;
	u32_t timeout_sec;
    ...
    ...

```

You can find further examples in the Chili 2 version of
`cascoda_bsp_chili.c`.

## Configuring TrustZone in the Cascoda SDK ##

### Changing the address space partition (ROM and RAM) ###

The allocation of secure and non-secure memory and flash is decided in the
`partition_M2351.h` file. The Cascoda SDK contains several such files: one for
each secure binary, and a default one for non-TrustZone targets. You will
usually need to modify the partition file of the secure binary you are using,
which lives in the same folder as the source code of the secure binary. For
instance the partition file of the default FreeRTOS secure binary (the
`freertos-default-secure` target) can be found at
`baremetal\platform\cascoda-nuvoton-chili2\freertos-default-secure\include\partition_M2351.h`.

To change the allocation of ROM and RAM, you will need to modify
`SCU_SECURE_SRAM_SIZE` and `FMC_SECURE_ROM_SIZE` within the
`partition_M2351.h` belonging to the secure binary you are using.

You must also modify the linker scripts (for instance, `secure.ld` and
`nonsecure.ld` within `freertos-default-secure`) by changing the `FLASH`,
`RAM` and `__top_veneer_table` to be consistent with the settings in
`partition_M2351.h`. Failing to do so will usually result in hard faults (for
instance, if the CPU is running non-secure code when it reaches the point of
inconsistency) or reading all zeroes from the stack and possibly executing
null instructions (if the CPU is in the secure mode).

### Configuring peripherals as secure or non-secure ###

The security status of peripherals is stored as integers within the
`partition_M2351.h` file, where each bit of the integer represents the
security status of a peripheral. 0 indicates the peripheral is secure, and 1
represents non-secure peripherals.

For instance, changing bit 24 of `SCU_INIT_PNSSET0_VAL` to 0 would make the
`PDMA1` peripheral accessible only to code running in secure mode.

Several peripherals on the M2351 can only run in secure mode. They are `SYS`,
`CLK`, `NMI`, `PDMA0`, `RTC`, `FMC`, `SCU`, `WDT` and `TMR01`.

### Adding non-secure callable functions ###

The best place to put application-specific non-secure callable functions is
within the secure binary. The `freertos-secure-demo` target is an example of
how to do just that: the secure binary contains the functions defined in
`nsc_functions.c`. These functions are marked as non-secure callable thanks
to the FreeRTOS macro `secureportNON_SECURE_CALLABLE`, which maps to
`__NONSECURE_ENTRY`.

## Common pitfalls ##

### The security status bit of an address is implementation specific ###

The M2351 uses bit 28 of an address to indicate whether the address refers to
secure or non-secure code, data or peripheral. When bit 28 is low, the
address should point to something secure, and vice-versa. **This convention
is platform specific**, check the datasheet of your device to make sure this
is correct, because getting this wrong makes things very difficult. You
should find this information in the section describing the IDAU
(Implementation Defined Attribution Unit).

### Read All Zeroes, Write Ignore

In our experience, bugs within TrustZone code usually result in runtime
errors that are very difficult to investigate.

The most common cause of such issues is if the security status of a
peripheral is different from the security status of the address used to
control that peripheral.

Accessing a secure peripheral using a non-secure address generally results in
hardfaults. Accessing a non-secure peripheral using a secure address is even
more insidious: it can result in the access failing transparently - reading
from the address returns all zeroes and writing to the address is silently
ignored. This is referred to in the datasheet as RAZWI (Read All Zeroes,
Write Ignore).

![Extract from the M2351 datasheet, showing its TrustZone memory map. The
addresses of each memory region show that bit 28 differentiates secure areas
from non-secure ones.](memory-map.png "Memory map of the M2351")

For instance, at one point in development, the CPU started hardfaulting
within the secure binary while configuring some peripherals. The debugger
revealed that the hardfault occurs when the CPU attempted to write to an
address that looks like `0x0000 0128`, which lies within program memory and
cannot be directly written to. The actual peripheral that is supposed to be
accessed lives at address `0xefff 0128`, which is similar to what the write
attempted to do, but lacks a critical offset.

Looking at the assembly reveals that `0xefff 0000` was put by the compiler
onto the stack (the alternative would be to hardcode it as part of an
immediate-mode instruction) because it was used in several writes. However,
the secure stack was accidentally misconfigured to live within non-secure
memory. Therefore, in order to write to the peripheral, the CPU tried reading
from the stack using the secure alias of the stack pointer. Since the stack
was configured as non-secure, reading from it always returns 0 and writing is
ignored (RAZWI).

### GCC cannot compile secure functions with arguments passed on the stack

Unfortunately, `arm-none-eabi-gcc` does not yet feature support for passing
arguments on the stack, or even putting the return result on the stack. They
must all reside within registers. This means that code which uses too many
arguments, or arguments that are larger than 32 bits will fail to compile.
Similarly, code that returns values larger than 32 bits will also fail.

The workaround for this issue is replacing the arguments with a pointer to a
struct which holds the same data. Since the pointer is 32 bits, it will be
passed to the function using a register.

For returning large data, you need to add a pointer argument to which the return
data is copied.
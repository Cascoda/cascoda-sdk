MEMORY
{
    /* Define each memory region. */
    FLASH (rx)  : ORIGIN = 0x10010000, LENGTH = 0x70000 /* 448K bytes (alias Flash). */
    RAM (rwx)          : ORIGIN = 0x30004000, LENGTH = 0x14000 /* 80 KBytes (alias RAM). */
}

/* Entry point. */
ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector Table Section. */
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.);
        KEEP(*(.vectors))
        /* Reset Handler position is hard-coded so that the secure code can boot into it */
        KEEP(*(.text.Reset_Handler*))

        /* Global Section Table. */
        . = ALIGN(4);
        __section_table_start = .;

        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        __data_section_table_end = .;

        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        __bss_section_table_end = .;

        __section_table_end = .;
        /* End of Global Section Table. */

        *(.after_vectors*)
    } > FLASH

    /* Privileged functions - Section needs to be 32 byte aligned to satisfy MPU requirements. */
    .privileged_functions : ALIGN(32)
    {
        . = ALIGN(32);
        __privileged_functions_start__ = .;
        *(privileged_functions)
        . = ALIGN(32);
        /* End address must be the last address in the region, therefore, -1. */
        __privileged_functions_end__ = . - 1;
    } > FLASH

    /* FreeRTOS System calls - Section needs to be 32 byte aligned to satisfy MPU requirements. */
    .freertos_system_calls : ALIGN(32)
    {
        . = ALIGN(32);
        __syscalls_flash_start__ = .;
        *(freertos_system_calls)
        . = ALIGN(32);
        /* End address must be the last address in the region, therefore, -1. */
        __syscalls_flash_end__ = . - 1;
    } > FLASH

    /* Main Text Section - Section needs to be 32 byte aligned to satisfy MPU requirements. */
    .text : ALIGN(32)
    {
        . = ALIGN(32);
        __unprivileged_flash_start__ = .;

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
         *crtbegin.o(.dtors)
         *crtbegin?.o(.dtors)
         *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
         *(SORT(.dtors.*))
         *(.dtors)
        KEEP(*(.eh_frame*))

        *(.text*)
        *(.rodata .rodata.* .constdata .constdata.*)
        . = ALIGN(32);
        /* End address must be the last address in the region, therefore, -1. */
        __unprivileged_flash_end__ = . - 1;
    } > FLASH

    /* For exception handling/unwind - some Newlib functions (in common
     * with C++ and StdC++) use this. */
    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx : ALIGN(4)
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    /* Text Section End. */
    __etext = .;

    /* Uninit Reserved Section. */
    .uninit_RESERVED : ALIGN(4)
    {
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4);
        _end_uninit_RESERVED = .;
    } > RAM

    /* Main Data section (RAM). */
    .data : ALIGN(4)
    {
        FILL(0xff)
        __data_start__ = .;

        /* Privileged data - It needs to be 32 byte aligned to satisfy MPU requirements. */
        . = ALIGN(32);
        __privileged_sram_start__ = .;
        *(privileged_data);
        . = ALIGN(32);
        /* End address must be the last address in the region, therefore, -1. */
        __privileged_sram_end__ = . - 1;

        *(vtable)
        *(.ramfunc*)
        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);


        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        __data_end__ = .;
    } > RAM AT>FLASH

    /* Main BSS Section. */
    .bss : ALIGN(4)
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    /* Default Noinit Section. */
    .noinit (NOLOAD) : ALIGN(4)
    {
        _noinit = .;
        *(.noinit*)
        . = ALIGN(4);
        _end_noinit = .;
    } > RAM

    .heap (COPY):
    {
        __HeapBase = .;
        __end__ = .;
        end = __end__;
        KEEP(*(.heap*))
        __HeapLimit = .;
    } > RAM

    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (COPY):
    {
        KEEP(*(.stack*))
    } > RAM

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")

    /* Create checksum value (used in startup). */
    PROVIDE(__valid_user_code_checksum = 0 -
                                         (_vStackTop
                                         + (ResetISR + 1)
                                         + (NMI_Handler + 1)
                                         + (HardFault_Handler + 1)
                                         + (( DEFINED(MemManage_Handler) ? MemManage_Handler : 0 ) + 1)   /* MemManage_Handler may not be defined. */
                                         + (( DEFINED(BusFault_Handler) ? BusFault_Handler : 0 ) + 1)     /* BusFault_Handler may not be defined. */
                                         + (( DEFINED(UsageFault_Handler) ? UsageFault_Handler : 0 ) + 1) /* UsageFault_Handler may not be defined. */
                                         ) );

    /* Provide basic symbols giving location and size of main text block,
     * including initial values of RW data sections. Note that these will need
     * extending to give a complete picture with complex images
     * (e.g multiple Flash banks). */
    _image_start    = LOADADDR(.text);
    _image_end      = LOADADDR(.data) + SIZEOF(.data);
    _image_size     = _image_end - _image_start;
}

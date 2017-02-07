OUTPUT_FORMAT("binary")
SECTIONS
{
    . = 0xC0100000;
    .text : AT(0xC0100000)
    {
        *(.text);
    }
    .data : ALIGN(0x1000)
    {
        *(.data);
    }
    .rodata : ALIGN(0x1000)
    {
        *(.rodata);
    }
    .bss : ALIGN(0x1000)
    {
        bss_start = .;
        *(.bss);
        bss_end = .;
    }
}

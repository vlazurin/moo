OUTPUT_FORMAT("binary")
SECTIONS
{
    . = 0x7E00;
    .text : AT(0x7E00)
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

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
	ram : ORIGIN = 0x08000000, LENGTH = 400K
}

SECTIONS
{
/*
    . = 0x23F00000;

    .text.start : ALIGN(4) { *(.text.start) }
    .text       : ALIGN(4) { *(.text*) }
    .rodata     : ALIGN(4) { *(.rodata*) }
    .data       : ALIGN(4) { *(.data*) }
    .bss        : ALIGN(4) { __bss_start = .; *(.bss* COMMON); __bss_end = .;}
*/
	.text :
	{
		KEEP(*(.text.vect))
		KEEP(*(.text.start))
		*(.text*)
	} >ram

	.rodata : { *(.rodata*) } >ram

	.data : { *(.data*) } >ram

	.bss : { *(.bss*) } >ram

    . = ALIGN(4);

    __end__ = ABSOLUTE(.);
}

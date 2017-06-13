OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

payload_start = 0x23F00000;
payload_length = 0x00064000;

MEMORY
{
	ram : ORIGIN = payload_start, LENGTH = payload_length
}

SECTIONS
{
	.text.start : { *(.text.start) } >ram
	.text : { *(.text*) } >ram
	.rodata : { *(.rodata*) } >ram
	.data : { *(.data*) } >ram
	.bss : { *(.bss*) } >ram

	. = ALIGN(4);
	__end__ = ABSOLUTE(.);
}

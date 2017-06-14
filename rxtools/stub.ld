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
	.text : SUBALIGN(16) { *(.text*) } >ram
	.rodata : SUBALIGN(16) { *(.rodata*) } >ram
	.data : SUBALIGN(16) { *(.data*) } >ram
	.bss : SUBALIGN(16) { *(.bss*) } >ram

	. = ALIGN(4);
	__end__ = ABSOLUTE(.);
}

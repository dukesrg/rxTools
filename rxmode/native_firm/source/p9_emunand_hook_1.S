@ p9 emunand hook
.section .patch.p9.nand.write, "a"
.thumb
.align 2

	ldr r4, _nand_sd_write
	blx r4

.align 4
_nand_sd_write: 
#ifdef PLATFORM_KTR
	.word	0x08077C44
#else
	.word	0x0807882C	@ 0x08077C48
#endif

.pool

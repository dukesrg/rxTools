@ p9 emunand hook
.section .patch.p9.nand.read, "a"
.thumb
.align 2

	ldr r4, _nand_sd_read
	blx r4

.align 4
_nand_sd_read:
#ifdef PLATFORM_KTR
	.word	0x08077C84
#else
	.word	0x0807886C	@ 0x08077C88
#endif

.pool

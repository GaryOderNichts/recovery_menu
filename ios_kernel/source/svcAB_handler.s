.arm

.extern lolserial_print

.global svcAB_handler
svcAB_handler:
	ldmia sp, {r0, r1}
	cmp r0, #0x4
	mov r0, r1
	bleq lolserial_print
	ldmia sp!, {r0-r12, pc}^

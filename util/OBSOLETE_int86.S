.section .lowtext,"ax"
.code32
.align 4

saved_eax: .long 0
saved_ebp: .long 0
saved_es: .word 0
saved_ds: .word 0
saved_flags: .word 0
saved_esp: .long 0
saved_idtr:
.short 0
.long 0
.short 0
rmidt:
.short 0x3ff
.long 0

.global int86
int86:
	push %ebp
	mov %esp, %ebp
	pushal
	mov %esp, saved_esp
	mov %ebp, saved_ebp

	cli
	call pic_restore

	sidt (saved_idtr)
	lidt (rmidt)

	# modify the int instruction
	mov $int_op, %ebx
	movb 8(%ebp), %al
	movb %al, 1(%ebx)

	ljmp $0x28,$0f
0:
	.code16
	# disable protection
	mov %cr0, %eax
	and $0xfffe, %ax
	mov %eax, %cr0

	ljmp $0,$0f

0:
	xor %ax, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs

	# load registers from the int86regs struct
	# point esp to the regs struct to load registers with popa/popf
	mov saved_ebp, %ebp
	mov 12(%ebp), %esp
	popal
	popfw
	pop %es
	pop %ds

	xor %ax, %ax
	mov %ax, %ss
	mov $0x7be0, %sp

int_op:	int $0x16

	cli

	# save all registers that we'll clobber before having the
	# chance to populate the int86regs structure
	mov %eax, saved_eax
	mov %ds, saved_ds
	mov %es, saved_es
	pushfw
	popw %ax
	mov %ax, saved_flags

	# re-enable protection
	mov %cr0, %eax
	or $1, %ax
	mov %eax, %cr0

	ljmp $0x8,$0f
0:
	.code32

	# set data selector (2) to all segment regs
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs
	nop

	# restore our saved stack pointer
	mov saved_esp, %esp
	mov saved_ebp, %ebp

	mov 12(%ebp), %edi  # get pointer to int86regs struct

	# Fill in the struct at the correct offsets
	# Assuming struct is: eax, ecx, edx, ebx, esp, ebp, esi, edi, eflags, es, ds
	mov saved_eax, %eax
	mov %eax, 28(%edi)

	# Store the flags (with carry bit from saved_flags)
	pushfw
	popw %ax
	and $0xfffe, %ax
	mov saved_flags, %cx
	and $1, %cx
	or %cx, %ax
	mov %ax, 32(%edi)

	mov saved_es, %ax
	mov %ax, 34(%edi)
	mov saved_ds, %ax
	mov %ax, 36(%edi)

	lidt (saved_idtr)

	call pic_remap

	sti
	# the original implementer did this
	in $0x60, %al

	popal
	pop %ebp
	ret

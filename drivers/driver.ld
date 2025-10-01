OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)    /* not required, but harmless */

PHDRS {
  text PT_LOAD FLAGS(5);   /* PF_R + PF_X */
  data PT_LOAD FLAGS(6);   /* PF_R + PF_W */
}

/* Put sections tightly with minimal alignment */
SECTIONS
{
  /* place everything starting at file offset 0 in the output file */
  . = 0;

  .text : {
    *(.text .text.*)
    *(.gnu.hash)      /* optional, keep if you need dynamic lookups */
  } :text

  .rodata : {
    *(.rodata .rodata.*)
  } :text

  .data : {
    *(.data .data.*)
    *(.dynamic)       /* if you need .dynamic keep it here, else discard */
  } :data

  .bss : {
    *(.bss COMMON)
  } :data

  /* Keep dynamic symbol & strtab so loader can find exported names */
  .dynsym : { KEEP(*(.dynsym .dynsym.*)) }
  .dynstr : { KEEP(*(.dynstr .dynstr.*)) }

  /* Discard stamina sections that only bloat the file */
  /DISCARD/ : { *(.comment) *(.note.*) }

}

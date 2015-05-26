#ifndef _ElfDefs_
#define _ElfDefs_

////////////////////////////////////// Error strings //////////////////////////////////////////////
const char ERR_NOINIT[] = "CElfReader object not initialised or memory mapping failed";
const char ERR_NOELF[] = "Not a valid Linux elf file";
const char ERR_NOSHDRTAB[] = "No section header table";
const char ERR_NOSHDRSTRTAB[] = "No section header string table";
const char ERR_NOSYMTAB[] = "No symbol table";
const char ERR_NOSYMBOL[] = "Symbol not found";

#endif

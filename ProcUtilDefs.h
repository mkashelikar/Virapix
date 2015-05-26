#ifndef _ProcUtilDefs_
#define _ProcUtilDefs_

const char PROC_DIR[] = "/proc";
const char PROC_FD_DIR[] = "fd";
const char PROC_MAPS_FILE[] = "maps";
const char PROC_EXE_FILE[] = "exe";

const char LDSO_PATTERN[] = "/ld-";
const char LIBC_PATTERN[] = "/libc-";

const char ERR_MAPSFILE[] = "Error opening /proc maps file";
const char ERR_SHLIBNOMAP[] = "Shared library not mapped in address space";

#endif

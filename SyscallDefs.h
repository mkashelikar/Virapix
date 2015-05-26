#ifndef _SyscallDefs_
#define _SyscallDefs_

const char SHLIB_REGEX[] = "lib.*\\.so($|\\..*)";

enum EBaseDir
{
    E_OLD,
    E_NEWBAD,
    E_NEWOK,
    E_OLDHOME
};

enum EPath
{
    E_BAD,
    E_NOCREATE,
    E_OLDEXIST,
    E_PARENTEXIST,
    E_NEWEXIST,
    E_OLDHOMEDIR,
    E_OLDNODEL
};

enum EPathArg
{
    E_ORIGOK,
    E_ORIGBAD,
    E_BADLEN,
    E_MODIF
};

const int F_ENTRY = 0x1;
const int F_EXIT = 0x2;
const int F_CREATE = 0x4;
const int F_DELETE = 0x8;
const int F_NOREPLIC = 0x10;
const int F_SOCKSENDTO = 0x20;
const int F_SOCKSENDMSG = 0x40;
const int F_SOCKCALL32 = 0x80;
const int F_SYSCALLAT = 0x100;
const int F_OLDHOMEDIR = 0x200;
const int F_BADNEWDIR = 0x400;

const int MODE_64 = 0x33;
const int MODE_32 = 0x23;

const int NOTIFY_ALTSYSCALL = 0x1;

#endif


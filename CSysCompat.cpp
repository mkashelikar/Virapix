#include "CSysCompat.h"

#ifdef __x86_64__

#include <sys/syscall.h>

CSysCompat* CSysCompat::m_pSelf = 0;

// CSysCompat::CSysCompat* m_pSelf = 0;

// Syscalls not intercepted will be removed soon!
namespace sys32
{
    const int NR_restart_syscall = 0;
    const int NR_exit = 1;
    const int NR_fork = 2;
    const int NR_read = 3;
    const int NR_write = 4;
    const int NR_open = 5;
    const int NR_close = 6;
    const int NR_waitpid = 7;
    const int NR_creat = 8;
    const int NR_link = 9;
    const int NR_unlink = 10;
    const int NR_execve = 11;
    const int NR_chdir = 12;
    const int NR_time = 13;
    const int NR_mknod = 14;
    const int NR_chmod = 15;
    const int NR_lchown = 16;
    const int NR_break = 17;
    const int NR_oldstat = 18;
    const int NR_lseek = 19;
    const int NR_getpid = 20;
    const int NR_mount = 21;
    const int NR_umount = 22;
    const int NR_setuid = 23;
    const int NR_getuid = 24;
    const int NR_stime = 25;
    const int NR_ptrace = 26;
    const int NR_alarm = 27;
    const int NR_oldfstat = 28;
    const int NR_pause = 29;
    const int NR_utime = 30;
    const int NR_stty = 31;
    const int NR_gtty = 32;
    const int NR_access = 33;
    const int NR_nice = 34;
    const int NR_ftime = 35;
    const int NR_sync = 36;
    const int NR_kill = 37;
    const int NR_rename = 38;
    const int NR_mkdir = 39;
    const int NR_rmdir = 40;
    const int NR_dup = 41;
    const int NR_pipe = 42;
    const int NR_times = 43;
    const int NR_prof = 44;
    const int NR_brk = 45;
    const int NR_setgid = 46;
    const int NR_getgid = 47;
    const int NR_signal = 48;
    const int NR_geteuid = 49;
    const int NR_getegid = 50;
    const int NR_acct = 51;
    const int NR_umount2 = 52;
    const int NR_lock = 53;
    const int NR_ioctl = 54;
    const int NR_fcntl = 55;
    const int NR_mpx = 56;
    const int NR_setpgid = 57;
    const int NR_ulimit = 58;
    const int NR_oldolduname = 59;
    const int NR_umask = 60;
    const int NR_chroot = 61;
    const int NR_ustat = 62;
    const int NR_dup2 = 63;
    const int NR_getppid = 64;
    const int NR_getpgrp = 65;
    const int NR_setsid = 66;
    const int NR_sigaction = 67;
    const int NR_sgetmask = 68;
    const int NR_ssetmask = 69;
    const int NR_setreuid = 70;
    const int NR_setregid = 71;
    const int NR_sigsuspend = 72;
    const int NR_sigpending = 73;
    const int NR_sethostname = 74;
    const int NR_setrlimit = 75;
    const int NR_getrlimit = 76;
    const int NR_getrusage = 77;
    const int NR_gettimeofday = 78;
    const int NR_settimeofday = 79;
    const int NR_getgroups = 80;
    const int NR_setgroups = 81;
    const int NR_select = 82;
    const int NR_symlink = 83;
    const int NR_oldlstat = 84;
    const int NR_readlink = 85;
    const int NR_uselib = 86;
    const int NR_swapon = 87;
    const int NR_reboot = 88;
    const int NR_readdir = 89;
    const int NR_mmap = 90;
    const int NR_munmap = 91;
    const int NR_truncate = 92;
    const int NR_ftruncate = 93;
    const int NR_fchmod = 94;
    const int NR_fchown = 95;
    const int NR_getpriority = 96;
    const int NR_setpriority = 97;
    const int NR_profil = 98;
    const int NR_statfs = 99;
    const int NR_fstatfs = 100;
    const int NR_ioperm = 101;
    const int NR_socketcall = 102;
    const int NR_syslog = 103;
    const int NR_setitimer = 104;
    const int NR_getitimer = 105;
    const int NR_stat = 106;
    const int NR_lstat = 107;
    const int NR_fstat = 108;
    const int NR_olduname = 109;
    const int NR_iopl = 110;
    const int NR_vhangup = 111;
    const int NR_idle = 112;
    const int NR_vm86old = 113;
    const int NR_wait4 = 114;
    const int NR_swapoff = 115;
    const int NR_sysinfo = 116;
    const int NR_ipc = 117;
    const int NR_fsync = 118;
    const int NR_sigreturn = 119;
    const int NR_clone = 120;
    const int NR_setdomainname = 121;
    const int NR_uname = 122;
    const int NR_modify_ldt = 123;
    const int NR_adjtimex = 124;
    const int NR_mprotect = 125;
    const int NR_sigprocmask = 126;
    const int NR_create_module = 127;
    const int NR_init_module = 128;
    const int NR_delete_module = 129;
    const int NR_get_kernel_syms = 130;
    const int NR_quotactl = 131;
    const int NR_getpgid = 132;
    const int NR_fchdir = 133;
    const int NR_bdflush = 134;
    const int NR_sysfs = 135;
    const int NR_personality = 136;
    const int NR_afs_syscall = 137;
    const int NR_setfsuid = 138;
    const int NR_setfsgid = 139;
    const int NR__llseek = 140;
    const int NR_getdents = 141;
    const int NR__newselect = 142;
    const int NR_flock = 143;
    const int NR_msync = 144;
    const int NR_readv = 145;
    const int NR_writev = 146;
    const int NR_getsid = 147;
    const int NR_fdatasync = 148;
    const int NR__sysctl = 149;
    const int NR_mlock = 150;
    const int NR_munlock = 151;
    const int NR_mlockall = 152;
    const int NR_munlockall = 153;
    const int NR_sched_setparam = 154;
    const int NR_sched_getparam = 155;
    const int NR_sched_setscheduler = 156;
    const int NR_sched_getscheduler = 157;
    const int NR_sched_yield = 158;
    const int NR_sched_get_priority_max = 159;
    const int NR_sched_get_priority_min = 160;
    const int NR_sched_rr_get_interval = 161;
    const int NR_nanosleep = 162;
    const int NR_mremap = 163;
    const int NR_setresuid = 164;
    const int NR_getresuid = 165;
    const int NR_vm86 = 166;
    const int NR_query_module = 167;
    const int NR_poll = 168;
    const int NR_nfsservctl = 169;
    const int NR_setresgid = 170;
    const int NR_getresgid = 171;
    const int NR_prctl = 172;
    const int NR_rt_sigreturn = 173;
    const int NR_rt_sigaction = 174;
    const int NR_rt_sigprocmask = 175;
    const int NR_rt_sigpending = 176;
    const int NR_rt_sigtimedwait = 177;
    const int NR_rt_sigqueueinfo = 178;
    const int NR_rt_sigsuspend = 179;
    const int NR_pread64 = 180;
    const int NR_pwrite64 = 181;
    const int NR_chown = 182;
    const int NR_getcwd = 183;
    const int NR_capget = 184;
    const int NR_capset = 185;
    const int NR_sigaltstack = 186;
    const int NR_sendfile = 187;
    const int NR_getpmsg = 188;
    const int NR_putpmsg = 189;
    const int NR_vfork = 190;
    const int NR_ugetrlimit = 191;
    const int NR_mmap2 = 192;
    const int NR_truncate64 = 193;
    const int NR_ftruncate64 = 194;
    const int NR_stat64 = 195;
    const int NR_lstat64 = 196;
    const int NR_fstat64 = 197;
    const int NR_lchown32 = 198;
    const int NR_getuid32 = 199;
    const int NR_getgid32 = 200;
    const int NR_geteuid32 = 201;
    const int NR_getegid32 = 202;
    const int NR_setreuid32 = 203;
    const int NR_setregid32 = 204;
    const int NR_getgroups32 = 205;
    const int NR_setgroups32 = 206;
    const int NR_fchown32 = 207;
    const int NR_setresuid32 = 208;
    const int NR_getresuid32 = 209;
    const int NR_setresgid32 = 210;
    const int NR_getresgid32 = 211;
    const int NR_chown32 = 212;
    const int NR_setuid32 = 213;
    const int NR_setgid32 = 214;
    const int NR_setfsuid32 = 215;
    const int NR_setfsgid32 = 216;
    const int NR_pivot_root = 217;
    const int NR_mincore = 218;
    const int NR_madvise = 219;
    const int NR_getdents64 = 220;
    const int NR_fcntl64 = 221;
    const int NR_na222 = 222;
    const int NR_na223 = 223;
    const int NR_gettid = 224;
    const int NR_readahead = 225;
    const int NR_setxattr = 226;
    const int NR_lsetxattr = 227;
    const int NR_fsetxattr = 228;
    const int NR_getxattr = 229;
    const int NR_lgetxattr = 230;
    const int NR_fgetxattr = 231;
    const int NR_listxattr = 232;
    const int NR_llistxattr = 233;
    const int NR_flistxattr = 234;
    const int NR_removexattr = 235;
    const int NR_lremovexattr = 236;
    const int NR_fremovexattr = 237;
    const int NR_tkill = 238;
    const int NR_sendfile64 = 239;
    const int NR_futex = 240;
    const int NR_sched_setaffinity = 241;
    const int NR_sched_getaffinity = 242;
    const int NR_set_thread_area = 243;
    const int NR_get_thread_area = 244;
    const int NR_io_setup = 245;
    const int NR_io_destroy = 246;
    const int NR_io_getevents = 247;
    const int NR_io_submit = 248;
    const int NR_io_cancel = 249;
    const int NR_fadvise64 = 250;
    const int NR_na251 = 251;
    const int NR_exit_group = 252;
    const int NR_lookup_dcookie = 253;
    const int NR_epoll_create = 254;
    const int NR_epoll_ctl = 255;
    const int NR_epoll_wait = 256;
    const int NR_remap_file_pages = 257;
    const int NR_set_tid_address = 258;
    const int NR_timer_create = 259;
    const int NR_timer_settime = (NR_timer_create+1);
    const int NR_timer_gettime = (NR_timer_create+2);
    const int NR_timer_getoverrun = (NR_timer_create+3);
    const int NR_timer_delete = (NR_timer_create+4);
    const int NR_clock_settime = (NR_timer_create+5);
    const int NR_clock_gettime = (NR_timer_create+6);
    const int NR_clock_getres = (NR_timer_create+7);
    const int NR_clock_nanosleep = (NR_timer_create+8);
    const int NR_statfs64 = 268;
    const int NR_fstatfs64 = 269;
    const int NR_tgkill = 270;
    const int NR_utimes = 271;
    const int NR_fadvise64_64 = 272;
    const int NR_vserver = 273;
    const int NR_mbind = 274;
    const int NR_get_mempolicy = 275;
    const int NR_set_mempolicy = 276;
    const int NR_mq_open = 277;
    const int NR_mq_unlink = (NR_mq_open+1);
    const int NR_mq_timedsend = (NR_mq_open+2);
    const int NR_mq_timedreceive = (NR_mq_open+3);
    const int NR_mq_notify = (NR_mq_open+4);
    const int NR_mq_getsetattr = (NR_mq_open+5);
    const int NR_kexec_load = 283;
    const int NR_waitid = 284;
    const int NR_na285 = 285;
    const int NR_add_key = 286;
    const int NR_request_key = 287;
    const int NR_keyctl = 288;
    const int NR_ioprio_set = 289;
    const int NR_ioprio_get = 290;
    const int NR_inotify_init = 291;
    const int NR_inotify_add_watch = 292;
    const int NR_inotify_rm_watch = 293;
    const int NR_migrate_pages = 294;
    const int NR_openat = 295;
    const int NR_mkdirat = 296;
    const int NR_mknodat = 297;
    const int NR_fchownat = 298;
    const int NR_futimesat = 299;
    const int NR_fstatat64 = 300;
    const int NR_unlinkat = 301;
    const int NR_renameat = 302;
    const int NR_linkat = 303;
    const int NR_symlinkat = 304;
    const int NR_readlinkat = 305;
    const int NR_fchmodat = 306;
    const int NR_faccessat = 307;
    const int NR_pselect6 = 308;
    const int NR_ppoll = 309;
    const int NR_unshare = 310;
    const int NR_set_robust_list = 311;
    const int NR_get_robust_list = 312;
    const int NR_splice = 313;
    const int NR_sync_file_range = 314;
    const int NR_tee = 315;
    const int NR_vmsplice = 316;
    const int NR_move_pages = 317;
    const int NR_getcpu = 318;
    const int NR_epoll_pwait = 319;
    const int NR_utimensat = 320;
    const int NR_signalfd = 321;
    const int NR_timerfd_create = 322;
    const int NR_eventfd = 323;
    const int NR_fallocate = 324;
    const int NR_timerfd_settime = 325;
    const int NR_timerfd_gettime = 326;
    const int NR_signalfd4 = 327;
    const int NR_eventfd2 = 328;
    const int NR_epoll_create1 = 329;
    const int NR_dup3 = 330;
    const int NR_pipe2 = 331;
    const int NR_inotify_init1 = 332;
    const int NR_preadv = 333;
    const int NR_pwritev = 334;
    const int NR_rt_tgsigqueueinfo = 335;
    const int NR_perf_counter_open = 336;
}

void CSysCompat::_MapSyscall32To64()
{
    m_sys32To64[sys32::NR_open] = SYS_open;
    m_sys32To64[sys32::NR_creat] = SYS_creat;
    m_sys32To64[sys32::NR_link] = SYS_link;
    m_sys32To64[sys32::NR_unlink] = SYS_unlink;
    m_sys32To64[sys32::NR_execve] = SYS_execve;
    m_sys32To64[sys32::NR_chdir] = SYS_chdir;
    m_sys32To64[sys32::NR_mknod] = SYS_mknod;
    m_sys32To64[sys32::NR_chmod] = SYS_chmod;
    m_sys32To64[sys32::NR_lchown] = SYS_lchown;
    m_sys32To64[sys32::NR_oldstat] = SYS_stat;
    m_sys32To64[sys32::NR_utime] = SYS_utime;
    m_sys32To64[sys32::NR_oldfstat] = SYS_fstat;
    m_sys32To64[sys32::NR_access] = SYS_access;
    m_sys32To64[sys32::NR_rename] = SYS_rename;
    m_sys32To64[sys32::NR_mkdir] = SYS_mkdir;
    m_sys32To64[sys32::NR_rmdir] = SYS_rmdir;
    m_sys32To64[sys32::NR_acct] = SYS_acct;
    m_sys32To64[sys32::NR_symlink] = SYS_symlink;
    m_sys32To64[sys32::NR_oldlstat] = SYS_lstat;
    m_sys32To64[sys32::NR_readlink] = SYS_readlink;
    m_sys32To64[sys32::NR_truncate] = SYS_truncate;
    m_sys32To64[sys32::NR_fchmod] = SYS_fchmod;
    m_sys32To64[sys32::NR_fchown] = SYS_fchown;
    m_sys32To64[sys32::NR_socketcall] = sys32::SYS_SOCKETCALL; // That's our dummy syscall number!
    m_sys32To64[sys32::NR_stat] = SYS_stat;
    m_sys32To64[sys32::NR_lstat] = SYS_lstat;
    m_sys32To64[sys32::NR_fstat] = SYS_fstat;
    m_sys32To64[sys32::NR_clone] = SYS_clone;
    m_sys32To64[sys32::NR_fchdir] = SYS_fchdir;
    m_sys32To64[sys32::NR_chown] = SYS_chown;
    m_sys32To64[sys32::NR_truncate64] = SYS_truncate;
    m_sys32To64[sys32::NR_stat64] = SYS_stat;
    m_sys32To64[sys32::NR_lstat64] = SYS_lstat;
    m_sys32To64[sys32::NR_fstat64] = SYS_fstat;
    m_sys32To64[sys32::NR_lchown32] = SYS_lchown;
    m_sys32To64[sys32::NR_fchown32] = SYS_fchown;
    m_sys32To64[sys32::NR_chown32] = SYS_chown;
    m_sys32To64[sys32::NR_setxattr] = SYS_setxattr;
    m_sys32To64[sys32::NR_lsetxattr] = SYS_lsetxattr;
    m_sys32To64[sys32::NR_fsetxattr] = SYS_fsetxattr;
    m_sys32To64[sys32::NR_getxattr] = SYS_getxattr;
    m_sys32To64[sys32::NR_lgetxattr] = SYS_lgetxattr;
    m_sys32To64[sys32::NR_fgetxattr] = SYS_fgetxattr;
    m_sys32To64[sys32::NR_listxattr] = SYS_listxattr;
    m_sys32To64[sys32::NR_llistxattr] = SYS_llistxattr;
    m_sys32To64[sys32::NR_flistxattr] = SYS_flistxattr;
    m_sys32To64[sys32::NR_removexattr] = SYS_removexattr;
    m_sys32To64[sys32::NR_lremovexattr] = SYS_lremovexattr;
    m_sys32To64[sys32::NR_fremovexattr] = SYS_fremovexattr;
    m_sys32To64[sys32::NR_utimes] = SYS_utimes;
    m_sys32To64[sys32::NR_openat] = SYS_openat;
    m_sys32To64[sys32::NR_mkdirat] = SYS_mkdirat;
    m_sys32To64[sys32::NR_mknodat] = SYS_mknodat;
    m_sys32To64[sys32::NR_fchownat] = SYS_fchownat;
    m_sys32To64[sys32::NR_futimesat] = SYS_futimesat;
    m_sys32To64[sys32::NR_fstatat64] = SYS_newfstatat;
    m_sys32To64[sys32::NR_unlinkat] = SYS_unlinkat;
    m_sys32To64[sys32::NR_renameat] = SYS_renameat;
    m_sys32To64[sys32::NR_linkat] = SYS_linkat;
    m_sys32To64[sys32::NR_symlinkat] = SYS_symlinkat;
    m_sys32To64[sys32::NR_readlinkat] = SYS_readlinkat;
    m_sys32To64[sys32::NR_fchmodat] = SYS_fchmodat;
    m_sys32To64[sys32::NR_faccessat] = SYS_faccessat;
    m_sys32To64[sys32::NR_utimensat] = SYS_utimensat;
}

void CSysCompat::_MapSyscall64To32()
{
    m_sys64To32[SYS_stat] = sys32::NR_stat;
    m_sys64To32[SYS_chdir] = sys32::NR_chdir;
    m_sys64To32[SYS_chmod] = sys32::NR_chmod;
    m_sys64To32[SYS_chown] = sys32::NR_chown;
    m_sys64To32[SYS_setxattr] = sys32::NR_setxattr;
    m_sys64To32[SYS_getxattr] = sys32::NR_getxattr;
    m_sys64To32[SYS_listxattr] = sys32::NR_listxattr;
    m_sys64To32[SYS_removexattr] = sys32::NR_removexattr;
}

#endif

///////////////////////////////////////////////////// Public methods ///////////////////////////////////////////////////

CSysCompat* CSysCompat::GetInstance()
{
#ifdef __x86_64__
    return m_pSelf ? m_pSelf : (m_pSelf = new CSysCompat);
#else
    return 0;
#endif
}

void CSysCompat::Destroy()
{
#ifdef __x86_64__
    delete m_pSelf;
    m_pSelf = 0;
#endif
}

///////////////////////////////////////////////////// Private constructor ///////////////////////////////////////////////////

CSysCompat::CSysCompat()
{
#ifdef __x86_64__
    _MapSyscall32To64();
    _MapSyscall64To32();
#endif
}


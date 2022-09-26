/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/* src/iperf_config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the `clock_gettime' function. */
#undef HAVE_CLOCK_GETTIME

/* Define to 1 if you have the `cpuset_setaffinity' function. */
#undef HAVE_CPUSET_SETAFFINITY

/* Have CPU affinity support. */
#undef HAVE_CPU_AFFINITY

/* Define to 1 if you have the `daemon' function. */
#undef HAVE_DAEMON

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Have IP_MTU_DISCOVER/IP_DONTFRAG/IP_DONTFRAGMENT sockopt. */
#undef HAVE_DONT_FRAGMENT

/* Define to 1 if you have the <endian.h> header file. */
#define HAVE_ENDIAN_H   1

/* Have IPv6 flowlabel support. */
#undef HAVE_FLOWLABEL

/* Define to 1 if you have the `getline' function. */
#define HAVE_GETLINE    1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H     1

/* Have IP_DONTFRAG sockopt. */
#undef HAVE_IP_DONTFRAG

/* Have IP_DONTFRAGMENT sockopt. */
#undef HAVE_IP_DONTFRAGMENT

/* Have IP_MTU_DISCOVER sockopt. */
#undef HAVE_IP_MTU_DISCOVER

/* Define to 1 if you have the <linux/tcp.h> header file. */
#undef HAVE_LINUX_TCP_H

/* Define to 1 if you have the <netinet/sctp.h> header file. */
#undef HAVE_NETINET_SCTP_H

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H     1

/* Define to 1 if you have the `sched_setaffinity' function. */
#define HAVE_SCHED_SETAFFINITY      1

/* Have SCTP support. */
#undef HAVE_SCTP_H

/* Define to 1 if you have the `sendfile' function. */
#undef HAVE_SENDFILE

/* Define to 1 if you have the `SetProcessAffinityMask' function. */
#undef HAVE_SETPROCESSAFFINITYMASK

/* Have SO_BINDTODEVICE sockopt. */
#undef HAVE_SO_BINDTODEVICE

/* Have SO_MAX_PACING_RATE sockopt. */
#define HAVE_SO_MAX_PACING_RATE     1

/* OpenSSL Is Available */
#undef HAVE_SSL

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H       1

/* Define to 1 if you have the <stdio.h> header file. */
#undef HAVE_STDIO_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H       1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H      1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H       1

/* Define to 1 if the system has the type `struct sctp_assoc_value'. */
#undef HAVE_STRUCT_SCTP_ASSOC_VALUE

/* Define to 1 if you have the <sys/endian.h> header file. */
#define HAVE_SYS_ENDIAN_H       1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H       1

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H        1

/* Have TCP_CONGESTION sockopt. */
#define HAVE_TCP_CONGESTION     1

/* Have tcpi_snd_wnd field in tcp_info. */
#undef HAVE_TCP_INFO_SND_WND

/* Have TCP_USER_TIMEOUT sockopt. */
#undef HAVE_TCP_USER_TIMEOUT

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H       1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#undef LT_OBJDIR

/* Name of package */
#define PACKAGE "iperf"

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#define PACKAGE_NAME "iperf"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING  "iperf"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "iperf"

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.11"

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS    1

/* Version number of package */
#define VERSION     "3.11"  

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

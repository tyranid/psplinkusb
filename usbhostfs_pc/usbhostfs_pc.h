#ifndef __USBHOSTFS_PC_H__
#define __USBHOSTFS_PC_H__

extern int g_pid;
extern int g_verbose;

#define V_PRINTF(level, fmt, ...) { if(g_verbose >= level) { fprintf(stderr, fmt, ## __VA_ARGS__); } }

#endif

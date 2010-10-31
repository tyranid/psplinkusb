/*
 * PSPLINK
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPLINK root for details.
 *
 * tty.h - PSPLINK kernel module tty code
 *
 * Copyright (c) 2005 James F <tyranid@gmail.com>
 *
 * $HeadURL: http://psp.jim.sh/svn/psp/trunk/psplinkusb/psplink/tty.h $
 * $Id: tty.h 2304 2007-08-26 17:21:11Z tyranid $
 */

void ttySetUsbHandler(PspDebugPrintHandler usbStdoutHandler, PspDebugPrintHandler usbStderrHandler, PspDebugInputHandler usbStdinHandler);
void ttyInit(void);

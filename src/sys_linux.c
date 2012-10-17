/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>

#include "quakedef.h"

qboolean stdin_ready;
int do_stdin = 1;


cvar_t sys_nostdout = { "sys_nostdout", "0" };
cvar_t sys_extrasleep = { "sys_extrasleep", "0" };


void Sys_Printf(char *fmt, ...)
{
	va_list argptr;
	char text[2048];
	unsigned char *p;

	if (sys_nostdout.value)
		return;

	va_start(argptr, fmt);
	vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	for (p = (unsigned char *)text; *p; p++)
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
}

void Sys_Quit(void)
{
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
	exit(0);
}

void Sys_CvarInit(void)
{
}

void Sys_Init(void)
{
}

void Sys_Error(struct qw_connection *qwc, char *error, ...)
{
	va_list argptr;
	char string[1024];

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);	//change stdin to non blocking

	va_start(argptr, error);
	vsnprintf(string, sizeof(string), error, argptr);
	va_end(argptr);
	fprintf(stderr, "Error: %s\n", string);

	//Host_Shutdown();

	//exit(1);
    if (qwc)
        qwc->abort = 1;
}

//returns -1 if not present
int Sys_FileTime(char *path)
{
	struct stat buf;

	if (stat(path, &buf) == -1)
		return -1;

	return buf.st_mtime;
}

static unsigned int secbase;

#if LINUX_DOES_NOT_SUCK /* haha, good joke! */
double Sys_DoubleTime(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
	{
		secbase = ts.tv_sec;
		return ts.tv_nsec / 1000000000.0;
	}

	return (ts.tv_sec - secbase) + ts.tv_nsec / 1000000000.0;
}

unsigned long long Sys_IntTime()
{
	struct timespec ts;
	unsigned long long ret;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
		secbase = ts.tv_sec;

	ret = ts.tv_sec - secbase;
	ret *= 1000000;
	ret += ts.tv_nsec/1000;

	return ret;
}
#else
double Sys_DoubleTime(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec / 1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}

unsigned long long Sys_IntTime()
{
	struct timeval tp;
	struct timezone tzp;
	unsigned long long ret;

	gettimeofday(&tp, &tzp);

	if (!secbase)
		secbase = tp.tv_sec;

	ret = tp.tv_sec - secbase;
	ret *= 1000000;
	ret += tp.tv_usec;

	return ret;
}
#endif

char *Sys_ConsoleInput(void)
{
	static char text[256];
	int len;

	if (!stdin_ready || !do_stdin)
		return NULL;	// the select didn't say it was ready
	stdin_ready = false;

	len = read(0, text, sizeof(text));
	if (len == 0)
	{			// end of file          
		do_stdin = 0;
		return NULL;
	}
	if (len < 1)
		return NULL;
	text[len - 1] = 0;	// rip off the /n and terminate

	return text;
}

static void checkmousepoll()
{
	FILE *f;
	char buf[64];
	int mousepoll;

	f = fopen("/sys/module/usbhid/parameters/mousepoll", "r");
	if (f)
	{
		if (fgets(buf, sizeof(buf), f))
		{
			mousepoll = atoi(buf);
			if (mousepoll > 2)
			{
				Com_Printf("Warning: USB mouse poll rate is set to %dHz\n", 1000/mousepoll);
			}
		}

		fclose(f);
	}
}

/*
int main(int argc, char **argv)
{
	double time, oldtime, newtime;

	COM_InitArgv(argc, argv);
#if !defined(CLIENTONLY)
	dedicated = COM_CheckParm("-dedicated");
#endif

	if (!dedicated)
	{
		signal(SIGFPE, SIG_IGN);

		// we need to check for -noconinput and -nostdout before Host_Init is called
		if (!COM_CheckParm("-noconinput"))
			fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

		if (COM_CheckParm("-nostdout"))
			sys_nostdout.value = 1;

#if id386
		Sys_SetFPCW();
#endif
	}

	Host_Init(argc, argv);

	checkmousepoll();

	oldtime = Sys_DoubleTime();
	while (1)
	{
		if (dedicated)
			NET_Sleep(10);

		// find time spent rendering last frame
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;
		oldtime = newtime;

		Host_Frame(time);

		if (dedicated)
		{
			if (sys_extrasleep.value)
				usleep(sys_extrasleep.value);
		}
	}
}
*/

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length)
{
	int r;
	unsigned long addr;
	int psize = getpagesize();

	addr = (startaddr & ~(psize - 1)) - psize;
	r = mprotect((char *)addr, length + startaddr - addr + psize, 7);
	if (r < 0)
		Sys_Error(NULL, "Protection change failed");
}


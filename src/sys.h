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
// sys.h -- non-portable functions

#ifdef _WIN32
#define Sys_MSleep(x) Sleep(x)
#else
#define Sys_MSleep(x) usleep((x) * 1000)
#endif

// file IO
int	Sys_FileTime (char *path);
void Sys_mkdir (char *path);

// memory protection
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length);

// an error will cause the entire program to exit
struct qw_connection;
void Sys_Error (struct qw_connection *qwc, char *error, ...); // __attribute__((noreturn));

// send text to the console
void Sys_Printf (char *fmt, ...);

void Sys_Quit (void);

double Sys_DoubleTime (void);
unsigned long long Sys_IntTime(void);

void Sys_SleepTime(unsigned int usec);

char *Sys_ConsoleInput (void);

void Sys_LowFPPrecision (void);
void Sys_HighFPPrecision (void);
void Sys_SetFPCW (void);

void Sys_CvarInit(void);
void Sys_Init (void);

#include "sys_video.h"


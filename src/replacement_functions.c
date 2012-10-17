#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

void Com_Printf(const char *fmt, ...)
{
	va_list argptr;
	char msg[1024];
	
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	// also echo to debugging console
	printf ("%s", msg);

}

swig -perl qw_connection.i
gcc -fpic -c qw_connection_wrap.c \
   -Dbool=char \
   -I/usr/lib/perl5/5.12.2/i486-linux/CORE
gcc -shared qw_connection_wrap.o -lqw_connect -o qw_connection.so


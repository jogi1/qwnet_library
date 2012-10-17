#!/usr/bin/perl
use qw_connection;

$qwc = qw_connection::QWC_Create(0);

if (qw_connection::QWC_GetState($qwc) == qw_connection::qwc_state_idle)
{
    qw_connection::QWC_Connect($qwc, "qw.dybbuk.de:27600", "\\name\\buffster!\\", 36000);
}
sleep(3);
qw_connection::QWC_Say($qwc, "plaz sucks donkey balls!");

sleep(5);
qw_connection::QWC_Disconnect($qwc);
print "disconnecting!\n";

sleep(3);

qw_connection::QWC_Quit($qwc);

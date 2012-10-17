#include "quakedef.h"
#define       CM_MSEC (1 << 7)                // same as CM_ANGLE2


void MSG_BeginReading(struct qw_connection *qwc)
{
	qwc->msg_readcount = 0;
	qwc->msg_badread = false;
}

int MSG_GetReadCount(struct qw_connection *qwc)
{
	return qwc->msg_readcount;
}

qboolean MSG_ReadData(struct qw_connection *qwc, void *destination, unsigned int size)
{
	if (qwc->msg_readcount + size <= qwc->msg_message->cursize)
	{
		memcpy(destination, qwc->net_message.data+qwc->msg_readcount, size);
		qwc->msg_readcount+= size;
		return true;
	}

	return false;
}

int MSG_ReadLong (struct qw_connection *qwc)
{
	int	c;

	if (qwc->msg_readcount + 4 > qwc->msg_message->cursize)
	{
		qwc->msg_badread = true;
		return -1;
	}

	c = qwc->msg_message->data[qwc->msg_readcount]
		+ (qwc->msg_message->data[qwc->msg_readcount + 1] << 8)
		+ (qwc->msg_message->data[qwc->msg_readcount + 2] << 16)
		+ (qwc->msg_message->data[qwc->msg_readcount + 3] << 24);

	qwc->msg_readcount += 4;

	return c;
}

int MSG_ReadShort (struct qw_connection *qwc)
{
	int	c;

	if (qwc->msg_readcount + 2 > qwc->msg_message->cursize)
	{
		qwc->msg_badread = true;
		return -1;
	}

	c = (short)(qwc->msg_message->data[qwc->msg_readcount]
	+ (qwc->msg_message->data[qwc->msg_readcount + 1] << 8));

	qwc->msg_readcount += 2;

	return c;
}

float MSG_ReadCoord (struct qw_connection *qwc)
{
	return MSG_ReadShort(qwc) * (1.0 / 8);
}

int MSG_ReadByte (struct qw_connection *qwc)
{
	int	c;

	if (qwc->msg_readcount + 1 > qwc->msg_message->cursize)
	{
		qwc->msg_badread = true;
		return -1;
	}

	c = (unsigned char)qwc->msg_message->data[qwc->msg_readcount];
	qwc->msg_readcount++;

	return c;
}

// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar (struct qw_connection *qwc)
{
	int	c;

	if (qwc->msg_readcount + 1 > qwc->msg_message->cursize)
	{
		qwc->msg_badread = true;
		return -1;
	}

	c = (signed char)qwc->msg_message->data[qwc->msg_readcount];
	qwc->msg_readcount++;
	
	return c;
}


float MSG_ReadAngle (struct qw_connection *qwc)
{
	return MSG_ReadChar(qwc) * (360.0 / 256);
}

char *MSG_ReadString (struct qw_connection *qwc)
{
	char *string;
	int l,c;

    string = qwc->readstring_buffer;

	l = 0;
	do
	{
		c = MSG_ReadByte (qwc);
#if 0
		if (c == 255)			// skip these to avoid security problems
			continue;			// with old clients and servers
#endif
		if (c == -1 || c == 0)		// msg_badread or end of string
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(qwc->readstring_buffer)-1);

	string[l] = 0;

	return string;
}

float MSG_ReadFloat (struct qw_connection *qwc)
{
	union
	{
		byte b[4];
		float f;
		int	l;
	} dat;

	dat.b[0] = qwc->msg_message->data[qwc->msg_readcount];
	dat.b[1] = qwc->msg_message->data[qwc->msg_readcount + 1];
	dat.b[2] = qwc->msg_message->data[qwc->msg_readcount + 2];
	dat.b[3] = qwc->msg_message->data[qwc->msg_readcount + 3];
	qwc->msg_readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;	
}

float MSG_ReadAngle16 (struct qw_connection *qwc)
{
	return MSG_ReadShort(qwc) * (360.0 / 65536);
}

void MSG_ReadDeltaUsercmd (struct qw_connection *qwc)
{
	int bits;

	bits = MSG_ReadByte (qwc);

	if (qwc->protoversion == 26)
	{
		// read current angles
		if (bits & CM_ANGLE1)
			MSG_ReadAngle16 (qwc);
		MSG_ReadAngle16 (qwc);		// always sent
		if (bits & CM_ANGLE3)
			MSG_ReadAngle16 (qwc);

		// read movement
		if (bits & CM_FORWARD)
			MSG_ReadChar(qwc) << 3;
		if (bits & CM_SIDE)
			MSG_ReadChar(qwc) << 3;
		if (bits & CM_UP)
			MSG_ReadChar(qwc) << 3;
	}
	else
	{
		// read current angles
		if (bits & CM_ANGLE1)
			MSG_ReadAngle16 (qwc);
		if (bits & CM_ANGLE2)
			MSG_ReadAngle16 (qwc);
		if (bits & CM_ANGLE3)
			MSG_ReadAngle16 (qwc);

		// read movement
		if (bits & CM_FORWARD)
			MSG_ReadShort (qwc);
		if (bits & CM_SIDE)
			MSG_ReadShort (qwc);
		if (bits & CM_UP)
			MSG_ReadShort (qwc);
	}

	// read buttons
	if (bits & CM_BUTTONS)
		MSG_ReadByte (qwc);

	if (bits & CM_IMPULSE)
		MSG_ReadByte (qwc);

	// read time to run command
	if (qwc->protoversion == 26)
	{
		if (bits & CM_MSEC)
			MSG_ReadByte (qwc);
	}
	else
	{
		MSG_ReadByte (qwc);		// always sent
	}
}



/*
void MSG_ReadDeltaUsercmd (usercmd_t *from, usercmd_t *move, int protoversion)
{
	int bits;
    usercmd_t *from, usercmd_t *move;

	memcpy (move, from, sizeof(*move));

	bits = MSG_ReadByte ();

	if (protoversion == 26)
	{
		// read current angles
		if (bits & CM_ANGLE1)
			move->angles[0] = MSG_ReadAngle16 ();
		move->angles[1] = MSG_ReadAngle16 ();		// always sent
		if (bits & CM_ANGLE3)
			move->angles[2] = MSG_ReadAngle16 ();

		// read movement
		if (bits & CM_FORWARD)
			move->forwardmove = MSG_ReadChar() << 3;
		if (bits & CM_SIDE)
			move->sidemove = MSG_ReadChar() << 3;
		if (bits & CM_UP)
			move->upmove = MSG_ReadChar() << 3;
	}
	else
	{
		// read current angles
		if (bits & CM_ANGLE1)
			move->angles[0] = MSG_ReadAngle16 ();
		if (bits & CM_ANGLE2)
			move->angles[1] = MSG_ReadAngle16 ();
		if (bits & CM_ANGLE3)
			move->angles[2] = MSG_ReadAngle16 ();

		// read movement
		if (bits & CM_FORWARD)
			move->forwardmove = MSG_ReadShort ();
		if (bits & CM_SIDE)
			move->sidemove = MSG_ReadShort ();
		if (bits & CM_UP)
			move->upmove = MSG_ReadShort ();
	}

	// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadByte ();

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte ();

	// read time to run command
	if (protoversion == 26)
	{
		if (bits & CM_MSEC)
			move->msec = MSG_ReadByte ();
	}
	else
	{
		move->msec = MSG_ReadByte ();		// always sent
	}
}


*/

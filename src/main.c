#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "quakedef.h"
#include "sleep.h"
#include "qwc_functions.h"
#include "qwc_string.h"

enum state
{
	state_uninitialised,
	state_sendchallenge,
	state_sendconnection,
	state_connected
};


/*
void Command_Do(struct qw_connection *qwc, char *s)
{
    int i;
    char buf[256], *c;

    if (strstr(s, "test quit") != NULL)
        QWC_Quit(qwc);


    if (strstr(s, "test disconnect") != NULL)
        QWC_Disconnect(qwc);

    if ((c = strstr(s, "test cmd")) != NULL)
    {
        if (strstr(s, "test") == s)
            return;
        i = snprintf(buf, sizeof(buf), "%c%s", clc_stringcmd, c+9);
        printf("doing: \"%s\"\n", buf);
        if (i < sizeof(buf))
            NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
    }

    if ((c = strstr(s, "test say")) != NULL)
    {
        if (strstr(s, "test") == s)
            return;
        i = snprintf(buf, sizeof(buf), "%csay %s", clc_stringcmd, c+9);
        if (i < sizeof(buf))
            NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
    }
}

*/

void DoNetQWStuff(struct qw_connection *qwc)
{
	unsigned int startframe, numframes;

	NetQW_GenerateFrames(qwc->netqw);
	NetQW_CopyFrames(qwc->netqw, qwc->frames, &qwc->netchan.outgoing_sequence, &qwc->startframe, &qwc->numframes);

	while(qwc->numframes)
	{
		qwc->startframe++;
		qwc->startframe &= UPDATE_MASK;
		qwc->numframes--;
	}
}

void ParseDownload(struct qw_connection *qwc)
{
    static int lp = -1;
    int i, size, percent;
    char buf[128];

    size = MSG_ReadShort(qwc);
    percent = MSG_ReadByte(qwc);

    qwc->download_file_percentage = percent;
    if (size == -1)
    {
        if (qwc->download_file)
        {
            fclose(qwc->download_file);
            qwc->download_file = NULL;
        }
    }

    if (qwc->download_file)
        fwrite(qwc->net_message.data + qwc->msg_readcount, 1, size, qwc->download_file);

    qwc->msg_readcount += size;

    if (percent != 100)
    {
        if (percent % 10 == 0 && lp != percent)
        {
            lp = percent;
            i = snprintf(buf, sizeof(buf), "%csay %i%%", clc_stringcmd, percent);
            if (i < sizeof(buf))
                NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
        }
        i = snprintf(buf, sizeof(buf), "%cnextdl", clc_stringcmd);
		if (i < sizeof(buf))
			NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
    }
    else
    {
        i = snprintf(buf, sizeof(buf), "%csay download finished", clc_stringcmd);
        if (i < sizeof(buf))
            NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
        fclose(qwc->download_file);
        qwc->download_file == NULL;
        qwc->download_file_percentage = -1;
    }
}

void ParsePlayerinfo(struct qw_connection *qwc)
{
    int flags, i;

    MSG_ReadByte(qwc);
    flags = MSG_ReadShort(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadByte(qwc);

    if (flags &PF_MSEC)
    {
        MSG_ReadByte(qwc);
    }

    if (flags &PF_COMMAND)
    {
       MSG_ReadDeltaUsercmd(qwc);
    }

    for (i=0;i<3;i++)
        if (flags & (PF_VELOCITY1 << i))
            MSG_ReadShort(qwc);

    if (flags & PF_MODEL)
        MSG_ReadByte(qwc);

    if (flags & PF_SKINNUM)
        MSG_ReadByte(qwc);

    if (flags & PF_EFFECTS)
        MSG_ReadByte(qwc);

    if (flags & PF_WEAPONFRAME)
        MSG_ReadByte(qwc);
}

void ParseLightstyles(struct qw_connection *qwc)
{
    MSG_ReadByte(qwc);
    MSG_ReadString(qwc);
}

void ParseBaseLine(struct qw_connection *qwc)
{
    MSG_ReadShort(qwc);
    MSG_ReadByte(qwc);
    MSG_ReadByte(qwc);
    MSG_ReadByte(qwc);
    MSG_ReadByte(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadAngle(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadAngle(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadAngle(qwc);
}

void ParseSpawnStaticSound(struct qw_connection *qwc)
{

    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadByte(qwc);
    MSG_ReadByte(qwc);
    MSG_ReadByte(qwc);
}

void ParseSoundList(struct qw_connection *qwc)
{
   	char buf[128];
	int	numsounds, n;
	int i;
	char *str;

   	numsounds = MSG_ReadByte(qwc);

	while (1)
	{
		str = MSG_ReadString (qwc);
		if (!str[0])
			break;
        //printf("%i: %s\n", numsounds, str);
		numsounds++;
	}

	n = MSG_ReadByte(qwc);

	if (n)
	{
		i = snprintf(buf, sizeof(buf), "%csoundlist %i %i", clc_stringcmd, qwc->servercount, n);
		if (i < sizeof(buf))
			NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
		return;
    }

    // Request modellist
    i = snprintf(buf, sizeof(buf), "%cmodellist %i %i", clc_stringcmd, qwc->servercount, 0);
    if (i < sizeof(buf))
        NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);

}

void ParseModelList(struct qw_connection *qwc)
{
   	char buf[128];
	int	numsounds, n;
	int i;
	char *str;

   	numsounds = MSG_ReadByte(qwc);

	while (1)
	{
		str = MSG_ReadString (qwc);
		if (!str[0])
			break;
        //printf("%i: %s\n", numsounds, str);
		numsounds++;
	}

	n = MSG_ReadByte(qwc);

	if (n)
	{
		i = snprintf(buf, sizeof(buf), "%cmodellist %i %i", clc_stringcmd, qwc->servercount, n);
		if (i < sizeof(buf))
			NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
		return;
    }

    // done with modellist, request first of static signon messages
    i = snprintf(buf, sizeof(buf), "%cprespawn %i 0 %i", clc_stringcmd, qwc->servercount, 367136248);//LittleLong(cl.worldmodel->checksum2));
    if (i < sizeof(buf))
        NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
}

void ParseClientData(struct qw_connection *qwc)
{
    int newparsecount;
    float latency;
    struct frame_s *frame;

    qwc->oldparsecountmod = qwc->parsecountmod;

    newparsecount = qwc->netchan.incoming_acknowledged;

    qwc->cl_oldparsecount = newparsecount - 1;

    qwc->cl_parsecount = newparsecount;

    qwc->parsecountmod = (qwc->cl_parsecount & UPDATE_MASK);

    frame = &qwc->frames[qwc->parsecountmod];

    qwc->parsecounttime = qwc->frames[qwc->parsecountmod].senttime;

    frame->receivedtime = qwc->realtime;

    latency = frame->receivedtime - frame->senttime;
}

void ParseDelta(struct qw_connection *qwc, int bits)
{
    int i;

    bits &=  ~511;

    if (bits & U_MODEL)
    {
        i = MSG_ReadByte(qwc);
        bits |= i;
    }

    if (bits & U_MODEL)
        MSG_ReadByte(qwc);

    if (bits & U_FRAME)
        MSG_ReadByte(qwc);

    if (bits & U_COLORMAP)
        MSG_ReadByte(qwc);
    
    if (bits & U_SKIN)
        MSG_ReadByte(qwc);

    if (bits & U_EFFECTS)
        MSG_ReadByte(qwc);

    if (bits & U_ORIGIN1)
        MSG_ReadCoord(qwc);
    if (bits & U_ANGLE1)
        MSG_ReadAngle(qwc);
    if (bits & U_ORIGIN2)
        MSG_ReadCoord(qwc);
    if (bits & U_ANGLE2)
        MSG_ReadAngle(qwc);
    if (bits & U_ORIGIN3)
        MSG_ReadCoord(qwc);
    if (bits & U_ANGLE3)
        MSG_ReadAngle(qwc);


}

void FlushEntityPacket(struct qw_connection *qwc)
{
    int word; 
    while (1)
    {
        word = (unsigned short) MSG_ReadShort(qwc);

        if (!word)
            break;

        ParseDelta(qwc, word);
    }
}

void ParsePacketEntities(struct qw_connection *qwc, qboolean delta)
{
    int oldpacket, newpacket, oldindex, newindex, newnum, oldnum, word;
    packet_entities_t *oldp, *newp, dummy;
    unsigned int deltasequence;
    byte from;
    qboolean full;

    newpacket = qwc->netchan.incoming_sequence & UPDATE_MASK;
    newp = &qwc->frames[newpacket].packet_entities;
    qwc->frames[newpacket].invalid = false;

    if (delta)
    {
        from = MSG_ReadByte(qwc);
        oldpacket = qwc->frames[newpacket].delta_sequence;

        if (qwc->netchan.outgoing_sequence - qwc->netchan.incoming_sequence >= UPDATE_BACKUP - 1)
        {
            FlushEntityPacket(qwc);
            qwc->validsequence = 0;
            return;
        }

        if (from & UPDATE_MASK != oldpacket &UPDATE_MASK)
        {
            FlushEntityPacket(qwc);
            qwc->validsequence = 0;
            return;
        }

        if (qwc->netchan.outgoing_sequence - oldpacket >= UPDATE_BACKUP -1)
        {
            FlushEntityPacket(qwc);
            return;
        }
        oldp = &qwc->frames[oldpacket & UPDATE_MASK].packet_entities;
        full = false;
    }
    else
    {
        oldp = &dummy;
        dummy.num_entities = 0;
        full = true;
    }

    qwc->oldvalidsequence = qwc->validsequence;
	qwc->validsequence = qwc->netchan.incoming_sequence;
	deltasequence = qwc->validsequence;

	oldindex = 0;
	newindex = 0;
	newp->num_entities = 0;

    while (1)
    {
        word = (unsigned short) MSG_ReadShort(qwc);
        if (qwc->msg_badread)
        {
            printf("badread in packet entity read\n");
            qwc->abort = 1;
            return;
        }
        
        if (!word)
        {
            while (oldindex < oldp->num_entities)
            {
                oldindex++;
            }
            break;
        }
        newnum = word & 511;
        oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;

        while (newnum < oldnum)
        {
            if (full)
            {
                FlushEntityPacket(qwc);
                qwc->validsequence = 0;
                return;
            }
            newp->entities[newindex] = oldp->entities[oldindex];
			//CL_SetupPacketEntity(oldnum, &newp->entities[newindex], word > 511);
			newindex++;
			oldindex++;
			oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;

        }

        if (newnum < oldnum)
		{
			// new from baseline
			if (word & U_REMOVE)
			{
				if (full)
				{
					printf("WARNING: U_REMOVE on full update\n");
					FlushEntityPacket (qwc);
					qwc->validsequence = 0;	// can't render a frame
					return;
				}
				continue;
			}
			if (newindex >= MAX_MVD_PACKET_ENTITIES )
            {
                printf("CL_ParsePacketEntities: newindex == MAX_PACKET_ENTITIES");
                qwc->abort = 1;
                return;
            }
			ParseDelta (qwc, word);
			newindex++;
			continue;
		}

        if (newnum == oldnum)
		{
			// delta from previous
			if (full)
			{
				qwc->validsequence = 0;
				deltasequence = 0;
				printf("WARNING: delta on full update");
			}
			if (word & U_REMOVE)
			{
				oldindex++;
				continue;
			}
			ParseDelta (qwc, word);
			newindex++;
			oldindex++;
		}
	}
    NetQW_SetDeltaPoint(qwc->netqw, deltasequence?deltasequence:-1);
}

void ParseBeam(struct qw_connection *qwc, int type)
{
    MSG_ReadShort(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
}

void ParseTempEntity(struct qw_connection *qwc)
{
    int type;

    type = MSG_ReadByte(qwc);

    switch (type)
    {
        case TE_WIZSPIKE:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_KNIGHTSPIKE:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_SPIKE:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_SUPERSPIKE:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_EXPLOSION:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_TAREXPLOSION:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_LIGHTNING1:
            ParseBeam(qwc, 0);
            break;

        case TE_LIGHTNING2:
            ParseBeam(qwc, 1);
            break;

        case TE_LIGHTNING3:
            ParseBeam(qwc, 2);
            break;

        case TE_LAVASPLASH:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_TELEPORT:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_GUNSHOT:
            MSG_ReadByte(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_BLOOD:
            MSG_ReadByte(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        case TE_LIGHTNINGBLOOD:
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            MSG_ReadCoord(qwc);
            break;

        default:
            qwc->abort = 1;
            return;
        }
}

void Parse_svc_serverdata(struct qw_connection *qwc)
{
    int protover, i;
    char buf[128];
    char *str;

    while (1)
    {
        qwc->protoversion = MSG_ReadLong(qwc);
        if (qwc->protoversion == QW_PROTOEXT_FTEX)
        {
            MSG_ReadLong(qwc);
            printf("FTE PROTOCOL!\n");
        }
        else
        {
            break;
        }
    }

    qwc->servercount = MSG_ReadLong(qwc); 

    str = MSG_ReadString(qwc);
    printf("gamedir: %s\n", str);

    // playernum
    MSG_ReadByte(qwc);

    str = MSG_ReadString(qwc);
    printf("\"%c\"\n", qwc->msg_message->data[qwc->msg_readcount]);
    printf("levelname: %s\n", str);

    if (qwc->msg_badread)
        printf("badread!\n");

    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);
    MSG_ReadFloat(qwc);

    i = snprintf(buf, sizeof(buf), "%csoundlist %i %i", clc_stringcmd, qwc->servercount, 0);
    if (i < sizeof(buf))
        NetQW_AppendReliableBuffer(qwc->netqw, buf, i+1);

    
}

void ParsePrint(struct qw_connection *qwc, char *s)
{
    int i;

    i = strlen(s);

    qwc->buffer_position += snprintf(qwc->buffer + qwc->buffer_position, sizeof(qwc->buffer) - qwc->buffer_position, "%s", s);

    if (s[i-1] == '\n')
    {
        QWC_StringAdd(qwc, qwc->buffer);

        qwc->buffer[0] = '\0';
        qwc->buffer_position = 0;
        
    }
    else
    {
    }
}

void ParseSoundPacket(struct qw_connection *qwc)
{
    int channel, volume;
    float attenuation;

    channel = MSG_ReadShort(qwc);
    volume = (channel & SND_VOLUME) ? MSG_ReadByte(qwc) : DEFAULT_SOUND_PACKET_VOLUME;
    attenuation = (channel & SND_ATTENUATION) ? MSG_ReadByte(qwc) / 64.0 : DEFAULT_SOUND_PACKET_ATTENUATION;
    MSG_ReadByte(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
    MSG_ReadCoord(qwc);
}

void ParseServerMessage(struct qw_connection *qwc)
{
    int cmd, i, j;
    int msg_svc_start;
    char *s;
    char buf[128];

 //   QWC_DPInit(qwc);
    qwc->packet_count++;

    ParseClientData(qwc);

    while (1)
    {
        if (qwc->msg_badread)
        {
            QWC_DPPrintf(qwc, "message bad read!\n");
            qwc->abort = 1;
            return;
        }

        msg_svc_start = qwc->msg_readcount;

        cmd = MSG_ReadByte(qwc);

        if (cmd == -1)
        {
            qwc->msg_readcount++;
            QWC_DPPrintf(qwc, "end of network message\n");
            break;
        }

        QWC_DPPrintf(qwc, "%i: ", (unsigned int)cmd);

        switch (cmd)
        {
            default:
                QWC_DPPrintf(qwc, "not a viable message!\n");
                qwc->abort = 1;
                return;

            case svc_nop:
                QWC_DPPrintf(qwc, "svc_nop\n");
                break;

            case svc_disconnect:
                QWC_DPPrintf(qwc, "scv_disconnect\n");
                break;

            case svc_time:
                QWC_DPPrintf(qwc, "svc_time\n");
                break;

            case svc_print:
                QWC_DPPrintf(qwc, "svc_print\n");
                QWC_DPPrintf(qwc, "%i\n", MSG_ReadByte(qwc));
                s = MSG_ReadString(qwc);
                ParsePrint(qwc, s);
                break;

            case svc_centerprint:
                QWC_DPPrintf(qwc, "svc_centerprint\n");
                MSG_ReadString(qwc);
                break;

            case svc_stufftext:
                QWC_DPPrintf(qwc, "svc_stufftext\n");
                s = MSG_ReadString(qwc);
                if (strncmp("cmd", s, 3) == 0)
                {
                    i = snprintf(buf, sizeof(buf), "%c%s", clc_stringcmd, s+3);
                    if (i < sizeof(buf))
                        NetQW_AppendReliableBuffer(qwc->netqw, buf, i+1);
                }

                if (strncmp("skins", s, 5) == 0)
                {
                    i = snprintf(buf, sizeof(buf), "%cbegin %i", clc_stringcmd, qwc->servercount);
                    if (i < sizeof(buf))
                        NetQW_AppendReliableBuffer(qwc->netqw, buf, i+1);
                }

                if (strncmp("download", s, 8) == 0)
                {
                    if (qwc->download_file)
                        fclose(qwc->download_file);
                    qwc->download_file = fopen(s+9, "w");

                    i = snprintf(buf, sizeof(buf), "%cdownload %s", clc_stringcmd, s+9);
                    if (i < sizeof(buf))
                        NetQW_AppendReliableBuffer(qwc->netqw, buf, i+1);
                }
 
                QWC_DPPrintf(qwc, "%s\n", s);
                break;

            case svc_damage:
                MSG_ReadByte(qwc);
                MSG_ReadByte(qwc);
                MSG_ReadCoord(qwc);
                MSG_ReadCoord(qwc);
                MSG_ReadCoord(qwc);
                QWC_DPPrintf(qwc, "svc_damage\n");
                break;

            case svc_serverdata:
                QWC_DPPrintf(qwc, "svc_serverdata\n");
                Parse_svc_serverdata(qwc);
                break;
                
            case svc_setangle:
                QWC_DPPrintf(qwc, "svc_setangle\n");
                MSG_ReadAngle(qwc);
                MSG_ReadAngle(qwc);
                MSG_ReadAngle(qwc);
                break;

            case svc_lightstyle:
                ParseLightstyles(qwc);
                QWC_DPPrintf(qwc, "svc_lightstyle\n");
                break;

            case svc_sound:
                QWC_DPPrintf(qwc, "svc_sound\n");
                ParseSoundPacket(qwc);
                break;
                
            case svc_stopsound:
                QWC_DPPrintf(qwc, "svc_stopsound\n");
                break;
                
            case svc_updatefrags:
                QWC_DPPrintf(qwc, "svc_updatefrags\n");
                MSG_ReadByte(qwc);
                MSG_ReadShort(qwc);
                break;

            case svc_updateping:
                QWC_DPPrintf(qwc, "svc_updateping\n");
                MSG_ReadByte(qwc);
                MSG_ReadShort(qwc);
                break;

            case svc_updatepl:
                QWC_DPPrintf(qwc, "svc_updatepl\n");
                MSG_ReadByte(qwc);
                MSG_ReadByte(qwc);
                break;

            case svc_updateentertime:
                QWC_DPPrintf(qwc, "svc_updateentertime\n");
                MSG_ReadByte(qwc);
                MSG_ReadFloat(qwc);
                break;

            case svc_updateuserinfo:
                QWC_DPPrintf(qwc, "svc_updateuserinfo\n");
                MSG_ReadByte(qwc);
                MSG_ReadLong(qwc);
                MSG_ReadString(qwc);
                break;

            case svc_spawnbaseline:
                ParseBaseLine(qwc);
                QWC_DPPrintf(qwc, "svc_spawnbaseline\n");
                break;

            case svc_spawnstatic:
                QWC_DPPrintf(qwc, "svc_spawnstatic\n");
                break;

            case svc_temp_entity:
                QWC_DPPrintf(qwc, "svc_temp_entity\n");
                ParseTempEntity(qwc);
                break;

            case svc_killedmonster:
                QWC_DPPrintf(qwc, "svc_killedmonster\n");
                break;

            case svc_foundsecret:
                QWC_DPPrintf(qwc, "svc_foundsecret\n");
                break;

            case svc_updatestat:
                QWC_DPPrintf(qwc, "svc_updatestat\n");
                MSG_ReadByte(qwc);
                MSG_ReadByte(qwc);
                break;

            case svc_updatestatlong:
                QWC_DPPrintf(qwc, "svc_updatestatlong\n");
                MSG_ReadByte(qwc);
                MSG_ReadLong(qwc);
                break;
                
            case svc_spawnstaticsound:
                QWC_DPPrintf(qwc, "svc_spawnstaticsound\n");
                ParseSpawnStaticSound(qwc);
                break;

            case svc_cdtrack:
                QWC_DPPrintf(qwc, "svc_cdtrack\n");
                MSG_ReadByte(qwc);
                break;

            case svc_intermission:
                QWC_DPPrintf(qwc, "svc_intermission\n");
                break;

            case svc_finale:
                QWC_DPPrintf(qwc, "svc_finale\n");
                break;
                
            case svc_sellscreen:
                QWC_DPPrintf(qwc, "svc_sellscreen\n");
                break;

            case svc_smallkick:
                QWC_DPPrintf(qwc, "svc_smallkick\n");
                break;

            case svc_bigkick:
                QWC_DPPrintf(qwc, "svc_bigkick\n");
                break;

            case svc_muzzleflash:
                QWC_DPPrintf(qwc, "svc_muzzleflash\n");
                MSG_ReadShort(qwc);
               break;

            case svc_setinfo:
                QWC_DPPrintf(qwc, "svc_setinfo\n");
                MSG_ReadByte(qwc);
                MSG_ReadString(qwc);
                MSG_ReadString(qwc);
                break;

            case svc_serverinfo:
                QWC_DPPrintf(qwc, "svc_serverinfo\n");
                break;
                
            case svc_download:
                QWC_DPPrintf(qwc, "svc_download\n");
                ParseDownload(qwc);
                break;
                
            case svc_playerinfo:
                QWC_DPPrintf(qwc, "svc_playerinfo\n");
                ParsePlayerinfo(qwc);
                break;

            case svc_nails:
                QWC_DPPrintf(qwc, "svc_nails\n");
                break;

            case svc_nails2:
                QWC_DPPrintf(qwc, "svc_nails2\n");
                break;

            case svc_chokecount:
                QWC_DPPrintf(qwc, "svc_chokecount\n");
                MSG_ReadByte(qwc);
                break;

            case svc_modellist:
                QWC_DPPrintf(qwc, "svc_modellist\n");
                ParseModelList(qwc);
                break;
             
            case svc_soundlist:
                QWC_DPPrintf(qwc, "svc_soundlist\n");
                ParseSoundList(qwc);
                break;
             
            case svc_packetentities:
                QWC_DPPrintf(qwc, "svc_packetentities\n");
                ParsePacketEntities(qwc, false);
                break;
             
            case svc_deltapacketentities:
                QWC_DPPrintf(qwc, "svc_deltapacketentities\n");
                break;
             
            case svc_maxspeed:
                QWC_DPPrintf(qwc, "svc_maxspeed\n");
                break;
             
            case svc_entgravity:
                QWC_DPPrintf(qwc, "svc_entgravity\n");
                break;
             
            case svc_qizmovoice:
                QWC_DPPrintf(qwc, "svc_qizmovoice\n");
                break;
        }
    }
}

qboolean Get_Message(struct qw_connection *qwc)
{
    unsigned int size;
    void *p;

    if (qwc->netqw)
    {
        size = NetQW_GetPacketLength(qwc->netqw); 
        if (size)
        {
            if (size > sizeof(qwc->net_message_buffer))
                size = sizeof(qwc->net_message_buffer);

            p = (void *) NetQW_GetPacketData(qwc->netqw);

            memcpy(qwc->net_message_buffer, p, size);

            NetQW_FreePacket(qwc->netqw);

            qwc->net_message.cursize = qwc->net_message_size = size;
            qwc->net_message.data = qwc->net_message_buffer;
            qwc->msg_message = &qwc->net_message;

            DoNetQWStuff(qwc);

            return true;
        }
    }

    return false;
}

void ConnectionlessPacket(struct qw_connection *qwc)
{
    int c;

    MSG_BeginReading(qwc);
    MSG_ReadLong(qwc);
    c = MSG_ReadByte(qwc);

    if (qwc->msg_badread)
        return;

    switch (c)
    {
        case A2C_CLIENT_COMMAND:
            printf("test\n");
            break;

        case A2C_PRINT:
            printf("test1\n");
            break;

        case svc_disconnect:
            printf("disconnect\n");
            break;
    }
}

void ReadPackets(struct qw_connection *qwc)
{
    while(NET_GetPacket(qwc, NS_CLIENT, &qwc->net_message, &qwc->net_from))
	{
		if (*(int *)qwc->net_message.data == -1)
		{
			ConnectionlessPacket(qwc);
		}
	}

    while (Get_Message(qwc))
    {
        // remote command packet
		if (*(int *)qwc->net_message_buffer == -1)	{
			ConnectionlessPacket (qwc);
			continue;
		}

		if (qwc->net_message_size < 8) {	
			Com_Printf ("%s: Runt packet\n", NET_AdrToString(&qwc->net_from));
			continue;
		}

		MSG_BeginReading(qwc);
		qwc->netchan.incoming_sequence = MSG_ReadLong(qwc) & 0x7fffffff;
        qwc->netchan.incoming_acknowledged = MSG_ReadLong(qwc) & 0x7fffffff;

		ParseServerMessage(qwc);
    }
}

void QWC_Disconnect(struct qw_connection *qwc)
{
    if (qwc == NULL)
        return;

    NetQW_Delete(qwc->netqw);
    qwc->netqw = NULL;
    qwc->state = qwc_state_idle;
    qwc->run_thread = 0;

    Sys_Thread_DeleteThread(qwc->thread);
    qwc->thread = NULL;
}

void QWC_Quit(struct qw_connection *qwc)
{
    if (qwc == NULL)
        return;

    if (qwc->netqw)
        QWC_Disconnect(qwc);
    qwc->quit = 1;
    qwc->run_thread = 0;

    QWC_Destroy(qwc);
}

void QWC_Set_FPS(struct qw_connection *qwc, unsigned int fps)
{
    if (qwc == NULL)
        return;

    NetQW_SetFPS(qwc->netqw, fps);
    qwc->fps = fps;
}

static void QWC_Thread_Handler(void *arg)
{
    struct qw_connection *qwc;
    unsigned int i;

    qwc = arg;

    i = 0;
    while (qwc->run_thread == 1 && qwc->abort == 0)
    {

        qwc->newtime = Sys_DoubleTime();
        qwc->time = qwc->newtime - qwc->oldtime;
        qwc->oldtime = qwc->newtime;

        qwc->extratime += qwc->time;
        if (qwc->extratime < 1.0/qwc->fps)
        {
            Sleep_Sleep(qwc, (unsigned int)((1.0/qwc->fps-qwc->extratime)*1000000));
            continue;
        }

        qwc->extratime -= 1.0/qwc->fps;

        if (qwc->extratime > 1.0/qwc->fps)
            qwc->extratime = 1.0/qwc->fps;

        if (qwc->netqw != NULL && qwc->quit == 0)
        {
            if (NetQW_GetState(qwc->netqw) != state_uninitialised)
            {
                ReadPackets(qwc);
                qwc->state = qwc_state_connected;
            }
            else if (NetQW_GetState(qwc->netqw) >= state_sendchallenge && NetQW_GetState(qwc->netqw) <= state_sendconnection)
                qwc->state = qwc_state_connecting;
        }
        else
            qwc->state = qwc_state_idle;
    }
    if (qwc->abort)
        QWC_Quit(qwc);
}

void QWC_Connect(struct qw_connection *qwc, char *host, char *userinfo, unsigned int port)
{
    if (qwc == NULL)
        return;
    if (host == NULL)
        return;

    qwc->netqw = (struct NetQW *)NetQW_Create(qwc, host, userinfo, port);
    if (qwc->netqw)
    {
        qwc->state = QWC_CONNECTING;

        qwc->run_thread = 1;
        qwc->thread = Sys_Thread_CreateThread(QWC_Thread_Handler, qwc);

        if (qwc->thread == NULL)
        {
            NetQW_Delete(qwc->netqw);
            qwc->state = QWC_IDLE;
            return;
        }
    }
}

struct qw_connection *QWC_Create(char *packet_dump_filename)
{
    struct qw_connection *qwc;

    qwc = calloc(1, sizeof(struct qw_connection));

    if (qwc == NULL)
        return NULL;

    qwc->packet_dump_filename = packet_dump_filename;
    qwc->fps = 72.0;
    NET_Init(qwc);
    Sleep_Init(qwc);
    Tools_Init(qwc);


    return qwc;
}

void QWC_Destroy(struct qw_connection *qwc)
{
    struct qwc_string *s;

    NET_Shutdown(qwc);

    s = qwc->console_output;

    while (s)
    {
        if (s->done == false)
            free(s->string);
        s = s->next;
    }
    
}


int QWC_GetSleepGranularity(struct qw_connection *qwc)
{
    if (qwc == NULL)
        return -1;

    return qwc->sleep_granularity;
}
void QWC_SetSetSleepGranularity(struct qw_connection *qwc, int sleep_granularity)
{
    if (qwc == NULL)
        return;

    qwc->sleep_granularity = sleep_granularity;
}

void QWC_Say(struct qw_connection *qwc, char *text)
{
    int i;
    char buf[512];

    i = snprintf(buf, sizeof(buf), "%csay %s", clc_stringcmd, text);
    if (i < sizeof(buf))
        NetQW_AppendReliableBuffer(qwc->netqw, buf, i + 1);
}

void QWC_SetSaveConsoleOutput(struct qw_connection *qwc, int save)
{
    if (qwc == NULL)
        return;

    qwc->save_console_output = save;
}

// returns one at a time whoever calls it needs to free it after
char *QWC_GetConsoleOutput(struct qw_connection *qwc)
{
    struct qwc_string *s;

    if (qwc == NULL)
        return NULL;

    if (qwc->console_output == NULL)
        return;

    s = qwc->console_output;

    while (s && s->done == true)
        s = s->next;

    if (s == NULL)
        return NULL;

    s->done = true;

    return s->string;
}

int QWC_GetQuit(struct qw_connection *qwc)
{
    if (qwc == NULL)
        return 1;

    return qwc->quit;
}

enum qwc_state QWC_GetState(struct qw_connection *qwc)
{
    if (qwc == NULL)
        return -1;

    return qwc->state;
}

#ifdef TESTPROG

int main (int argc, char *argv[])
{
    double extratime = 0.001;
    int i;
    int port;

    struct qw_connection *qwc;

    qwc = QWC_Create(NULL);
    if (qwc == NULL)
        return;


    QWC_Connect(qwc, argv[2] , "\\name\\test\\", atoi(argv[1]));
    sleep(6);
    QWC_Say(qwc, "we are buffy!");
    sleep(2);

    QWC_Say(qwc, "resistance is furry!! MEOW");

    sleep(3);
    QWC_Disconnect(qwc);

    sleep(1);

    QWC_Connect(qwc, argv[2] , "\\name\\test\\", atoi(argv[1]));
    sleep(6);
    QWC_Say(qwc, "we are buffy!");
    sleep(2);

    QWC_Say(qwc, "resistance is furry!! MEOW");

    sleep(3);
    QWC_Disconnect(qwc);

    sleep(1);

    QWC_Quit(qwc);

    free(qwc); 
}

#endif

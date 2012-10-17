
struct map_table
{
    char *name;
    int crc;
};

struct map_table map_table[] = {
    {"dm3", 367136248}
}

int MapTable_GetCRC(char *name)
{
    int i;

    for (i=0;i<sizeof(map_table)/sizeof(*map_table);i++)
    {
        if (strcmp(map_table[i].name, name) == 0)
            return map_table[i].crc;
    }

    return 0;
}

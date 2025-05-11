#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include <error_codes.h>
#include "../game/game_structs.h"
#include "../log/log.h"

typedef struct
{
    int    INITIAL_LIFE_UNITS;
    int    LIFE_UNIT_TO_TIME;

    double DENSITY_NOURRITURE;
    double DENSITY_LINEMATE;
    double DENSITY_DERAUMERE;
    double DENSITY_SIBUR;
    double DENSITY_MENDIANE;
    double DENSITY_PHIRAS;
    double DENSITY_THYSTAME;

    int    DELAY_AVANCE;
    int    DELAY_DROITE;
    int    DELAY_GAUCHE;
    int    DELAY_VOIR;
    int    DELAY_INVENTAIRE;
    int    DELAY_PREND;
    int    DELAY_POSE;
    int    DELAY_EXPULSE;
    int    DELAY_BROADCAST;
    int    DELAY_INCANTATION;
    int    DELAY_FORK;
    int    DELAY_CONNECT_NBR;

    bool   SPAWN_MORE_RESOURCES;
    int    SPAWN_MORE_RESOURCES_TIME;
    double SPAWN_MORE_RESOURCES_NOURRITURE;
    double SPAWN_MORE_RESOURCES_LINEMATE;
    double SPAWN_MORE_RESOURCES_DERAUMERE;
    double SPAWN_MORE_RESOURCES_SIBUR;
    double SPAWN_MORE_RESOURCES_MENDIANE;
    double SPAWN_MORE_RESOURCES_PHIRAS;
    double SPAWN_MORE_RESOURCES_THYSTAME;

    log_level LOG_LEVEL;
    char* LOG_FILE_PATH;
    bool LOG_ERASE;

} config_t;

config_t* m_config_content = NULL;

static void m_init_config_content()
{
    if (!m_config_content)
        return;
    m_config_content->INITIAL_LIFE_UNITS = 10;
    m_config_content->LIFE_UNIT_TO_TIME = 126;

    m_config_content->DENSITY_NOURRITURE = 1.0;
    m_config_content->DENSITY_LINEMATE   = 0.02;
    m_config_content->DENSITY_DERAUMERE  = 0.02;
    m_config_content->DENSITY_SIBUR      = 0.04;
    m_config_content->DENSITY_MENDIANE   = 0.04;
    m_config_content->DENSITY_PHIRAS     = 0.04;
    m_config_content->DENSITY_THYSTAME   = 0.005;

    m_config_content->DELAY_AVANCE       = 7;
    m_config_content->DELAY_DROITE       = 7;
    m_config_content->DELAY_GAUCHE       = 7;
    m_config_content->DELAY_VOIR         = 7;
    m_config_content->DELAY_INVENTAIRE   = 1;
    m_config_content->DELAY_PREND        = 7;
    m_config_content->DELAY_POSE         = 7;
    m_config_content->DELAY_EXPULSE      = 7;
    m_config_content->DELAY_BROADCAST    = 7;
    m_config_content->DELAY_INCANTATION  = 300;
    m_config_content->DELAY_FORK         = 42;
    m_config_content->DELAY_CONNECT_NBR  = 0;

    m_config_content->SPAWN_MORE_RESOURCES           = true;
    m_config_content->SPAWN_MORE_RESOURCES_TIME      = 100;
    m_config_content->SPAWN_MORE_RESOURCES_NOURRITURE= 0.5;
    m_config_content->SPAWN_MORE_RESOURCES_LINEMATE  = 0.02;
    m_config_content->SPAWN_MORE_RESOURCES_DERAUMERE = 0.02;
    m_config_content->SPAWN_MORE_RESOURCES_SIBUR     = 0.04;
    m_config_content->SPAWN_MORE_RESOURCES_MENDIANE  = 0.04;
    m_config_content->SPAWN_MORE_RESOURCES_PHIRAS    = 0.04;
    m_config_content->SPAWN_MORE_RESOURCES_THYSTAME  = 0.005;

    m_config_content->LOG_LEVEL = LOG_LEVEL_WARN;
    m_config_content->LOG_FILE_PATH = strdup("log.txt");
    m_config_content->LOG_ERASE = true;
}

void parse_set_initial_density(spawn_ctx* ctx)
{
    if (!m_config_content)
        return;
    ctx->d_nourriture  = m_config_content->DENSITY_NOURRITURE;
    ctx->d_linemate    = m_config_content->DENSITY_LINEMATE;
    ctx->d_deraumere   = m_config_content->DENSITY_DERAUMERE;
    ctx->d_sibur       = m_config_content->DENSITY_SIBUR;
    ctx->d_mendiane    = m_config_content->DENSITY_MENDIANE;
    ctx->d_phiras      = m_config_content->DENSITY_PHIRAS;
    ctx->d_thystame    = m_config_content->DENSITY_THYSTAME;
}

void parse_set_commands_delay(command cmd[MAX_COMMANDS])
{
    if (!m_config_content)
        return;
    cmd[AVANCE].delay       = m_config_content->DELAY_AVANCE;
    cmd[DROITE].delay       = m_config_content->DELAY_DROITE;
    cmd[GAUCHE].delay       = m_config_content->DELAY_GAUCHE;
    cmd[VOIR].delay         = m_config_content->DELAY_VOIR;
    cmd[INVENTAIRE].delay   = m_config_content->DELAY_INVENTAIRE;
    cmd[PREND].delay        = m_config_content->DELAY_PREND;
    cmd[POSE].delay         = m_config_content->DELAY_POSE;
    cmd[EXPULSE].delay      = m_config_content->DELAY_EXPULSE;
    cmd[BROADCAST].delay    = m_config_content->DELAY_BROADCAST;
    cmd[INCANTATION].delay  = m_config_content->DELAY_INCANTATION;
    cmd[FORK].delay         = m_config_content->DELAY_FORK;
    cmd[CONNECT_NBR].delay  = m_config_content->DELAY_CONNECT_NBR;
}

void parse_set_respawn_context(spawn_ctx* ctx)
{
    if (!m_config_content)
        return;
    ctx->period = m_config_content->SPAWN_MORE_RESOURCES_TIME;
    ctx->d_nourriture  = m_config_content->SPAWN_MORE_RESOURCES_NOURRITURE;
    ctx->d_linemate    = m_config_content->SPAWN_MORE_RESOURCES_LINEMATE;
    ctx->d_deraumere   = m_config_content->SPAWN_MORE_RESOURCES_DERAUMERE;
    ctx->d_sibur       = m_config_content->SPAWN_MORE_RESOURCES_SIBUR;
    ctx->d_mendiane    = m_config_content->SPAWN_MORE_RESOURCES_MENDIANE;
    ctx->d_phiras      = m_config_content->SPAWN_MORE_RESOURCES_PHIRAS;
    ctx->d_thystame    = m_config_content->SPAWN_MORE_RESOURCES_THYSTAME;
}

void parse_set_log_config(log_config* log)
{
    log->LOG_ERASE = m_config_content->LOG_ERASE;
    log->LOG_FILE_PATH = m_config_content->LOG_FILE_PATH;
    log->LOG_LEVEL = m_config_content->LOG_LEVEL;
}

bool parse_respawn_resources()
{
    if (!m_config_content)
        return false;

    return m_config_content->SPAWN_MORE_RESOURCES;
}

void parse_set_life_unit(int* life_unit)
{
    if (!m_config_content)
        return;
    
    *life_unit = m_config_content->LIFE_UNIT_TO_TIME;
}

void parse_set_start_life_units(int* start_life_units)
{
    if (!m_config_content)
        return;

    *start_life_units = m_config_content->INITIAL_LIFE_UNITS;
}

void parse_free_config()
{
    free(m_config_content);
    m_config_content = NULL;
}

int parse_config(const char *filename)
{
    char    line[256];
    FILE*   fp;
    char*   p;
    char*   eq;
    char*   key;
    char*   val;
    char    c;

    m_config_content = malloc(sizeof(config_t));

    /* Set default values */
    m_init_config_content();

    fp = fopen(filename, "r");
    if (!fp)
    {
        perror("Unable to open config file");
        return ERROR;
    }

    while (fgets(line, sizeof(line), fp))
    {
        p = line;

        while (isspace((unsigned char)*p)) p++;

        if (*p == '#' || *p == '\0' || *p == '\n')
            continue;

        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = p;
        val = eq + 1;

        char *kend = key + strlen(key) - 1;
        while (kend > key && isspace((unsigned char)*kend)) *kend-- = '\0';

        while (isspace((unsigned char)*val)) val++;
        char *vend = val + strlen(val) - 1;
        while (vend > val && (isspace((unsigned char)*vend) || *vend=='\r' || *vend=='\n'))
            *vend-- = '\0';

        if (strcmp(key, "INITIAL_LIFE_UNITS") == 0)
            m_config_content->INITIAL_LIFE_UNITS = atoi(val);
        else if (strcmp(key, "LIFE_UNIT_TO_TIME") == 0)
            m_config_content->LIFE_UNIT_TO_TIME = atoi(val);
        else if (strcmp(key, "DENSITY_NOURRITURE") == 0)
            m_config_content->DENSITY_NOURRITURE = strtod(val, NULL);
        else if (strcmp(key, "DENSITY_LINEMATE") == 0)
            m_config_content->DENSITY_LINEMATE = strtod(val, NULL);
        else if (strcmp(key, "DENSITY_DERAUMERE") == 0)
            m_config_content->DENSITY_DERAUMERE = strtod(val, NULL);
        else if (strcmp(key, "DENSITY_SIBUR") == 0)
            m_config_content->DENSITY_SIBUR = strtod(val, NULL);
        else if (strcmp(key, "DENSITY_MENDIANE") == 0)
            m_config_content->DENSITY_MENDIANE = strtod(val, NULL);
        else if (strcmp(key, "DENSITY_PHIRAS") == 0)
            m_config_content->DENSITY_PHIRAS = strtod(val, NULL);
        else if (strcmp(key, "DENSITY_THYSTAME") == 0)
            m_config_content->DENSITY_THYSTAME = strtod(val, NULL);
        else if (strcmp(key, "DELAY_AVANCE") == 0)
            m_config_content->DELAY_AVANCE = atoi(val);
        else if (strcmp(key, "DELAY_DROITE") == 0)
            m_config_content->DELAY_DROITE = atoi(val);
        else if (strcmp(key, "DELAY_GAUCHE") == 0)
            m_config_content->DELAY_GAUCHE = atoi(val);
        else if (strcmp(key, "DELAY_VOIR") == 0)
            m_config_content->DELAY_VOIR = atoi(val);
        else if (strcmp(key, "DELAY_INVENTAIRE") == 0)
            m_config_content->DELAY_INVENTAIRE = atoi(val);
        else if (strcmp(key, "DELAY_PREND") == 0)
            m_config_content->DELAY_PREND = atoi(val);
        else if (strcmp(key, "DELAY_POSE") == 0)
            m_config_content->DELAY_POSE = atoi(val);
        else if (strcmp(key, "DELAY_EXPULSE") == 0)
            m_config_content->DELAY_EXPULSE = atoi(val);
        else if (strcmp(key, "DELAY_BROADCAST") == 0)
            m_config_content->DELAY_BROADCAST = atoi(val);
        else if (strcmp(key, "DELAY_INCANTATION") == 0)
            m_config_content->DELAY_INCANTATION = atoi(val);
        else if (strcmp(key, "DELAY_FORK") == 0)
            m_config_content->DELAY_FORK = atoi(val);
        else if (strcmp(key, "DELAY_CONNECT_NBR") == 0)
            m_config_content->DELAY_CONNECT_NBR = atoi(val);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES") == 0)
        {
            c = val[0];
            m_config_content->SPAWN_MORE_RESOURCES = (c=='y'||c=='Y'||c=='1');
        }
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_TIME") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_TIME = atoi(val);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_NOURRITURE") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_NOURRITURE = strtod(val, NULL);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_LINEMATE") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_LINEMATE = strtod(val, NULL);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_DERAUMERE") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_DERAUMERE = strtod(val, NULL);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_SIBUR") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_SIBUR = strtod(val, NULL);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_MENDIANE") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_MENDIANE = strtod(val, NULL);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_PHIRAS") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_PHIRAS = strtod(val, NULL);
        else if (strcmp(key, "SPAWN_MORE_RESOURCES_THYSTAME") == 0)
            m_config_content->SPAWN_MORE_RESOURCES_THYSTAME = strtod(val, NULL);
        else if (strcmp(key, "LOG_LEVEL") == 0)
            m_config_content->LOG_LEVEL = atoi(val);
        else if (strcmp(key, "LOG_FILE_PATH") == 0)
        {
            free(m_config_content->LOG_FILE_PATH);
            m_config_content->LOG_FILE_PATH = strdup(val);
        }
        else if (strcmp(key, "LOG_ERASE") == 0)
        {
            c = val[0];
            m_config_content->LOG_ERASE = (c=='y'||c=='Y'||c=='1');
        }
        
    }

    fclose(fp);
    return SUCCESS;
}

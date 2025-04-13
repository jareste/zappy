#ifndef GAME_H
#define GAME_H

#include "game_structs.h"

int game_play();
int game_init(int width, int height, char **teams, int nb_clients);
int game_register_player(int fd, char *team_name);

#endif /* GAME_H */

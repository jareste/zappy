#ifndef GAME_H
#define GAME_H

#include "game_structs.h"

int game_play();
int game_init(int width, int height, char **teams, int nb_clients, int nb_teams);
void game_clean();
int game_register_player(int fd, char *team_name);
int game_register_observer(int fd);
int game_execute_command(int fd, char *cmd, char *arg);

int game_get_client_count();
int game_get_team_count();
int game_get_team_remaining_clients(int fd);
void game_get_map_size(int *width, int *height);

int game_kill_player(int fd);

#endif /* GAME_H */

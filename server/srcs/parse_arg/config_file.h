#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

void parse_set_initial_density(spawn_ctx* ctx);
void parse_set_commands_delay(command cmd[MAX_COMMANDS]);
int parse_config(const char *filename);

bool parse_respawn_resources();
void parse_set_respawn_context(spawn_ctx* ctx);
void parse_set_start_life_units(int* start_life_units);
void parse_set_life_unit(int* life_unit);
void parse_set_log_config(log_config* log);

void parse_free_config();

#endif /* CONFIG_FILE_H */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#define CONFIG_MAX_RULES 100
#define CONFIG_FILE_PATH "config.txt"

/**
example usuage:
config_init();
char value[100];

config_read("some_key", value);
printf("found: %s\n", value);

config_exit();	
 **/

typedef struct Config_rule {
    char* key;
    char* value;
} Config_rule;

Config_rule config_rules[CONFIG_MAX_RULES];
int config_init();
void config_read(char const * const key, char* const output);
void config_exit();

#endif
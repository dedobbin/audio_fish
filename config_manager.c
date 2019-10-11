#include "config_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

int rule_compare (const void* a, const void* b);
char* search (const Config_rule const* start, const Config_rule const* end, const char const* key);
int n_rules;
int config_init(){
	printf("config_init, reading from %s, max rules: %d\n", CONFIG_FILE_PATH, CONFIG_MAX_RULES);
	n_rules = 0;
	FILE* fp = fopen(CONFIG_FILE_PATH, "r");
	if (!fp) return -1;
	fseek(fp, 0L, SEEK_END);
	int filesize = ftell(fp);
	rewind(fp);

	char* content = malloc(filesize);
	fread(content, filesize, 1 , fp);

	const char* delims = "=\n";
	const char* empty = "EOF";
	char** content_address_p;
	const char* token;
	char* p;
	int i = 0;

	//TODO: Last key and value pair give invalid reads (valgrind)
	content_address_p = &content;
	while (*content_address_p) {
		p = *content_address_p;
		while (0 != (token = strsep(&p, delims))) {
			if (0 == *token || n_rules >= CONFIG_MAX_RULES) {
				token = empty;
			} else if (i%2==0){
				config_rules[n_rules].key = strdup(token);
			} else {
				config_rules[n_rules].value = strdup(token);
				n_rules++;
			}
			i++;
		}
		++content_address_p;
	}
	
	free(content);
	qsort(config_rules, n_rules, sizeof(Config_rule), rule_compare);

	fclose(fp);
	return 1;
}

void config_exit(){
    int i;
    if (config_rules){
        for (i = 0; i < n_rules; i ++){
            free(config_rules[i].key);
            free(config_rules[i].value);
        }
    }
}

void config_read(char const * const key, char* const output){
	char* found = search(config_rules, config_rules + n_rules, key);
	strcpy(output, found);
}

/************* internal support stuff ************/

/**
 * Only works if sorted on key
 */
char* search (const Config_rule const* start, const Config_rule const* end, const char const* key){
    int arr_len = end - start;
    Config_rule center = start[arr_len / 2];
    int cmp = strcmp(center.key, key);
	if (cmp == 0){
        return center.value;
	} else if (cmp > 0  && arr_len > 1){
        return search(start, start + arr_len / 2, key);
    } else if (cmp < 0 && arr_len > 1){
        return search(start + arr_len / 2, end, key);
    }
	return "";
}

int rule_compare (const void const* a, const void const* b){
	return strcmp(((Config_rule*)a)->key, ((Config_rule*)b)->key);
}

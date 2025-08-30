#ifndef DBMANAGER_H
#define DBMANAGER_H
#include <stdint.h>

char* get_full_path(char* path);
void init_appdir();
void get_identity(uint8_t privkey[32], uint8_t name[64], uint8_t prefix[17]);
void save_identity(uint8_t privkey[32], uint8_t name[64], uint8_t id[32]);

#endif
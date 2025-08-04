#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "common.h"
#include <stdint.h>

User get_account(uint8_t privkey[32], char username[17], char name[32]);

#endif
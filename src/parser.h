/** @file
 *  @brief Header file for parser.c
 */

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "structs.h"

void start_handle_thread(struct user*);
void handle_connection(struct user*);

#endif // PARSER_H_INCLUDED

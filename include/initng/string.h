/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef INITNG_STRING_H
#define INITNG_STRING_H


/*
 * MACRO JUMP_SPACES, will increase string, until it founds a valid token.
 */

#define JUMP_SPACES(s) while ((s)[0] == ' ' || (s)[0] == '\t') (s)++;

#define JUMP_NSPACES(s) while ((s)[0] == ' ' || (s)[0] == '\t' || (s)[0] == '\n') (s)++;

/*
 * MACRO JUMP_TO_NEXT_ROW, will increase string, until it hits a new row token.
 * and after that also jump the new row token.
 */

#define JUMP_TO_NEXT_ROW(s) \
    while((s)[0] != '\0' && (s)[0] != '\n' && (s)[0] != ';') \
	(s)++; \
    if((s)[0]!='\0') (s)++;

#define JUMP_TO_NEXT_VROW(s) \
    while((s)[0] != '\0' && (s)[0] != ';') \
	(s)++; \
    if((s)[0]!='\0') (s)++;

/*
 * REALLY_JUMP_TO_NEXT_ROW wont stop on a ';' char.
 */
#define REALLY_JUMP_TO_NEXT_ROW(s) \
    while((s)[0] != '\0' && (s)[0] != '\n') \
	(s)++; \
    if((s)[0]!='\0') (s)++;

/*
 * This ones jumps all chars, until a space or tab,
 * then jump the spaces to, so it will point to next word.
 */
#define JUMP_TO_NEXT_WORD(s) \
    while((s)[0] && (s)[0] != ' ' && (s)[0] != '\t') \
	(s)++; \
    while ((s)[0] == ' ' || (s)[0] == '\t') (s)++;

char *initng_string_dup_next_word(const char **string);
char *initng_string_dup_line(char **string);


const char *initng_string_basename(const char *string);
int initng_string_strip_end(char **string);
char *initng_string_dirname(const char *string);

void initng_string_replace(char * dest, char * str, const char * n, const char * r);

/* to use with split_delim */
#define WHITESPACE " \t\n\r\v"
char **initng_string_split_delim(const char *string, const char *delim,
				 size_t *argc);
void initng_string_split_delim_free(char **strs);


/* pattern searching */
int initng_string_match(const char *string, const char *pattern);
int initng_string_match_in_service(const char *string, const char *pattern);
int initng_string_mprintf(char **p, const char *format, ...);

void initng_string_fix_escapes(char *str);

#endif /* INITNG_STRING_H */

/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2009 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2009 Ismael Luceno <ismael.luceno@gmail.com>
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

#ifndef INITNG_HASH_H
#define INITNG_HASH_H

#include <stdint.h>
#include <unistd.h>

typedef uint32_t hash_t;


/* This hash table implementation aims at wasting as less space as possible
 * by providing no flexibility at all :P.
 *
 * It's indexed by the seven most significant bits of the hashes, which is
 * important to conserve the same order as in the list.
 *
 * Considering that the biggest list initng uses is active_db, and normally
 * it's bellow 90 items, 128 buckets should be enough to avoid most collisions.
 *
 * In the case of a collision, since the order in the list is the same, we
 * know it's near, so we can reach it by just walking in the right direction.
 */

#define initng_hash_lookup(table, hash) table[hash >> 25]

typedef void * hash_table[128];

hash_t initng_hash(const char *key, size_t len);

#endif

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

#include <initng.h>
#include <sys/types.h>
#include <stdint.h>

/**
 * Perform a hash on a string.
 *
 * @param str   start of buffer to hash
 * @return      hash
 */
hash_t initng_hash_str(cost char *str)
{
	return initng_hash_buf(key, strlen(key))
}

/**
 * Perform a hash on a buffer.
 *
 * @param buf   start of buffer to hash
 * @param len   length of buffer in octets
 * @return      hash
 */
hash_t initng_hash_buf(const char *buf, size_t len)
{
	hash_t hash = 0;

	while (len--) {
		hash += *key++;
		hash += hash << 10;
		hash += hash >>  6;
	}

	hash += hash <<  3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return hash;
}

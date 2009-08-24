/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2007 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2007 Ismael Luceno <ismael.luceno@gmail.com>
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

/* FIXME : Avoid __typeof__ to be fully standard.
 */

#ifndef INITNG_LIST_H
#define INITNG_LIST_H

struct _list_s {
	struct _list_s *next, *prev;
};

typedef struct _list_s list_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

static inline void initng_list_init(list_t *list)
{
	list->next = list;
	list->prev = list;
}

/**
 * Add a new entry.
 *
 * @param newe   New entry to be added.
 * @param head   List head to add it after.
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void initng_list_add(list_t *newe, list_t *head)
{
	/* make sure its not already added */
	if (newe->next || newe->prev)
		return;

	head->next->prev = newe;
	newe->next = head->next;
	newe->prev = head;
	head->next = newe;
}

/**
 * Add a new entry at the tail.
 *
 * @param newe   New entry to be added.
 * @param head   List head to add it before.
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void initng_list_add_tail(list_t *newe, list_t *head)
{
	/* make sure its not already added */
	if (newe->next || newe->prev)
		return;

	head->prev->next = newe;
	head->prev = newe;
	newe->next = head;
	newe->prev = head->prev;
}

/**
 * Deletes an entry from the list.
 *
 * @param entry   The element to delete from the list.
 *
 * Note: initng_list_empty on entry does not return true after this, the entry
 *       is in an undefined state.
 */
static inline void initng_list_del(list_t *entry)
{
	/* only if list is added on a struct */
	if (entry->next || entry->prev) {
		entry->next->prev = entry->prev;
		entry->prev->next = entry->next;
		entry->next = NULL;
		entry->prev = NULL;
	}
}

/**
 * Delete from one list and add as another's head.
 *
 * @param list   The entry to move.
 * @param head   The head that will precede our entry.
 */
static inline void initng_list_move(list_t *list, list_t *head)
{
	initng_list_del(list);
	initng_list_add(list, head);
}

/**
 * Delete from one list and add as another's tail.
 *
 * @param list   The entry to move.
 * @param head   The head that will follow our entry.
 */
static inline void list_move_tail(list_t *list, list_t *head)
{
	initng_list_del(list);
	initng_list_add_tail(list, head);
}

/**
 * Tests whether a list is empty.
 *
 * @param list   The list to test.
 */
static inline int initng_list_isempty(list_t *list)
{
	return list->next == list;
}

/**
 * Join two lists and re-initialize the emptied list.
 *
 * @param list   The new list to add.
 * @param head   The place to add it in the first list.
 *
 * The list at @list is reinitialized.
 */
static inline void initng_list_join(list_t *list, list_t *head)
{
	list_t *first;
	list_t *last;
	list_t *at;

	if (initng_list_isempty(list))
		return;

	first = list->next;
	last = list->prev;
	at = head->next;

	first->prev = head;
	head->next = first;
	last->next = at;
	at->prev = last;
	initng_list_init(list);
}

/**
 * Get the struct for this entry.
 *
 * @param ptr      The &struct list_head pointer.
 * @param type     The type of the struct this is embedded in.
 * @param member   The name of the list_struct within the struct.
 */
#define initng_list_entry(ptr, type, member) \
	((type *)((void *)(ptr) - (void *)(&((type *)0)->member)))


/**
 * Iterate over a list of given type.
 *
 * @param pos      The type * to use as a loop counter.
 * @param head     The head for your list.
 * @param member   The name of the list_struct within the struct.
 */
#define initng_list_foreach(pos, head, member) \
	for (pos = initng_list_entry((head)->next, __typeof__(*pos), member); \
	     &pos->member != (head); \
	     pos = initng_list_entry(pos->member.next, __typeof__(*pos), member))

/**
 * Iterate over list of given type in reverse order.
 *
 * @param pos      The type * to use as a loop counter.
 * @param head     The head for your list.
 * @param member   The name of the list_struct within the struct.
 */
#define initng_list_foreach_rev(pos, head, member) \
	for (pos = initng_list_entry((head)->prev, __typeof__(*pos), member); \
	     &pos->member != (head); \
	     pos = initng_list_entry(pos->member.prev, __typeof__(*pos), member))


/**
 * Iterate over a list of given type, safe against removal of list entry.
 *
 * @param pos      The type * to use as a loop counter.
 * @param n        Another type * to use as temporary storage.
 * @param head     The head for your list.
 * @param member   The name of the list_struct within the struct.
 */
#define initng_list_foreach_safe(pos, n, head, member) \
	for (pos = initng_list_entry((head)->next, __typeof__(*pos), member), \
	     n = initng_list_entry(pos->member.next, __typeof__(*pos), member); \
	     &pos->member != (head); \
	     pos = n, \
	     n = initng_list_entry(n->member.next, __typeof__(*n), member))

/**
 * Iterate over list of given type, safe against removal of list entry.
 *
 * @param pos      The type * to use as a loop counter.
 * @param n        Another type * to use as temporary storage.
 * @param head     The head for your list.
 * @param member   The name of the list_struct within the struct.
 */
#define initng_list_foreach_rev_safe(pos, n, head, member) \
	for (pos = initng_list_entry((head)->prev, __typeof__(*pos), member), \
	     n = initng_list_entry(pos->member.prev, __typeof__(*pos), member); \
	     &pos->member != (head); \
	     pos = n, \
	     n = initng_list_entry(n->member.prev, __typeof__(*n), member))


#endif /* !defined(INITNG_LIST_H) */

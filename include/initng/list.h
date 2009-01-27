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

#ifndef INITNG_LIST_H
#define INITNG_LIST_H

struct list_s {
	struct list_s *next, *prev;
};

typedef struct list_s list_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

static inline void initng_list_init(list_t *list)
{
	list->next = list;
	list->prev = list;
}

/**
 * initng_list_add - add a new entry
 * @newe: new entry to be added
 * @head: list head to add it after
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
 * initng_list_add_tail - add a new entry
 * @newe: new entry to be added
 * @head: list head to add it before
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
 * initng_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
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
 * initng_list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void initng_list_move(list_t *list, list_t *head)
{
	initng_list_del(list);
	initng_list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(list_t *list, list_t *head)
{
	initng_list_del(list);
	initng_list_add_tail(list, head);
}

/**
 * initng_list_isempty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int initng_list_isempty(list_t *head)
{
	return head->next == head;
}

/**
 * initng_list_join - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
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
 * initng_list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define initng_list_entry(ptr, type, member) \
	((type *)((void *)(ptr) - (void *)(&((type *)0)->member)))


/**
 * initng_list_foreach - iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define initng_list_foreach(pos, head, member) \
	for (pos = initng_list_entry((head)->next, typeof(*pos), member); \
	     &pos->member != (head); \
	     pos = initng_list_entry(pos->member.next, typeof(*pos), member))

/**
 * initng_list_foreach - iterate over list of given type in reverse order
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define initng_list_foreach_rev(pos, head, member) \
	for (pos = initng_list_entry((head)->prev, typeof(*pos), member); \
	     &pos->member != (head); \
	     pos = initng_list_entry(pos->member.prev, typeof(*pos), member))


/**
 * initng_list_foreach_safe - iterate over list of given type safe against
 *                            removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define initng_list_foreach_safe(pos, n, head, member) \
	for (pos = initng_list_entry((head)->next, typeof(*pos), member), \
	     n = initng_list_entry(pos->member.next, typeof(*pos), member); \
	     &pos->member != (head); \
	     pos = n, \
	     n = initng_list_entry(n->member.next, typeof(*n), member))

/**
 * initng_list_foreach_rev_safe - iterate over list of given type safe against
 *                                removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define initng_list_foreach_rev_safe(pos, n, head, member) \
	for (pos = initng_list_entry((head)->prev, typeof(*pos), member), \
	     n = initng_list_entry(pos->member.prev, typeof(*pos), member); \
	     &pos->member != (head); \
	     pos = n, \
	     n = initng_list_entry(n->member.prev, typeof(*n), member))


#endif /* ! INITNG_LIST_H */

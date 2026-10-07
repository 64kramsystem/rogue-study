/*
 * Functions for dealing with linked lists of goodies
 * Functions with names starting with an "_" have compainion #defines
 * in rogue.h which take the address of the first argument and pass it on.
 *
 * list.c	1.4 (A.I. Design) 12/5/85
 */

#include "rogue.h"
#include "screen.h"

static void	*find_free_entity_slot(void);

/*
 * detach:
 *	Takes an item out of whatever linked list it might be in
 */
void
list_detach(list, item)
	register Entity **list, *item;
{
	if (*list == item)
		*list = next(item);
	if (prev(item) != NULL) item->previous_entity->next_entity = next(item);
	if (next(item) != NULL) item->next_entity->previous_entity = prev(item);
	item->next_entity = NULL;
	item->previous_entity = NULL;
}

/*
 * _attach:
 *	add an item to the head of a list
 */
void
list_attach(list, item)
	register Entity **list, *item;
{
	if (*list != NULL)
	{
		item->next_entity = *list;
		(*list)->previous_entity = item;
		item->previous_entity = NULL;
	}
	else
	{
		item->next_entity = NULL;
		item->previous_entity = NULL;
	}
	*list = item;
}

/*
 * _free_list:
 *	Throw the whole blamed thing away
 */
void
list_free(list_head)
	register Entity **list_head;
{
	register Entity *item;

	while (*list_head != NULL)
	{
	item = *list_head;
	*list_head = next(item);
	release_entity(item);
	}
}

/*
 * allocate_entity
 *	Get a new item with a specified size
 */
Entity *
allocate_entity()
{
	register Entity *item;
#ifdef DEBUG
	if ((item = (Entity *) find_free_entity_slot()) == NULL)
		if (me())show_message("no more things!");
	else
#else
	if ((item = (Entity *) find_free_entity_slot()) != NULL)
#endif //DEBUG
			 item->next_entity = item->previous_entity = NULL;
	return item;
}

/*
 * find_free_entity_slot: simple allocation of a Entity
 */
static
void *  //@ maybe should be Entity*, as this is a specialized malloc()
find_free_entity_slot()
{
	register int i;

	for (i=0;i<MAXITEMS;i++)
	{
		if (entity_slot_used[i] == 0)
		{
			if (++allocated_entity_count > peak_entity_count)
			peak_entity_count = allocated_entity_count;
			entity_slot_used[i]++;
			fill_bytes(&entity_pool[i],sizeof(Entity),0);
			return &entity_pool[i];
		}
	}
	return NULL;
}

/*
 * release_entity:
 *	Free up an item
 */
int
release_entity(item)
	register Entity *item;
{
	register int i;

	for (i=0;i<MAXITEMS;i++)
	{
		if (item == &entity_pool[i])
		{
			--allocated_entity_count;
			entity_slot_used[i] = 0;
			return 1;
		}
	}
	return 0;
}

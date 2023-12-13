#include "list.h"

#include <stdio.h>
#include <stdlib.h>

static void free_node(struct node *node, void (*callback_free)(void *));

enum move { back, forward };

static struct node *list_search(int index, struct list *list);

void list_init(struct list *list, void (*callback_free)(void *))
{
	list->first = NULL;
	list->last = NULL;
	list->len = 0;
	list->callback_free = callback_free;
}

void list_append(void *data, struct list *list)
{
	struct node *node = malloc(sizeof(struct node));
	node->data = data;

	if(list->len == 0)
	{
		list->first = node;
		list->last = node;

		node->prev = NULL;
		node->next = NULL;
	}
	else
	{
		list->last->next = node;

		node->prev = list->last;
		node->next = NULL;

		list->last = node;
	}
	list->len++;
}

static struct node *list_search(int index, struct list *list)
{
	int middle = list->len / 2, count;
	struct node *node;
	enum move mv;

	if(middle > index)
	{
		mv = forward;
		count = index;
	}
	else
	{
		mv = back;
		count = list->len - index - 1;
	}

	node = (mv == forward) ? list->first : list->last;
	for(; count != 0; count--)
	{
		switch(mv)
		{
			case forward:
				node = node->next;
				break;
			case back:
				node = node->prev;
				break;
		}
	}

	return node;
}

void *list_get(int index, struct list *list)
{
	return list_search(index, list)->data;
}

void list_remove(struct node *node, struct list *list)
{
	if(list->first == list->last)
	{
		list->first = NULL;
		list->last = NULL;
	}
	else if(node == list->first)
	{
		list->first = node->next;
		list->first->prev = NULL;
	}
	else if(node == list->last)
	{
		list->last = node->prev;
		list->last->next = NULL;
	}
	else 
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}

	free_node(node, list->callback_free);

	list->len--;
}

void list_clean(struct list *list)
{
	struct node *node;
	for(node = list->first; node; node = node->next)
		free_node(node, list->callback_free);
}

static void free_node(struct node *node, void(*callback_free)(void *))
{
	if(callback_free)
		callback_free(node->data);
	free(node);
}

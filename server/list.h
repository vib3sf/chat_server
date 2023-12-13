#ifndef LIST_H
#define LIST_H

struct node
{
	void *data;
	struct node *prev;
	struct node *next;
};

struct list
{
	struct node *first;
	struct node *last;
	int len;
	void (*callback_free)(void *);
};

void list_init(struct list *list, void (*callback_free)(void *));
void list_append(void *data, struct list *list);
void *list_get(int index, struct list *list);
void list_remove(struct node *node, struct list *list);
void list_clean(struct list *list);

#endif

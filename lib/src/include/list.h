#ifndef LIST_H
#define LIST_H

struct list_elem {
  struct list_elem *next;
  struct list_elem *prev;
  void *data;
};

struct list {
	struct list_elem *head;
	struct list_elem *tail;
};

#define LIST_INIT(l) do { (l)->head = NULL; (l)->tail = NULL; } while(0)
/* fixme double linked ? */
#define LIST_INSERT_TAIL(l, n) do { if((l)->head == NULL) { (l)->head = (struct list_elem*)(n);} else {(l)->tail->next = (struct list_elem*)(n); } (l)->tail = (struct list_elem *)(n); } while(0)

#define LIST_INSERT_HEAD(l,n) do { (n)->next = (l)->head; (l)->head = (struct list_elem*)(n); } while(0)

/* check for NULL? */
#define LIST_REMOVE_HEAD(l) do { (l)->head = (l)->head->next; } while(0)

/* FIXME: Implement! */
#define LIST_REMOVE_ELEM(l, n) do { } while(0)

#define LIST_GET_HEAD(l) ((l)->head == NULL?NULL:((l)->head->data))
#define LIST_POP_HEAD(l) do { (l)->head = (l)->head->next; if((l)->head == NULL) (l)->tail = NULL; } while(0)
#endif

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
#define LIST_INSERT_TAIL(l, n) do {                \
  if((l)->head == NULL) { 				\
    (l)->head = (struct list_elem*)(n);            \
    ((struct list_elem*)(n))->prev = NULL;		   \
  } else {                                         \
    (l)->tail->next = (struct list_elem*)(n);      \
  }                                                \
  ((struct list_elem*)(n))->prev = (l)->tail;	   \
  (l)->tail = (struct list_elem *)(n);             \
  ((struct list_elem*)(n))->next = NULL;	   \
} while(0)

#define LIST_INSERT_HEAD(l,n) do {                 \
  (n)->next = (l)->head;                           \
  (struct list_elem*)(n)->prev = NULL;		   \
  (l)->head = (struct list_elem*)(n);              \
} while(0)

/* check for NULL? */
#define LIST_REMOVE_HEAD(l) do {                   \
  (l)->head = (l)->head->next;                     \
  if((l)->head != NULL) {                          \
    ((struct list*)(l))->head->prev = NULL;	   \
  } else {                                         \
    (l)->tail = NULL;                              \
  }						   \
} while(0)


/* FIXME: Implement! */
#define LIST_REMOVE_ELEM(l, n) do {                                      \
    if(((struct list_elem*)n)->prev != NULL) {                           \
      ((struct list_elem*)n)->prev->next = ((struct list_elem*)n)->next; \
  }                                                                      \
    if(((struct list_elem*)n)->next != NULL) {                           \
      ((struct list_elem*)n)->next->prev = ((struct list_elem*)n)->prev; \
  }                                                                      \
    if(((struct list_elem*)n)== (l)->head) {                             \
      (l)->head = (l)->head->next;	                                 \
  }                                                                      \
  if(((struct list_elem*)n) == (l)->tail) {                              \
    (l)->tail = (l)->tail->prev;                                         \
  }                                                                      \
} while(0)

#define LIST_GET_HEAD(l) ((l)->head == NULL?NULL:((l)->head->data))

#define LIST_PRINT(l) do {                                               \
struct list_elem *le;                                                    \
printf("list head %p tail %p\n", (l)->head, (l)->tail);                  \
 for(le = (l)->head; le != NULL; le=le->next) {		                 \
  printf("elem: %p next: %p prev: %p data %p\n",                         \
	 le, le->next, le->prev, le->data);                              \
 }                                                                       \
} while(0)
#endif

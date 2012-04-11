#ifndef LIST_H
#define LIST_H

struct lwsf_list_elem {
  struct lwsf_list_elem *next;
  struct lwsf_list_elem *prev;
  void *data;
};

struct lwsf_list {
	struct lwsf_list_elem *head;
	struct lwsf_list_elem *tail;
};

#define LIST_INIT(l) do { (l)->head = NULL; (l)->tail = NULL; } while(0)
/* fixme double linked ? */
/* Fixme this needs to be done atomically */
#define LIST_INSERT_TAIL(l, n) do {                \
  if((l)->head == NULL) { 				\
    (l)->head = (struct lwsf_list_elem*)(n);            \
    ((struct lwsf_list_elem*)(n))->prev = NULL;		   \
  } else {                                         \
    (l)->tail->next = (struct lwsf_list_elem*)(n);      \
  }                                                \
  ((struct lwsf_list_elem*)(n))->next = NULL;	   \
  ((struct lwsf_list_elem*)(n))->prev = (l)->tail;	   \
  (l)->tail = (struct lwsf_list_elem *)(n);             \
} while(0)

#define LIST_INSERT_HEAD(l,n) do {                 \
	((struct lwsf_list_elem*)(n))->next = (l)->head;   \
	((struct lwsf_list_elem*)(n))->prev = NULL;	   \
  (l)->head = (struct lwsf_list_elem*)(n);              \
} while(0)

/* check for NULL? */
#define LIST_REMOVE_HEAD(l) do {                   \
  (l)->head = (l)->head->next;                     \
  if((l)->head != NULL) {                          \
    ((struct lwsf_list*)(l))->head->prev = NULL;	   \
  } else {                                         \
    (l)->tail = NULL;                              \
  }						   \
} while(0)


/* FIXME: Implement! */
#define LIST_REMOVE_ELEM(l, n) do {                                      \
    if(((struct lwsf_list_elem*)n)->prev != NULL) {                           \
      ((struct lwsf_list_elem*)n)->prev->next = ((struct lwsf_list_elem*)n)->next; \
  }                                                                      \
    if(((struct lwsf_list_elem*)n)->next != NULL) {                           \
      ((struct lwsf_list_elem*)n)->next->prev = ((struct lwsf_list_elem*)n)->prev; \
  }                                                                      \
    if(((struct lwsf_list_elem*)n)== (l)->head) {                             \
      (l)->head = (l)->head->next;	                                 \
  }                                                                      \
  if(((struct lwsf_list_elem*)n) == (l)->tail) {                              \
    (l)->tail = (l)->tail->prev;                                         \
  }                                                                      \
} while(0)

#define LIST_GET_HEAD(l) ((l)->head == NULL?NULL:((l)->head->data))

#define LIST_PRINT(l) do {                                               \
struct lwsf_list_elem *le;                                                    \
printf("lwsf_list head %p tail %p\n", (l)->head, (l)->tail);                  \
 for(le = (l)->head; le != NULL; le=le->next) {		                 \
  printf("elem: %p next: %p prev: %p data %p\n",                         \
	 le, le->next, le->prev, le->data);                              \
 }                                                                       \
} while(0)
#endif

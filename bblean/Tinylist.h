// ==============================================================
// Tinylist - defines

#ifndef __TINYLIST_H
#define __TINYLIST_H

struct list_node { struct list_node *next; void *v; };
//struct string_node { struct string_node *next; TCHAR str[1]; };

#define dolist(_e,_l) for (_e=(_l);_e;_e=_e->next)

void append_node (void *a,const void *e);
void cons_node (void *a, const void *e);
void remove_node (void *a, const void *e);
void remove_item(void *a, void *e);
void delete_assoc(void *a, void *e);
void reverse_list (void *d);
int listlen(const void *p);
void *nth_node(const void*, int);
void *member(const void *a, const void *e);
void *assoc_ptr(const void *a, const void *e);
void *assoc(const void *a, const void *e);
void *new_node(const void *p);
void freeall(void *p);

TCHAR *new_str(const TCHAR*);
void free_str(TCHAR**);
void replace_str(TCHAR **s, const TCHAR *n);

void append_string_node(struct string_node **p, const TCHAR *s);
struct string_node *new_string_node(const TCHAR *s);


/*----------------------------------------------------------------------------*/
#endif

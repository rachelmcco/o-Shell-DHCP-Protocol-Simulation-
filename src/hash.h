#ifndef __cs361_hash__
#define __cs361_hash__

#include <stdbool.h>

void hash_dump (void); // for debugging if needed

void hash_destroy (void);
void hash_init (size_t);
char *hash_find (char *);
bool hash_insert (char *, char *);
char **hash_keys (void);
bool hash_remove (char *);

#endif

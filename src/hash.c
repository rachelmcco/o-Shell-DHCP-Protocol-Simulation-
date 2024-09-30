#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash.h"

static ssize_t find_index (char *);
static unsigned long hash (unsigned char *);
static bool insert_help (char *, char *, bool);
static void rehash (size_t);

#define MINSIZE 100
#define PRIME 97

typedef struct kvpair
{
  char *key;
  char *value;
  bool alive;
} kvpair_t;

static kvpair_t *table = NULL;
static size_t capacity = 0;
static size_t entries = 0;

void
hash_destroy (void)
{
  if (table == NULL)
    return;

  for (size_t i = 0; i < capacity; i++)
    {
      if (table[i].key != NULL)
        {
          free (table[i].key);
          table[i].key = NULL;
        }
      if (table[i].value != NULL)
        {
          free (table[i].value);
          table[i].value = NULL;
        }
    }
  free (table);
}

/* Dumps the table contents to STDOUT (useful for debugging) */
void
hash_dump (void)
{
  printf ("TABLE:\n");
  for (size_t i = 0; i < capacity; i++)
    if (table[i].key != NULL)
      printf ("  [%zd].%s = %s%s\n", i, table[i].key, table[i].value,
              (!table[i].alive ? " [deleted]" : ""));
}

/* Initializes the hash table to a given size (minimum 100) */
void
hash_init (size_t size)
{
  if (table != NULL)
    free (table);

  // Minimum of 100 entries to start
  if (size < MINSIZE)
    size = MINSIZE;

  table = calloc (size, sizeof (kvpair_t));
  capacity = size;
  entries = 0;
}

/* Find the value for a given key. Returns NULL if there is no entry for
   the given key. */
char *
hash_find (char *key)
{
  if (table == NULL) // uninitialized table
    return NULL;

  ssize_t index = find_index (key);
  assert (index >= 0);

  if (table[index].key == NULL) // key not found
    return NULL;

  // If .key is not NULL, find_index found a key match
  assert (!strcmp (table[index].key, key));
  if (table[index].alive) // not marked for deletion
    return table[index].value;

  return NULL;
}

/* Inserts a new entry into the hash table. If there is already an entry
   for the given key, replace the value (freeing the old one). */
bool
hash_insert (char *key, char *value)
{
  if (table == NULL) // uninitialized table
    return false;

  return insert_help (key, value, true);
}

/* Gets a list of pointers to the keys in the hash table. */
char **
hash_keys (void)
{
  if (table == NULL) // uninitialized table
    return NULL;

  char **keys = calloc (entries + 1, sizeof (char *));
  size_t next = 0;
  for (size_t i = 0; i < capacity; i++)
    if (table[i].key != NULL && table[i].alive)
      keys[next++] = table[i].key;
  return keys;
}

/* Removes a key-value pair from the hash table. Marks the entry as
   deleted. Can be overwritten later. */
bool
hash_remove (char *key)
{
  if (table == NULL) // uninitialized table
    return false;

  ssize_t index = find_index (key);
  assert (index >= 0);

  if (table[index].key == NULL) // key not found
    return true;

  // If .key is not NULL, find_index found a key match
  assert (!strcmp (table[index].key, key));
  table[index].alive = false;
  entries--;

  if ((entries < capacity / 4) && (capacity / 2 >= MINSIZE))
    rehash (capacity / 2);

  return true;
}

/* **********************************************************************
 *                Helper functions only below this point                *
 * ********************************************************************** */

static bool
insert_help (char *key, char *value, bool dup)
{
  ssize_t index = find_index (key);
  assert (index >= 0); // failed to find an open slot; should never happen

  if (table[index].key == NULL)
    {
      // New entry for this key. Check if rehashing is needed.
      if (entries + 1 > capacity / 2)
        {
          rehash (capacity * 2);
          index = find_index (key);
        }

      // Set the entries in the table (duplicating if requested)
      if (dup)
        {
          table[index].key = strdup (key);
          table[index].value = strdup (value);
        }
      else
        {
          table[index].key = key;
          table[index].value = value;
        }
      entries++;
      table[index].alive = true;

      return true;
    }

  // If table[index].key is not NULL, it must match the existing key OR
  // the existing entry has been deleted. Otherwise, the double hashing
  // should have found a different location. If the key does not match
  // the current key, replace it.
  if (strcmp (table[index].key, key))
    {
      assert (!table[index].alive);
      free (table[index].key);
      if (dup)
        table[index].key = strdup (key);
      else
        table[index].key = key;
    }

  // Free the old value and replace it
  free (table[index].value);
  if (dup)
    table[index].value = strdup (value);
  else
    table[index].value = value;
  table[index].alive = true;

  return true;
}

static ssize_t
find_index (char *str)
{
  if (table == NULL)
    return -1;

  unsigned long keyhash = hash ((unsigned char *)str);
  // probe is 1 .. PRIME, guaranteed non-zero
  unsigned long probe = PRIME - (keyhash % PRIME);

  // Use double hashing to resolve collisions
  size_t index = (size_t)(keyhash % capacity);
  ssize_t first_open = -1;
  for (size_t i = 0; i < capacity; i++)
    {
      size_t trial = (index + (size_t)(i * probe)) % capacity;
      // If the key is NULL, we have not encountered the string. If there
      // was an earlier open spot, use that one. Otherwise, use the spot
      // with the empty key.
      if (table[trial].key == NULL)
        {
          if (first_open < 0)
            return (ssize_t)trial;
          else
            return first_open;
        }

      // Keep track of the first open index. This will be returned if
      // the key has not been previously entered and deleted.
      if (first_open < 0 && !table[trial].alive)
        first_open = trial;

      // If the key has been previously used, re-use this position
      if (!strcmp (table[trial].key, str))
        return (ssize_t)trial;
    }

  // Due to rehashing, there should always be at least 50% of the table
  // entries free. So there should always be a free space.
  abort ();
}

static unsigned long
hash (unsigned char *string)
{
  if (string == NULL)
    return 0;

  unsigned long hash = 5381;

  // Derived from djb2 by Dan Bernstein
  // hash = hash * 33 + ch
  for (unsigned char *ptr = string; *ptr != '\0'; ptr++)
    hash = ((hash << 5) + hash) + *ptr;

  return hash;
}

static void
rehash (size_t newcap)
{
  size_t oldcap = capacity;
  capacity = newcap;
  kvpair_t *oldtable = table;
  table = calloc (capacity, sizeof (kvpair_t));
  entries = 0;

  for (int i = 0; i < oldcap; i++)
    {
      if (oldtable[i].key != NULL && oldtable[i].alive)
        insert_help (oldtable[i].key, oldtable[i].value, false);
    }
}

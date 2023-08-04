#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"


static cache_entry_t *cache = NULL;
static int cache_size = 0;
//static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cacheEnabled = 0;
int fifoIndex = 0;
int global_var = 0;

//This function creates the cache
int cache_create(int num_entries) {

//a cache will be created only if global_var is zero to ensure that the cache is only created once
//and the cache entries should be between 2 and 4096
  while (global_var == 0 && num_entries >= 2 && num_entries <= 4096)
  {
    //allocating a space for the cache
    cache = calloc(num_entries, sizeof(cache_entry_t));
    if (cache != NULL) //if calloc is successful
    {
      global_var = 1;
      cache_size = num_entries;
      return 1;
    }
    return 0;
  }
  return -1; //if condition for while loop is not met, we will get an error

  
}

//this function destroys the cache, that we created in the function above
int cache_destroy(void) {
  //check if cache is already destroyed
  if (global_var == false)
  {
    return -1;
  }
  //else we resize to zero, making the cache empty
  cache_size = 0;
  global_var = false; //indicates cache is inactive
  free(cache); //successfully destroying cache
  return 1;
  
}

//to find cache entry
int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
int i;
while (global_var == 0 || buf == NULL) //checking if cache was initiliazed
{
  return -1;
}

for (i = 0; i < cache_size; i++) //traversing through all the entries
{
  if (cache[i].valid == true)
  {
    if (cache[i].block_num == block_num && cache[i].disk_num == disk_num)
    {
      num_hits++;
      num_queries += 1;
      cacheEnabled++;
      memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
      return 1;
      }
  }
  }
  return -1;
}

//to update existing cache entry
void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  //traversing
  for (int j = 0; j < cache_size; j++) // will traverse through the cache
  {
    //checking if disk and blk number match
    if (cache[j].block_num == block_num && cache[j].disk_num == disk_num)
    {
      num_hits += 1;
      cacheEnabled += 1;
      memcpy(cache[j].block, buf, JBOD_BLOCK_SIZE);
    }
  }
 
}

//to insert new entry in cache
int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  int i;
  //int fifoIndex = 0;
  //verifiying if cache is initialized, and if disk and blk number is in range
  while (global_var == 0 || buf == NULL)
  {
    return -1;
  }
  while (disk_num < 0 || disk_num > 16)
  {
    return -1;
  }
  while (block_num < 0 || block_num > 256)
  {
    return -1;
  }

  //traversing
  for (i = 0; i <= cache_size; i++)
  {
    if (cache[i].valid == true && cache[i].disk_num == disk_num && cache[i].block_num == block_num)
    {
      cacheEnabled += 1;
      cache[i].block_num = block_num;
      cache[i].disk_num = disk_num;
      cache[i].valid = true;
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      return 1;
    }
    else
    {
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      return -1;
    }
  }

  cacheEnabled++;
  cache[fifoIndex].block_num = block_num;
  cache[fifoIndex].disk_num = disk_num;
  memcpy(cache[fifoIndex].block, buf, JBOD_BLOCK_SIZE);
  return 1;

}

bool cache_enabled(void) {
   if (cache_size == 0)
  {
    return false;
  }
  else
  {
    return true;
  }
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
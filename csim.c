//#define _XOPEN_SOURCE
/*
 * Reads a Valgrind trace and simulates an LRU cache, 
 * counting hits, misses, and evictions
 * 
 * Grace Hunter
 * Wheaton College
 * CSCI 351 - Intro to Computer Systems
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "cachelab.h"

/* struct to represent one cache line */
struct line {
  int valid;
  int age; //accesses to this set since this block was last accessed
  int tag;
};

/* struct to represent one cache set */
struct set {
  struct line **lines;
};

/* struct to represent a cache */
struct cache {
  int S, E, B;
  struct set **sets;
  char* trace; //path to trace file
};

void access_block(struct cache *c, unsigned int addr, int *hme);
struct cache *construct_cache(char argc, char **argv);
void destroy_cache(struct cache *c);
  
int main(int argc, char **argv){
  /* construct cache */
  struct cache *c = construct_cache(argc, argv);

  /* hits misses evictions */
  int *hme = malloc(sizeof(int) * 3);
  hme[0] = 0;
  hme[1] = 0;
  hme[2] = 0;

  /* read and simulate trace file */
  FILE *f = fopen(c->trace, "r");
  char i;
  unsigned int addr;
  int size;
  int err;

  while(!feof(f)){
    err = fscanf(f, " %c %x,%d", &i, &addr, &size);
    if(err == 3){
      switch(i){
      case 'L':
	access_block(c, addr, hme);
	break;
      case 'S':
	access_block(c, addr, hme);
	break;
      case 'M':
	access_block(c, addr, hme);
	access_block(c, addr, hme);
	break;
      }
    }
  }
  fclose(f);

  printSummary(hme[0], hme[1], hme[2]);
  
  /* free memory */
  free(hme);
  destroy_cache(c);
  return 0;
}

/* access one memory address using cache */
void access_block(struct cache *c, unsigned int addr, int* hme){
  //get set
  int block_no = addr / c->B;
  int set_no = block_no % c->S;
  int tag = block_no / c->S;
  struct set *s = c->sets[set_no];
  
  //tag matching
  int i;
  int found = 0;
  for(i = 0; i < c->E; i++){
    s->lines[i]->age++;
    if(s->lines[i]->valid && s->lines[i]->tag == tag){
      hme[0]++; //hit
      s->lines[i]->age = 0;
      found = 1;
    }
  }

  //doesn't exist? store block
  if(!found){
    //find invalid block
    int invalid_found = 0;
    for(i = 0; i < c->E; i++){
      if(!s->lines[i]->valid){
	invalid_found = 1;
	s->lines[i]->valid = 1;
	s->lines[i]->tag = tag;
	s->lines[i]->age = 0;
	hme[1]++;
	return;
      }
    }

    //no invalid blocks: evict
    if(!invalid_found){
      struct line *toevict = s->lines[0];
      for(i = 0; i < c->E; i++){
	if(toevict->age < s->lines[i]->age){
	  toevict = s->lines[i];
	}
      }
      toevict->tag = tag;
      toevict->age = 0;
      hme[1]++;
      hme[2]++;
    }
  }
}

struct cache *construct_cache(char argc, char **argv){
  //create cache
  struct cache *c = malloc(sizeof(struct cache));
  
  //get options from argv
  int opt;
  while((opt = getopt(argc, argv, ":hvs:E:b:t:")) != -1){
    switch(opt){
    case 'h':
      printf("helping you...\n");
      break;
    case 'v':
      //  v = 1;
      break;
    case 's':
      c->S = 1 << atoi(optarg);
      break;
    case 'E':
      c->E = atoi(optarg);
      break;
    case 'b':
      c->B = 1 << atoi(optarg);
      break;
    case 't':
      c->trace = optarg;
      break;
    case ':':
      break;
    case '?':
      break;
    }
  }
  //populate cache with sets
  c->sets = malloc(sizeof(struct set *) * c->S);
  int i;
  //populate sets with lines
  for(i = 0; i < c->S; i++){
    c->sets[i] = malloc(sizeof(struct set));
    c->sets[i]->lines = malloc(sizeof(struct line *) * c->E);
    
    //initialize lines
    int j;
    for(j = 0; j < c->E; j++){
      c->sets[i]->lines[j] = malloc(sizeof(struct line));
      c->sets[i]->lines[j]->age = 0;
      c->sets[i]->lines[j]->valid = 0;
    }
  }

  return c;
}

/* free all memory associated with a cache */
void destroy_cache(struct cache *c){

  //free each set
  int i;
  for(i = 0; i < c->S; i++){

    //free each line
    int j;
    for(j = 0; j < c->E; j++){
      free(c->sets[i]->lines[j]);
    }

    free(c->sets[i]->lines);
    free(c->sets[i]);
  }

  //free cache
  free(c->sets);
  free(c);
}

#include <stdio.h>
#include <cmath>
#include <stdlib.h>
#include<limits>
FILE *trace;

struct list{
 unsigned int dist;
  int read_write;
  list *next;
};

struct node{
 unsigned int tag;
  bool dirty;
  list *next;
};

struct cache{
  bool dirty;
  unsigned int tag;
};

node table[1000000];

cache l1[64][8],l2[512][8],l3[2048][16];


int l1_misses,l2_misses,l3_misses,l1_hits,l2_hits,l3_hits;

void print()
{
  int i;
  node *tmp;
  tmp=table;
  for(i=0;i<100000;i++)
  {
    if(tmp[i].dirty != false)
    {
      printf("%d \t\t",tmp[i].tag);
      list *head;
      head = tmp[i].next;
      while (head !=NULL)
	{
	  printf("%d \t\t",head->dist);
	  head= head->next;
	}
      printf("\n");
    }
  }
}

int hash( unsigned int addr)
{
  node *tmp;
  tmp = table;
  unsigned int i=addr;
  while(1)
  {
    if(tmp[i].dirty == false or tmp[i].tag == addr) 
      return i;
    else if(i==999999)
      i=0;
    else
      i++;
  }
}


void update(node *head,int change,int rw)
{
  list *tmp = head->next;
  //  printf("%d \n",tmp->dist);
  list *tmp1;
  while(tmp->next != NULL)
    tmp = tmp->next;
  tmp->dist = change - tmp->dist;
  tmp1 = (list*) malloc(sizeof(list));
  tmp1->read_write = rw;
  tmp1->dist= change;
  tmp->next = tmp1;
  tmp1->next=NULL;
  //print();
}

void print_cache()
{
  int i=0,j=0;
  printf("L1 cache content are : \n");
  for(i=0;i<64;i++)
    {
      for(j=0;j<8;j++)
	printf("%d\t %d\t",l1[i][j].dirty,l1[i][j].tag);
      printf("\n");
    }
  
  printf("L2 cache content are : \n");

  for(i=0;i<512;i++)
    {
      for(j=0;j<8;j++)
	{
	  // if(l2[i][j].dirty != false){
	    printf("%d\t\t %d",l2[i][j].dirty,l2[i][j].tag);
	    printf("\n");//}
	}
    }
}


void initialize ()
{ 
  int i=0,j=0;
  node *tmp;
  tmp = table;
  l1_misses = 0;
  l1_hits = 0;
  l2_misses = 0;
  l2_hits = 0;

  l3_misses = 0;
  l3_hits = 0;
  while(j<1000000)
    {
      tmp->tag = 0;
      tmp->dirty = false;
      tmp->next = NULL;
      tmp++;
      j++;
    }
  for(i=0;i<64;i++)
    for(j=0;j<8;j++)
      {
	l1[i][j].tag=0;
	l1[i][j].dirty = false;
	l2[i][j].tag=0;
	l2[i][j].dirty = false;
      }
  for(i=64;i<512;i++)
    for(j=0;j<8;j++)
      {
	l2[i][j].tag=0;
	l2[i][j].dirty = false;
      }
  for(i=0;i<2048;i++)
    for(j=0;j<16;j++)
      {
	l3[i][j].tag=0;
	l3[i][j].dirty= false;
      }
	
}
int check_l3(cache cach[][16],int index, unsigned int tag)
{
  int i=0;
  for(i=0;i<16;i++)
    {
      if(cach[index][i].dirty == false)
	{
	  cach[index][i].dirty = true;
	  cach[index][i].tag = tag;
	  return 0;
	}
      else if(cach[index][i].tag == tag)
	{
	  return 1;
	}
    }
  return i;
}

int check_l12(cache cach[][8],int index, unsigned int tag)
{
  int i=0;
  for(i=0;i<8;i++)
    {
      if(cach[index][i].dirty == false)
	{
	  cach[index][i].dirty = true;
	  cach[index][i].tag = tag;
	  return 0;
	}
      else if(cach[index][i].tag == tag)
	{
	  return 1;
	}
    }
  return i;
}

int search(int temp)
{
  int temp_in,i=0;
  temp_in= temp;

  node *dist_rem;
  while(1)
  {
    if(table[temp_in].tag == temp)
    {
      i=table[temp_in].next->dist;
      dist_rem = table;
      dist_rem = dist_rem + temp_in;
      if(dist_rem->next != NULL)
	dist_rem->next=dist_rem->next->next;
      return i;
    }
    else
    {	
      if(temp_in == 1000000)
	temp_in=0;
      else
	temp_in++;
    }
  }
}
int evict_block_l1_l2(cache cach[][8],int index, int dist_evict,int *remove)
{
  int i=0;
  int temp = 0,temp_in=0;
  for(i=0;i<8;i++)
  {
    temp=(cach[index][i].tag)%1000000;
    temp_in = temp;
    while(1)
    {
      if(table[temp_in].tag == temp)
	{
	  if(table[temp_in].next != NULL and  dist_evict > table[temp_in].next->dist)
	    {
	      dist_evict = table[temp_in].next->dist;
	      *remove = i;
	    }
	  else if (table[temp_in].next == NULL )
	    {
	      dist_evict = std::numeric_limits<int>::max();
	      return dist_evict;
	    }
	  break;
	}
      else
	{
	  if(temp_in == 1000000)
	    temp_in =0;
	  else
	    temp_in++;
	}
    }
  }
  return dist_evict;
}
int evict_block_l3(cache cach[][16],int index, int dist_evict,int *remove)
{
  int i=0;
  int temp = 0,temp_in=0;
  for(i=0;i<16;i++)
  {
    temp=(cach[index][i].tag)%1000000;
    temp_in = temp;
    while(1)
    {
      if(table[temp_in].tag == temp)
	{
	  if(table[temp_in].next != NULL and  dist_evict > table[temp_in].next->dist)
	    {
	      dist_evict = table[temp_in].next->dist;
	      *remove = i;
	    }
	  else if (table[temp_in].next == NULL )
	    {
	      dist_evict = std::numeric_limits<int>::max();
	      return dist_evict;
	    }
	  break;
	}
      else
	{
	  if(temp_in == 1000000)
	    temp_in =0;
	  else
	    temp_in++;
	}
    }
  }
  return dist_evict;
}

void cache_calculator()
{
  unsigned int addr=0,temp=0,tag=0,result_l1 =0 ;
  trace = fopen("memtrace.out","r");
  
  // knowing optimal misses
  while(1)
  {
    if(feof(trace))
      break;
    else
    {
      fread(&addr,4,1,trace);
      //      printf("\t %u\n",addr);
      tag = (addr<<1)>>1;

      tag = tag/64;
      // printf("%u\n",tag);      
      int index= tag%64;

      result_l1 = check_l12(l1,index,tag);

      if(result_l1 == 0)
	l1_misses++;
      else if(result_l1 == 1)
	l1_hits++;
      else
	{
	  l1_misses++;
	  temp = tag%1000000;
	  int temp_in=0, remove=0;
	  int dist_evict=0,dist_in=0;
	  dist_in = search(temp);
	  
	  dist_evict = evict_block_l1_l2(l1,index,dist_in,&remove);

	  if(dist_evict != dist_in)
	    {
	      unsigned int t = l1[index][remove].tag;
	      l1[index][remove].tag = tag;
	      tag = t;
	      //dist_rem->next = dist_rem->next->next;
	    }
	  dist_in = dist_evict;
	  index=tag%512;

	  result_l1 = check_l12(l2,index,tag);
	  if(result_l1 == 0)
	    l2_misses++;
	  else if(result_l1 == 1)
	    l2_hits++;
	  else 
	    {
	      l2_misses++;
	      //  temp = tag%1000000;
	      int temp_in=0, remove=0;
	      dist_evict = evict_block_l1_l2(l2,index,dist_in,&remove);
	      if(dist_evict != dist_in)
	      {
		unsigned int t = l2[index][remove].tag;
		l2[index][remove].tag = tag;
		tag = t;
	      }
	      dist_in = dist_evict;
	      index = tag%2048;

	      result_l1 = check_l3(l3,index,tag);
	      if(result_l1 == 0)
		l3_misses++;
	      else if(result_l1 == 1)
		l3_hits++;
	      else 
		{
		  l3_misses++;
		  //  temp = tag%1000000;
		  int temp_in=0, remove=0;
		  dist_evict = evict_block_l3(l3,index,dist_in,&remove);
		  if(dist_evict != dist_in)
		    {
		      unsigned int t = l3[index][remove].tag;
		      l3[index][remove].tag = tag;
		      tag = t;
		    }
		}
	    }
	}
    }
  }
}
int main(int argc, char *argv[])
{
  trace =  fopen("memtrace.out","r");
  unsigned int addr=0,tag=0,temp=0;
  node *tmp;
  int i=0,j=0;
  initialize();
  temp = pow(2,31);
  //  tmp=table;
  while(1)
  {
    //
    if(feof(trace))
      break;
    else
    {
      i++;
      fread(&addr,4,1,trace);
      //  printf("%u \n",addr);
      unsigned int read_write = 0;
      if((addr - ((addr << 1)>>1)) == temp)
	read_write = 1;
      else
	read_write = 0;
      tag = (addr <<1)>>1;
      tag = (tag/64)%1000000;
      int index =0;
      index = hash(tag);

      if(table[index].dirty == false)
      {
	//	printf("tag is %u tmp is \n",tag);
	table[index].tag = tag;
	table[index].dirty=true;
	list *head;
	head = (list*)malloc(sizeof(list));
	head->dist = i;
	head->read_write = read_write;
	head->next = NULL;
	table[index].next = head;

      }
      else
	{
	  update(table+index,i,read_write);
	  //printf("hello\n");
	  //	print();
	}
    
    }
    print();
  }

  //  print_cache();
  cache_calculator();
  print_cache();
}

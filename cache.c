#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct cacheBlock cacheBlock;
struct cacheBlock{
unsigned long long tag;
int valid;
cacheBlock* next;
cacheBlock* prev;
};

typedef struct cacheLine cacheLine;
struct cacheLine{
cacheBlock* head;
cacheBlock* tail;
};

cacheLine* cache;
cacheLine* pre;
int cacheSize;
int blockSize;
int numLines;
int numBlocks;

char* toBinary(char* address)
{
	char* binaryAddress = malloc(sizeof(char)*((strlen(address)*4)+1));
	for(int i =2;/*Skip 0x */ i < strlen(address); i++)
	{
		switch(address[i])
		{
			case '0': strcat(binaryAddress, "0000"); break;
			case '1': strcat(binaryAddress, "0001"); break;
			case '2': strcat(binaryAddress, "0010"); break;
			case '3': strcat(binaryAddress, "0011"); break;
			case '4': strcat(binaryAddress, "0100"); break;
			case '5': strcat(binaryAddress, "0101"); break;
			case '6': strcat(binaryAddress, "0110"); break;
			case '7': strcat(binaryAddress, "0111"); break;
			case '8': strcat(binaryAddress, "1000"); break;
			case '9': strcat(binaryAddress, "1001"); break;
			case 'a': strcat(binaryAddress, "1010"); break;
			case 'b': strcat(binaryAddress, "1011"); break;
			case 'c': strcat(binaryAddress, "1100"); break;
			case 'd': strcat(binaryAddress, "1101"); break;
			case 'e': strcat(binaryAddress, "1110"); break;
			case 'f': strcat(binaryAddress, "1111"); break;
		}
	}
	binaryAddress[(strlen(address))*4] = '\0';
	return binaryAddress;
}

unsigned long long toDecimal(char* address)
{
	unsigned long long sum = 0;
	int power = 0;
	for(int i = strlen(address)-1; i >= 0; i--)
	{
		if(address[i] == '1')
		{
			sum += pow(2, power);
			power++;
		}
		else
		{
			power++;
		}
	}
	return sum;
}

char* prefetchBin(char* address)
{
	char* binaryAddress = malloc(sizeof(char)*(strlen(address)+1));
	strcpy(binaryAddress, address);
	int mod = (log(blockSize)/log(2))+1;
	int pos = strlen(address)-mod;
	if(binaryAddress[pos] == '0')
	{
		binaryAddress[pos] = '1';
	}
	else
	{
		while(binaryAddress[pos] == '1')
		{
			binaryAddress[pos] = '0';
			pos--;
		}
		binaryAddress[pos] = '1';
	}
	
	binaryAddress[strlen(address)] = '\0';
	return binaryAddress;
}

char* addressIndex(char* address)
{	
	int indexSize = log(numLines)/log(2);
	int offsetSize = log(blockSize)/log(2);
	char* index = malloc(sizeof(char)*(indexSize+1));
	int counter = 0;
	for(int i = strlen(address)-(indexSize+offsetSize); i < (strlen(address)-offsetSize); i++)
	{
		index[counter] = address[i];
		counter++;
	}
	index[counter] = '\0';
	return index;	
}

char* addressTag(char* address)
{
	int tagSize = strlen(address)-((log(numLines)/log(2)) + (log(blockSize)/log(2)));
	char* tag = malloc(sizeof(char)*(tagSize+1));
	for(int i = 0; i < tagSize; i++)
	{
		tag[i] = address[i];
	}
	tag[tagSize] = '\0';
	return tag;
}

/*char* addressOffset(char* address)
{
	int offsetSize = log(blockSize)/log(2);
	char* offset = malloc(sizeof(char)*(offsetSize+1));
	int counter = 0;
	for(int i = (strlen(address)-offsetSize); i < strlen(address); i++)
	{
		offset[counter] = address[i];
		counter++;
	}
	offset[counter] = '\0';
	return offset;
}*/

void maintainLRU(cacheBlock* ptr, int index, int whichCache)//applies lru to current node
{
	if(whichCache == 0)
	{
		if(ptr->next == NULL)//Tail
		{
			ptr->next = cache[index].head;
			ptr->prev->next = NULL;
			cache[index].tail = ptr->prev;
			ptr->next->prev = ptr;
			cache[index].head = ptr;
			ptr->prev = NULL;
		}
		else
		{	
			ptr->prev->next = ptr->next;
			ptr->next->prev = ptr->prev;
			ptr->next = cache[index].head;
			ptr->next->prev = ptr;
			cache[index].head = ptr;
			ptr->prev = NULL;
		
		}
	}
	if(whichCache == 1)
	{
		if(ptr->next == NULL)//Tail
		{
			ptr->next = pre[index].head;
			ptr->prev->next = NULL;
			pre[index].tail = ptr->prev;
			ptr->next->prev = ptr;
			pre[index].head = ptr;
			ptr->prev = NULL;
		}
		else
		{	
			ptr->prev->next = ptr->next;
			ptr->next->prev = ptr->prev;
			ptr->next = pre[index].head;
			ptr->next->prev = ptr;
			pre[index].head = ptr;
			ptr->prev = NULL;
		
		}
	}
}

int main(int argc, char* argv[])
{
	int times = 0;
	char* filename = argv[5];
	FILE* file = fopen(filename, "r");
	cacheSize = atoi(argv[1]); // Must be power of 2
	blockSize = atoi(argv[4]);//Power of 2
	char* assoc = argv[2];
	int assocType = -1; //0 = Direct, 1 = Full Assoc, x = xWay Assoc Must be power of 2
	char nWay[(strlen(assoc))-5];
	if(strcmp(assoc, "direct") == 0)
	{
		assocType = 0;
	}
	else if(strcmp(assoc, "assoc") == 0)
	{
		assocType = 1;
	}
	else
	{
		int counter = 0;
		for(int i = 6; i < strlen(assoc); i++)
		{	
			nWay[counter] = assoc[i];
			counter++;
		}
		nWay[counter] = '\0';
		assocType = atoi(nWay);
		if((cacheSize/(blockSize*assocType) == 1))
		{
			assocType = 1;
		}
	}
	//policy LRU regardless
	if(assocType == 0)
	{
		numLines = cacheSize/blockSize;
		numBlocks = 1;

		cache = malloc(sizeof(cacheLine)*numLines);
		for(int i = 0; i < numLines; i++)
		{
			cacheBlock* temp = malloc(sizeof(cacheBlock));
			temp->valid = 0;
			temp->next = NULL;
			temp->prev = NULL;
			temp->tag = 0;
			cache[i].head = temp;
		}

		pre = malloc(sizeof(cacheLine)*numLines);
		for(int i = 0; i < numLines; i++)
		{
			cacheBlock* temp1 = malloc(sizeof(cacheBlock));
			temp1->valid = 0;
			temp1->next = NULL;
			temp1->prev = NULL;
			temp1->tag = 0;
			pre[i].head = temp1;
		}
	}
	else if(assocType == 1)
	{
		numLines = 1;
		numBlocks = cacheSize/blockSize;

		cache = malloc(sizeof(cacheLine));
		cacheBlock* temp = malloc(sizeof(cacheBlock));
		temp->valid = 0;
		temp->prev = NULL;
		temp->tag = 0;
		cache[0].head = temp;
		for(int j = 1; j < numBlocks; j++)
		{
			temp->next = malloc(sizeof(cacheBlock));
			temp->next->prev = temp;
			temp->next->valid = 0;
			temp->next->tag = 0;
			temp = temp->next;
		}
		temp->next = NULL;
		cache[0].tail = temp;

		pre = malloc(sizeof(cacheLine));
		cacheBlock* temp1 = malloc(sizeof(cacheBlock));
		temp1->valid = 0;
		temp1->prev = NULL;
		temp1->tag = 0;
		pre[0].head = temp1;
		for(int j = 1; j < numBlocks; j++)
		{
			temp1->next = malloc(sizeof(cacheBlock));
			temp1->next->prev = temp1;
			temp1->next->valid = 0;
			temp1->next->tag = 0;
			temp1 = temp1->next;
		}
		temp1->next = NULL;
		pre[0].tail = temp1;
	}
	else
	{
		numLines = cacheSize/(blockSize*assocType);
		numBlocks = assocType;

		cache = malloc(sizeof(cacheLine)*numLines);
		for(int i = 0; i < numLines; i++)
		{
			cacheBlock* temp = malloc(sizeof(cacheBlock));
			temp->valid = 0;
			temp->prev = NULL;
			temp->tag = 0;
			cache[i].head = temp;
			for(int j = 1; j < numBlocks; j++)
			{
				temp->next = malloc(sizeof(cacheBlock));
				temp->next->prev = temp;
				temp->next->valid = 0;
				temp->next->tag = 0;
				temp = temp->next;
			}
			temp->next = NULL;
			cache[i].tail = temp;
		}

		pre = malloc(sizeof(cacheLine)*numLines);
		for(int i = 0; i < numLines; i++)
		{
			cacheBlock* temp1 = malloc(sizeof(cacheBlock));
			temp1->valid = 0;
			temp1->prev = NULL;
			temp1->tag = 0;
			pre[i].head = temp1;
			for(int j = 1; j < numBlocks; j++)
			{
				temp1->next = malloc(sizeof(cacheBlock));
				temp1->next->prev = temp1;
				temp1->next->valid = 0;
				temp1->next->tag = 0;
				temp1 = temp1->next;
			}
			temp1->next = NULL;
			pre[i].tail = temp1;
		}
	}

	char ignore[20];
	char operation = 'z';
	char address[30];
	int reads = 0;
	int writes = 0;
	int hits = 0;
	int misses = 0;
	int readsA = 0;
	int writesA = 0;
	int hitsA = 0;
	int missesA = 0;
	cacheBlock* ptr;
	cacheBlock* prePtr;
	while(!feof(file))
	{
		fscanf(file, "%s " "%c " "%s\n", ignore, &operation, address);
		ignore[19] = '\0';
		address[19] = '\0';
		if(strcmp(ignore, "#eof") == 0)
		{
			break;
		}
		times++;
		if(operation == 'W')
		{
			writes++;
			writesA++;
		}
		char* binaryAddress = toBinary(address); //Ask: do i free?
		char* index = addressIndex(binaryAddress);
		char* tag = addressTag(binaryAddress);
		int decIndex = toDecimal(index);
		unsigned long long decTag = toDecimal(tag);
		char* preA = prefetchBin(binaryAddress);
		char* preI = addressIndex(preA);
		char* preT = addressTag(preA);
		int preIndex = toDecimal(preI);
		unsigned long long preTag = toDecimal(preT);
		if(assocType == 0)//DIRECT
		{
			ptr = cache[decIndex].head;
			if(ptr->valid == 1)
			{
				if(ptr->tag == decTag)
				{
					hits++;
				}
				else
				{
					reads++;
					misses++;
					ptr->tag = decTag;
					ptr->valid = 1;
				}
			}
			else
			{
				reads++;
				misses++;
				ptr->tag = decTag;
				ptr->valid = 1;
			}
		
			prePtr = pre[decIndex].head;
			if(prePtr->valid == 1)
			{
				if(prePtr->tag == decTag)
				{
					hitsA++;
				}
				else
				{
					readsA++;
					missesA++; 
					prePtr->tag = decTag;
					prePtr->valid = 1;
					if(preIndex < numLines)
					{
						if(pre[preIndex].head->tag != preTag)
						{
							readsA++;
							pre[preIndex].head->tag = preTag;
							pre[preIndex].head->valid = 1;
						}
					}
				}
			}
			else
			{
				readsA++;
				missesA++;
				prePtr->tag = decTag;
				prePtr->valid = 1;
				if(preIndex < numLines)
				{
					if(pre[preIndex].head->tag != preTag)
					{
						readsA++;
						pre[preIndex].head->tag = preTag;
						pre[preIndex].head->valid = 1;
					}	
				}
			}
		}
		else if(assocType == 1)
		{
			ptr = cache[0].head;
			for(int i = 0; i < numBlocks; i++)
			{
				if(ptr->valid == 0)
				{
					reads++;
					misses++;
					ptr->tag = decTag;
					ptr->valid = 1;
					if(ptr != cache[0].head)
					{
						maintainLRU(ptr, 0, 0);
					}
					break;
				}
				else
				{
					if(ptr->tag == decTag)
					{
						hits++;
						if(ptr != cache[0].head)
						{
							maintainLRU(ptr, 0, 0);
						}
						break;
					}
					else
					{
						if(ptr->next == NULL)//at tail and miss
						{
							reads++;
							misses++;
							ptr->tag = decTag;
							ptr->valid = 1;
							maintainLRU(ptr, 0, 0);
							break;
						}
						else
						{
							ptr = ptr->next;
						}
					}
				}
			}

			prePtr = pre[0].head;
			cacheBlock* temp = NULL;
			for(int i = 0; i < numBlocks; i++)
			{
				if(prePtr->tag == preTag)//Used to later see if the prefetched tag exists
				{
					temp = prePtr;
				}
				if(prePtr->valid == 0)
				{
					readsA++;
					missesA++;
					prePtr->tag = decTag;
					prePtr->valid = 1;
					if(prePtr != pre[0].head)
					{
						maintainLRU(prePtr, 0, 1);
					}
					if(temp == NULL)//prefetched tag dne
					{
						readsA++;
						pre[0].tail->tag = preTag;
						pre[0].tail->valid = 1;
						cacheBlock* ptr2 = pre[0].tail;
						maintainLRU(ptr2, 0, 1);
					}
					break;
				}
				else
				{
					if(prePtr->tag == decTag)
					{
						hitsA++;
						if(prePtr != pre[0].head)
						{
							maintainLRU(prePtr, 0, 1);
						}
						break;
					}
					else
					{
						if(prePtr->next == NULL)//at tail and miss
						{
							readsA++;
							missesA++;
							prePtr->tag = decTag;
							prePtr->valid = 1;
							maintainLRU(prePtr, 0, 1);
							if(temp == NULL)//prefetched tag dne
							{
								readsA++;
								pre[0].tail->tag = preTag;
								pre[0].tail->valid = 1;
								cacheBlock* ptr2 = pre[0].tail;
								maintainLRU(ptr2, 0, 1);
							}
							break;
						}
						else
						{
							prePtr = prePtr->next;
						}
					}
				}
			}
		}
		else
		{
			ptr = cache[decIndex].head;
			for(int i = 0; i < numBlocks; i++)
			{
				if(ptr->valid == 0)
				{		
					reads++;
					misses++;
					ptr->tag = decTag;
					ptr->valid = 1;
					if(ptr != cache[decIndex].head)
					{
						maintainLRU(ptr, decIndex, 0);
						break;
					}
					break;
				}
				else
				{
					if(ptr->tag == decTag)
					{
						hits++;
						if(ptr != cache[decIndex].head)
						{
							maintainLRU(ptr, decIndex, 0); break;
						}
						break;
					}
					else
					{
						if(ptr->next == NULL)//at tail and miss
						{		
							reads++;
							misses++; 
							ptr->tag = decTag;
							ptr->valid = 1;
							maintainLRU(ptr, decIndex, 0);
							break;
						}
						else
						{
							ptr = ptr->next;
						}
					}
				}
			}
			
			prePtr = pre[decIndex].head;
			for(int i = 0; i < numBlocks; i++)
			{
				if(prePtr->valid == 0)
				{
					readsA++;
					missesA++;
					prePtr->tag = decTag;
					prePtr->valid = 1;
					if(prePtr != pre[decIndex].head)
					{
						maintainLRU(prePtr, decIndex, 1);
					}
					if(preIndex < numLines)
					{
						cacheBlock* temp = pre[preIndex].head; //prefetch check
						while(temp != NULL)
						{
							if(temp->tag == preTag)//exists and makes temp point to preexisting fetched tag
							{
								break;
							}
							temp = temp->next;
						}
						if(temp == NULL)//prefetch dne
						{
							readsA++;
							pre[preIndex].tail->tag = preTag;
							pre[preIndex].tail->valid = 1;
							cacheBlock* ptr2 = pre[preIndex].tail;
							maintainLRU(ptr2, preIndex, 1);
						}
					}
					break;
				}
				else
				{
					if(prePtr->tag == decTag)
					{
						hitsA++;
						if(prePtr != pre[decIndex].head)
						{
							maintainLRU(prePtr, decIndex, 1);
						}
						break;
					}
					else
					{
						if(prePtr->next == NULL)//at tail and miss
						{
							readsA++;
							missesA++;
							prePtr->tag = decTag;
							prePtr->valid = 1;
							maintainLRU(prePtr, decIndex, 1);
							if(preIndex < numLines)
							{
								cacheBlock* temp = pre[preIndex].head; //prefetch check
								while(temp != NULL)
								{
									if(temp->tag == preTag)//exists and makes temp point to preexisting fetched tag
									{
										break;
									}
									temp = temp->next;
								}
								if(temp == NULL)//prefetch dne
								{
									readsA++;
									pre[preIndex].tail->tag = preTag;
									pre[preIndex].tail->valid = 1;
									cacheBlock* ptr2 = pre[preIndex].tail;
									maintainLRU(ptr2, preIndex, 1);
								}
							}
							break;
						}
						else
						{
							prePtr = prePtr->next;
						}
					}
				}
			}
		}
	}
	printf("no-prefetch\nMemory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\nwith-prefetch\nMemory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n", reads, writes, hits, misses, readsA, writesA, hitsA, missesA);
}

#ifndef CACHE_H_

#define CACHE_H_



#include <iostream>

#include <fstream>

#include <bitset>

#include <cmath>

#include <vector>

#include "cache.h"

#include <unordered_map>



using namespace std;



class CacheBlock {

public:

    int tag; //block number

	bool valid;

	bool dirty = false;

	int lru = 1;

	CacheBlock(int num) {

		tag = num;

	}

};



class CacheSet { //	WAY

private:

	int maxSize;

public:

    unordered_map<int, CacheBlock> blocks;

	CacheSet(int setSize, int assoc) {

		this->maxSize = pow(2, setSize);
	}

};



class Cache {

private:

    int setSize;                // number of sets in the cache

    int assoc;                  // associativity of the cache (WAYs)

    int blockSize;              // block size in bytes

    int hitLatency;             // hit latency in cycles

    int missLatency;            // miss latency in cycles

    int wrAlloc;         		// write allocate policy

    Cache* lowerCache;          // pointer to the lower-level cache (or nullptr for L2 cache)

    vector<CacheSet> sets;      // vector of cache sets

    int tagSize;				// number of tags in the cache

    int maxLruSize;				// number of LRU values



public:
	int hitCount = 0;
	int totalLatency = 0;
	int missCount = 0;

    Cache(int setSize, int assoc, int blockSize, int hitLatency, int missLatency, int writeAllocate, Cache* lowerCache);

	~Cache();

	bool read(unsigned long int address);
	
	bool write(unsigned long int address);
	
	void writeBack(unsigned long int address, uint32_t setIndex, uint32_t tagIndex);

	void updateHitRate(bool hit);

	void updateLruPolicy(int setIndex, int counterBlocksInLine, CacheBlock presentBlock);

	void missHandler(uint32_t setIndex, uint32_t tagIndex, int counterBlocksInLine, unsigned long int address);

	void printStats();
};



#endif  // CACHE_H_

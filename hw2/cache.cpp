/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

/* Made by On Fabian and Muli Klien */



#include <iostream>
#include <cstdlib>
#include <fstream>

#include <cmath>

#include <vector>

#include "cache.h"

#include <unordered_map>



using namespace std;



Cache::Cache(int cacheSize, int setSize, int assoc, int blockSize, int hitLatency, int missLatency, int writeAllocate, Cache* lowerCache) {

	this->setSize = setSize; // # log2 lines in a Way

	this->assoc = assoc;

	this->blockSize = blockSize;

	this->hitLatency = hitLatency;

	this->missLatency = missLatency;

	this->wrAlloc = writeAllocate;

	this->lowerCache = lowerCache;

	this->tagSize = 32 - (blockSize + setSize);

	this->sets = vector<CacheSet>(pow(2, assoc), CacheSet(setSize, assoc));

	this->maxLruSize = pow(2, assoc);
	this->maxCacheSize = pow(2, cacheSize);
}

Cache::~Cache() {};

bool Cache::read(unsigned long int address) {

	// compute the set index and tag for the given address

	int maskSet = (1 << setSize) - 1;

	int maskTag = (1 << tagSize) - 1;

	uint32_t setIndex = address >> blockSize;

	setIndex &= maskSet;
	setIndexCache = setIndex;
	uint32_t tagIndex = address >> (blockSize + setSize);

	tagIndex &= maskTag;

	// #blocks in line

	int counterBlocksInLine = 0;

	int i = 0;
	// Search all ways for tag
	for (i = 0; i< pow(2, assoc); i++) {

		auto it = sets[i].blocks.find(setIndex);

		// check if the key was found

		if (it != sets[i].blocks.end()) {

			counterBlocksInLine++;

		}

	}
	bool hit = false;

	// checks for read miss/hit and update LRU policy

	for (i = 0; i < pow(2, assoc); i++){

		auto it = sets[i].blocks.find(setIndex);


		// check if the key was found

		if (it != sets[i].blocks.end()) {

			// check if the val was found
			if (it->second.tag == tagIndex) {

				// read hit! 
				hit = true;

				if (it->second.lru == maxLruSize) {

					// this's MRU

					break;

				}

				updateLruPolicy(setIndex, counterBlocksInLine, &it->second);

				break;

			}

		}

	}      
	/*
 
	l1 -> hit -> bye 

	l1 -> miss -> l2:

	l2 hit -> write block to l1 -> bye

	l2 miss -> write block to l2 and l1 -> bye

	*/

	updateHitRate(hit);
	if (hit) {

    // read hit!

    return hit;

	} else { // got miss

		if (lowerCache != nullptr) {
			// In next level cache
			if (lowerCache->read(address)) { //L2 Cache won't get here (because lowerCache==nullptr)
			
				// got hit from prev level cache
				missHandler(setIndex, tagIndex, counterBlocksInLine, address); //miss handler for L1 cache

				return true;

			}

		}

		else { // read miss on 2 caches -> fetching block from Main Mem to cache L2

			missHandler(setIndex, tagIndex, counterBlocksInLine, address);

			return true;	

		}

	}

	return false; // shouldn't get here

}
	
bool Cache::write(unsigned long int address) {
	
	// compute the set index and tag for the given address
	int maskSet = (1 << setSize) - 1;
	int maskTag = (1 << tagSize) - 1;
	uint32_t setIndex = address >> blockSize;
	setIndex &= maskSet;
	setIndexCache = setIndex;
	uint32_t tagIndex = address >> (blockSize + setSize);
	tagIndex &= maskTag;

	int counterBlocksInLine = 0; // #blocks in line
	int i = 0;
	for (i = 0; i< pow(2, assoc); i++) {
		auto it = sets[i].blocks.find(setIndex);
		// check if the key was found
		if (it != sets[i].blocks.end()) {
			counterBlocksInLine++;
		}
	}
	bool hit = false;

	// checks for write hit and update LRU policy

	for (i = 0; i < pow(2, assoc); i++){

		auto it = sets[i].blocks.find(setIndex);

		// check if the key was found

		if (it != sets[i].blocks.end()) {

			// check if the val was found
			if (it->second.tag == tagIndex) {

				// write hit! 
				hit = true;

				if (!wrAlloc && lowerCache == nullptr) {
					it->second.dirty = true;
				}
				else if (lowerCache != nullptr) {
					it->second.dirty = true;
				}
				if (it->second.lru == maxLruSize) {

					// this's MRU

					break;
				}
				updateLruPolicy(setIndex, counterBlocksInLine, &it->second);
				break;
			}
		}
	}

	/* 
	l1 -> hit -> bye 

	l1 -> miss -> l2:

	l2 hit -> write allocate == 1 ? fetch block to prev level cache and write there : write in curr cache -> bye

	l2 miss -> write allocate == 1 ? write block to l2 and l1 : Do nothing-> bye
	*/

	if (!wbFlag) {
		updateHitRate(hit);
	}
	if (hit) {

		// write hit!
		if (wrAlloc && lowerCache == nullptr) {
			// in next level cache -> fetch the block to prev level cache and write there
			return hit;
		}	
		else if (!wrAlloc && lowerCache == nullptr) { // dont fetch block to L1
			return !hit;
		}
		else {
			return hit;
		}
	} else { // got miss

		if (lowerCache != nullptr) {
			
			// In next level cache
			if (lowerCache->write(address)) { //L2 Cache won't get here (because lowerCache==nullptr)
		
			// got hit from prev level cache
			missHandler(setIndex, tagIndex, counterBlocksInLine, address); //miss handler for L1 cache

			return true;

			}
			else {
				return false;
			}
		}
		else { // write miss on 2 caches -> fetching block from Main Mem to cache L2
			missHandler(setIndex, tagIndex, counterBlocksInLine, address);
			return true;	
		}
	}
	return false; // shouldn't get here
}


void Cache::updateHitRate(bool hit) {
		// update the hit rate counter and latency counter
		if (hit) {
			hitCount++;
			totalLatency += hitLatency;
		} else {
			missCount++;
			totalLatency += missLatency;
		}
	}

void Cache::updateLruPolicy(int setIndex, int counterBlocksInLine, CacheBlock* presentBlock) {
		for (int j = 0; j < pow(2, assoc); j++) {
			auto t = sets[j].blocks.find(setIndex);
			// check if the key was found
			if (t != sets[j].blocks.end()) {
				if (t->second.lru > presentBlock->lru) {

					// decrease lru val from othere WAYs members in line
					t->second.lru--;
				}
			}
		}
		// sets the updated LRU value
		presentBlock->lru = counterBlocksInLine;
		return;
	}

void Cache::missHandler(uint32_t setIndex, uint32_t tagIndex, int counterBlocksInLine, unsigned long int address) {

		CacheBlock* blockMin;

		int minValLru = pow(2, assoc);
		int i=0;
		unsigned long int addr = -1;
		if (lowerCache != nullptr) { // In L1 Cache -> check inclusive w/ valid bit
			for (i = 0; i < pow(2, lowerCache->assoc); i++) {
				auto t = lowerCache->sets[i].blocks.find(lowerCache->setIndexCache);
				if (t != lowerCache->sets[i].blocks.end()) {
					if (!t->second.valid) { // L2 Cache block is not valid
						addr = t->second.addr; // address of L2 block evicted
						t->second.addr = address; // Keep align to real address
						blockMin = &t->second;
						
						break;
					}
				}
			}
			if (addr != -1) { //Inclusive need to restore -> evict L1 block before continue to the original miss
				blockMin->valid = true;
				int maskSet = (1 << setSize) - 1;
				int maskTag = (1 << tagSize) - 1;
				uint32_t setIndexEvict = addr >> blockSize;
				setIndexEvict &= maskSet;
				uint32_t tagIndexEvict = addr >> (blockSize + setSize);
				tagIndexEvict &= maskTag;
				// Find block in L1 -> evict and continue to original miss
				for (i=0; i < pow(2, assoc); i++) {
					auto iterator = sets[i].blocks.find(setIndexEvict);
					if (iterator != sets[i].blocks.end()) {
						if (iterator->second.tag == tagIndexEvict) {
							sets[i].blocks.erase(setIndexEvict);
							
							counterBlocksInLine--;
						}
					}
				}
			}
		}
		if (counterBlocksInLine == pow(2, assoc)) {// OVERRIDE
			if (operation == 'w' && !wrAlloc) { // don't fetch this block!!
				return;
			}
			// cache WAY line are full

			// check for LRU, check dirty, override, max size and update lru policy

			for (i = 0; i < pow(2, assoc); i++){
				auto it = sets[i].blocks.find(setIndex);
				if (pow(2, assoc) == 1) {// Fully Assocative
					blockMin = &it->second;
				}
				if (it != sets[i].blocks.end()) {
					if (it->second.lru < minValLru) {
						minValLru = it->second.lru;
						blockMin = &it->second;
					}
				}
				
			}
			
			// check dirty and override block with new one from next level cache
			if (blockMin->dirty) {
				writeBack(blockMin->addr, setIndex, tagIndex);

			}	
			blockMin->tag = tagIndex;
			
			if (lowerCache == nullptr) { //In L2 Cache, need to keep inclusive
				blockMin->valid = false;
			}
			else {
				blockMin->addr = address;
			}
			if (operation == 'w' && lowerCache != nullptr) { // in L1 cache, wr allocate enabled, need to write dirty to new block
				blockMin->dirty = true;
			}
			else if (operation == 'r' && lowerCache != nullptr) { // in L1 cache, wr allocate enabled, need to write dirty to new block
				blockMin->dirty = false;
			}
			updateLruPolicy(setIndex, counterBlocksInLine, blockMin);
		}

		else { // create new block, insert to WAYs line and update LRU policy
			if (operation == 'w' && !wrAlloc) { // don't fetch this block!!
				return;
			}
			CacheBlock newBlock(tagIndex, address);// creating new block
			for (i = 0; i < pow(2, assoc); i++){

				auto it = sets[i].blocks.find(setIndex); 

				if (it == sets[i].blocks.end()) { //This WAY doesn't hold the setIndex
					newBlock.lru = counterBlocksInLine + 1;
					if (lowerCache != nullptr && operation == 'w' && wrAlloc) {
						newBlock.dirty = true;
					}
					sets[i].blocks.insert(make_pair(setIndex, newBlock));
					break;

				}

			}

		}
		return;	

	}

void Cache::writeBack(unsigned long int address, uint32_t setIndex, uint32_t tagIndex) {
	if (lowerCache != nullptr) {
		lowerCache->wbFlag = true;
		lowerCache->write(address);
		lowerCache->wbFlag = false;
	}
	return;
}

double Cache::MissRate() {
	return 1 - ((double)hitCount / (hitCount + missCount));
}

double Cache::AvgAccTime() {
	double calcL1 = MissRate();
	double calcL2 = lowerCache->MissRate();
	return (hitLatency + calcL1*(lowerCache->hitLatency+calcL2*lowerCache->missLatency));
}

void Cache::printStats() {

		// print cache statistics

		cout << "Cache stats:" << endl;

		cout << "Hit rate: " << (double)hitCount / (hitCount + missCount) * 100 << "%" << endl;

		cout << "Miss rate: " << (double)missCount / (hitCount + missCount) * 100 << "%" << endl;

		cout << "Average latency: " << (double)totalLatency / (hitCount + missCount) << " cycles" << endl;

	}

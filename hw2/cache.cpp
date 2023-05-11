/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

/* Made by On Fabian and Muli Klien */



#include <iostream>

#include <fstream>

#include <bitset>

#include <cmath>

#include <vector>

#include "cache.h"

#include <unordered_map>



using namespace std;



Cache::Cache(int setSize, int assoc, int blockSize, int hitLatency, int missLatency, int writeAllocate, Cache* lowerCache) {

	this->setSize = setSize; // #lines in a Way

	this->assoc = assoc;

	this->blockSize = blockSize;

	this->hitLatency = hitLatency;

	this->missLatency = missLatency;

	this->wrAlloc = writeAllocate;

	this->lowerCache = lowerCache;

	this->tagSize = 32 - (blockSize + setSize);

	this->sets = vector<CacheSet>(pow(2, assoc), CacheSet(setSize, assoc));

	this->maxLruSize = pow(2, assoc);
}

Cache::~Cache() {};

bool Cache::read(unsigned long int address) {

        // compute the set index and tag for the given address

		int maskSet = (1 << setSize) - 1;

		int maskTag = (1 << tagSize) - 1;

		uint32_t setIndex = address >> blockSize;

		setIndex &= maskSet;

		uint32_t tagIndex = address >> (blockSize + setSize);

		tagIndex &= maskTag;



		// #blocks in line

		int counterBlocksInLine = 0;

		int i = 0;

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

					updateLruPolicy(setIndex, counterBlocksInLine, it->second);

					break;

				}

			}

		} 	 /* 
				l1 -> hit -> bye 

				l1 -> miss -> l2:

				l2 hit -> write block to l1 -> bye

				l2 miss -> write block to l2 and l1 -> bye

			*/

		updateHitRate(hit);

		if (hit) {

            // read hit!

			hitLatency++; //TODO: change this variable definision

            return hit;

		} else { // got miss

			missLatency++; //TODO: change this variable definision

			// read miss! access lower-level cache (if present) TODO: add check dirty bit (inclusive)

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

	uint32_t tagIndex = address >> (blockSize + setSize);

	tagIndex &= maskTag;
	
	// #blocks in line

	int counterBlocksInLine = 0;

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
				it->second.dirty = true;
				
				if (it->second.lru == maxLruSize) {

					// this's MRU

					break;

				}

				updateLruPolicy(setIndex, counterBlocksInLine, it->second);

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

	updateHitRate(hit);

	if (hit) {

		// write hit!

		if (wrAlloc && lowerCache == nullptr) {
			// in next level cache -> fetch the block to prev level cache and write there
			return hit;
		}	
		return hit;

	} else { // got miss

		if (lowerCache != nullptr) {
			
			// In next level cache
			if (lowerCache->write(address)) { //L2 Cache won't get here (because lowerCache==nullptr)
			
			// got hit from prev level cache
			missHandler(setIndex, tagIndex, counterBlocksInLine, address); //miss handler for L1 cache

			return true;

			}

		}

		else { // wrie miss on 2 caches -> fetching block from Main Mem to cache L2

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

void Cache::updateLruPolicy(int setIndex, int counterBlocksInLine, CacheBlock presentBlock) {

		for (int j = 0; j < pow(2, assoc); j++) {

			auto t = sets[j].blocks.find(setIndex);

			// check if the key was found

			if (t != sets[j].blocks.end()) {

				if (t->second.lru > presentBlock.lru) {

					// decrease lru val from othere WAYs members in line

					t->second.lru--;

				}

			}

		}

		// sets the updated LRU value

		presentBlock.lru = counterBlocksInLine;

		return;

	}

void Cache::missHandler(uint32_t setIndex, uint32_t tagIndex, int counterBlocksInLine, unsigned long int address) {

		CacheBlock* blockMin;

		int minValLru = pow(2, assoc);
		int i=0;

		if (counterBlocksInLine == pow(2, assoc)) {

			// cache WAYs line are full

			// check for LRU, check dirty, override and update lru policy

			for (i = 0; i < pow(2, assoc); i++){

				auto it = sets[i].blocks.find(setIndex);

				if (it != sets[i].blocks.end()) {

					if (it->second.lru < minValLru) {

						minValLru = it->second.lru;

						blockMin = &it->second;

					}

				}

			}

			// check dirty and override block with new one from next level cache

			if (blockMin->dirty) {

				writeBack(address, setIndex, tagIndex);

			}	

			blockMin->tag = tagIndex;

			blockMin->dirty = false;

			blockMin->valid = true;

			updateLruPolicy(setIndex, counterBlocksInLine, *blockMin);

		}

		else { // create new block, insert to WAYs line and update LRU policy

			CacheBlock newBlock(tagIndex);// creating new block

			for (i = 0; i < pow(2, assoc); i++){

				auto it = sets[i].blocks.find(setIndex);

				if (it == sets[i].blocks.end()) { //This WAY doesn't hold the setIndex

					sets[i].blocks.insert(make_pair(setSize, newBlock));

					updateLruPolicy(setIndex, counterBlocksInLine, newBlock);

				}

			}

		}	

	}

void Cache::writeBack(unsigned long int address, uint32_t setIndex, uint32_t tagIndex) {

    if (lowerCache != nullptr) {

		lowerCache->write(address);
       }
}


void Cache::printStats() {

		// print cache statistics

		cout << "Cache stats:" << endl;

		cout << "Hit rate: " << (double)hitCount / (hitCount + missCount) * 100 << "%" << endl;

		cout << "Miss rate: " << (double)missCount / (hitCount + missCount) * 100 << "%" << endl;

		cout << "Average latency: " << (double)totalLatency / (hitCount + missCount) << " cycles" << endl;

	}

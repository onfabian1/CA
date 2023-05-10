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

Cache::Cache(int setSize, int assoc, int blockSize, int hitLatency, int missLatency, bool writeAllocate, Cache* lowerCache) {
        this->setSize = setSize; // #lines in a Way
        this->assoc = assoc;
        this->blockSize = blockSize;
        this->hitLatency = hitLatency;
        this->missLatency = missLatency;
        this->writeAllocate = writeAllocate;
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
		} 	 /* 	l1 -> hit -> bye 
				l1 -> miss -> l2:
				l2 hit -> write block to l1 -> bye
				l2 miss -> write block to l2 and l1 -> bye
			*/
		updateHitRate(hit);
		if (hit) {
            		// read hit!
			hitLatency++; //TODO: change this variable definision
            		return hit;
        	} else {
			missLatency++; //TODO: change this variable definision
            		// read miss in L1 Cache! access lower-level cache (if present) TODO: add check dirty bit (inclusive)
            		if (lowerCache != nullptr) {
               			if (lowerCache->read(address)) { // got hit from prev level cache
					missHandler(setIndex, tagIndex, counterBlocksInLine); //miss handler for L1 cache
					return true;
				}
        		}
			else { // read miss on 2 caches -> fetching block from Main Mem to cache L2
				missHandler( setIndex, tagIndex, counterBlocksInLine);
				return false;	
			}
		}
		return true; // shouldn't get here
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

void Cache::missHandler(uint32_t setIndex, uint32_t tagIndex, int counterBlocksInLine) {

		CacheBlock* blockMin;

		int minValLru = pow(2, assoc);
		int i=0;
		// block present in lower cache - hit!

		if (counterBlocksInLine == pow(2, assoc)) {

			// l2 level cache WAYs line are full

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
				//writeBack(setIndex, tagIndex);
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


void Cache::printStats() {
		// print cache statistics
		cout << "Cache stats:" << endl;
		cout << "Hit rate: " << (double)hitCount / (hitCount + missCount) * 100 << "%" << endl;
		cout << "Miss rate: " << (double)missCount / (hitCount + missCount) * 100 << "%" << endl;
		cout << "Average latency: " << (double)totalLatency / (hitCount + missCount) << " cycles" << endl;
	}

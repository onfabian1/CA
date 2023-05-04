/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */
/* This assignment was made by On fabian & Muli Klein */

#include "bp_api.h"
#include <math.h>
#include <iostream>
#include <vector>
#include <bitset>

using namespace std;

enum state {SNT = 0, WNT = 1, WT = 2, ST = 3};

const int NOT_USING_SHARE = 0;
const int USING_SHARE_LSB = 1;
const int USING_SHARE_MID = 2;
 
 
class fsm_2bit {
	private:
		unsigned curr_state;

	public:
		fsm_2bit(): curr_state(0) {}
		~fsm_2bit() = default;
		int get_state();
		void move_state(bool t);
		void set_state(unsigned curr_state_tmp);
};

int fsm_2bit::get_state() {
	return curr_state;
}

void fsm_2bit::move_state(bool t) {
	if ((t==true) && (curr_state != ST)) {
		curr_state++;
	} else if ((t==false) && (curr_state != SNT)) {
		curr_state--;
	}
}

void fsm_2bit::set_state(unsigned curr_state_tmp) {
	curr_state = curr_state_tmp;
} 

class BP_row {
	private:
		uint32_t tag;
		uint32_t target;
		bool isGlobalHist;
		bool isGlobalTable;
		int shared; 
		unsigned fsmState;
		int history_size;
		uint32_t history;
		std::vector<fsm_2bit> fsms;
	public:
		static uint32_t global_history; 
		static vector<fsm_2bit> global_fsms;
		BP_row(uint32_t tmp_tag, uint32_t tmp_target, bool tmp_isGlobalHist, bool tmp_isGlobalTable, int tmp_isGlobalTable, unsigned tmp_fsmState, int tmp_historySize);
		~BP_row() = default;
		
		uint32_t gettmp_tag(); 

		void updateTarget(uint32_t tmp_target);

		void changeHistory(bool decision);

		void changeFSMState(bool decision, uint32_t pc);

		bool getFSMstate(uint32_t pc, uint32_t *dst);
};

BP_row::BP_row(uint32_t tmp_tag, uint32_t tmp_target, bool tmp_isGlobalHist, bool tmp_isGlobalTable, int tmp_isGlobalTable, unsigned tmp_fsmState, int tmp_historySize) {
	tag = tmp_tag;
	target = tmp_target;
	isGlobalHist = tmp_isGlobalHist;
	isGlobalTable = tmp_isGlobalTable;
	shared = tmp_isGlobalTable;
	fsmState = tmp_fsmState;
	history_size = tmp_historySize;
	fsms = vector<fsm_2bit>(int(pow(2, tmp_historySize)));

	if (isGlobalHist) {
		history = 0;
	}
	if (isGlobalTable) {
		fsms.clear();
	}else {
		for(vector<fsm_2bit>::iterator it = fsms.begin(); it != fsms.end(); ++it) {
			it->set_state(fsmState);
		}
	}
}

uint32_t BP_row::gettmp_tag() {
	return this->tag;
}

void BP_row::updateTarget(uint32_t tmp_target) {
	target = tmp_target;
}

void BP_row::changeHistory(bool decision) {
	if (isGlobalHist) {
		global_history = (global_history*2) | decision;
		global_history &= (pow(2,history_size)) - 1;
	    } else {
		history = (history*2) | decision;
		history &= (global_history*2) - 1;
	    }
}

void BP_row::changeFSMState(bool decision, uint32_t pc) {
	uint32_t index = 0;
	uint32_t pc_masked = 0;
    
	if (isGlobalHist && isGlobalTable) {
    		if (shared == NOT_USING_SHARE) {
                index = global_history;
        } else if (shared == USING_SHARE_LSB) {
		pc_masked = pc/pow(2,2);
        } else if (shared == USING_SHARE_MID) {
	        pc_masked = pc/pow(2,16);
        }
	index = pc_masked ^ global_history;
        index &= (pow(2,history_size)) - 1;
        global_fsms[index].move_state(decision);
    } else if (isGlobalTable) {
        if (shared == NOT_USING_SHARE) {
     	       index = history;
        } else if (shared == USING_SHARE_LSB) {
               pc_masked = pc/pow(2,2);
        } else if (shared == USING_SHARE_MID) {
               pc_masked = pc/pow(2,16);
        }
	index = pc_masked ^ history;
        index &= (pow(2,history_size)) - 1;
        global_fsms[index].move_state(decision);
    } else if (isGlobalHist) {
	index = global_history & ((pow(2,history_size)) - 1);
        fsms[index].move_state(decision);
    } else {
        index = history & ((pow(2,history_size)) - 1);
        fsms[index].move_state(decision);
    }
}


bool BP_row::getFSMstate(uint32_t pc, uint32_t *dst) {
	int index;
	uint32_t historyValue;
	switch (shared) {

		case NOT_USING_SHARE: 
		    if (isGlobalHist && isGlobalTable) {
		    	index = global_history & ((pow(2,history_size)) - 1);
		    } 
		    else if (isGlobalTable) {
		    	index = history & ((pow(2,history_size)) - 1);
		    } 
		    else {
		    	index = history & ((pow(2,history_size)) - 1);
		    }
		    break;

		case USING_SHARE_LSB:
		    historyValue = isGlobalHist ? global_history : history;
		    index = (((pc/pow(2,2)) ^ historyValue) & ((pow(2,history_size)) - 1));
		    break;

		case USING_SHARE_MID:
		    historyValue = isGlobalHist ? global_history : history;
		    index = (((pc/pow(2,16)) ^ historyValue) & ((pow(2,history_size)) - 1));
		    break;

		default:
		    // Invalid shared value
		    return false;
	}

	if (index >= 0 && index < (pow(2,history_size))) {
		if (global_fsms[index].get_state() & 2) {
		    *(dst) = target; 
		    return true;
		} 
		else {
		    *(dst) = pc + 4;
		    return false;
		}
	} else if (index >= 0 && index < (pow(2,history_size))) {
	 	if (fsms[index].get_state() & 2) {
		    *(dst) = target; 
		    return true;
		} else {
		    *(dst) = pc + 4;
		    return false;
		}
	} 
	else {
		// Invalid index value
		return false;
	}
	return 0;
}


class BP {

	public:
		unsigned btbSize;
		unsigned historySize; 
		unsigned tagSize;
		unsigned fsmState;
		bool isGlobalHist;
		bool isGlobalTable; 
		int Shared;
		unsigned flushes_counter;
		unsigned updates_counter;
		vector<BP_row*> btbTable;

		BP(unsigned _btbSize, unsigned tmp_historySize, unsigned tmp_tagSize, unsigned tmp_fsmState, bool tmp_isGlobalHist, bool tmp_isGlobalTable, int tmp_isGlobalTable):
			btbSize(_btbSize), 
			historySize(tmp_historySize),
			tagSize(tmp_tagSize),
			fsmState(tmp_fsmState),
			isGlobalHist(tmp_isGlobalHist),
			isGlobalTable(tmp_isGlobalTable),
			Shared(tmp_isGlobalTable),
			flushes_counter(0),
			updates_counter(0),
			btbTable(std::vector<BP_row*>(btbSize))	{}

		~BP() {
			for (vector<BP_row*>::iterator it = btbTable.begin(); it != btbTable.end(); ++it) {
				if ((*it)) {
					delete (*it);
				}
			}
		}

		int index_size(unsigned btbSize);

		bool getTagFromBTB(uint32_t pc, uint32_t* tmp_pcTag, uint32_t* tmp_btb_index, uint32_t* tmp_tag_in_btb);

};

int BP::index_size(unsigned btbSize) {
	return int(floor(log2(btbSize)));
}

bool BP::getTagFromBTB(uint32_t pc, uint32_t* tmp_pcTag, uint32_t* tmp_btb_index, uint32_t* tmp_tag_in_btb) {
	uint32_t btb_index = pc >> 2; 
	uint32_t pctmp_tag = btb_index; 
	int btb_index_size = index_size(btbSize);
	pctmp_tag = (pctmp_tag >> btb_index_size) & ((1 << tagSize) - 1);
	btb_index = btb_index & ((pow(2,btb_index_size)) - 1);
	*tmp_btb_index = btb_index;
	*tmp_pcTag = pctmp_tag;
	if (btbTable[btb_index]) {
		*tmp_tag_in_btb = btbTable[btb_index]->gettmp_tag();
		return true;
	}
	return false;
}


BP* bp;
vector<fsm_2bit> BP_row::global_fsms = vector<fsm_2bit>(0);
uint32_t BP_row::global_history = 0;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared) {
	try {
		bp = new BP(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
		if (bp == NULL) {
			return -1;
		}
		BP_row::global_fsms.resize(int(pow(2, historySize)));
		for(vector<fsm_2bit>::iterator it = BP_row::global_fsms.begin(); it != BP_row::global_fsms.end(); ++it) {
			it->set_state(fsmState);
		}
		return 0;
	} 
	catch (bad_alloc& e) {
		return -1;
	}
}


bool BP_predict(uint32_t pc, uint32_t* dst) { 
    // Initialize variables for the BTB tag, index, and tag in BTB
    uint32_t tag, btb_index, tag_in_btb;
    
    // Check if the BTB has an entry for the given PC and retrieve the tag, index, and tag in BTB if it does
    bool flag = bp->getTagFromBTB(pc, &tag, &btb_index, &tag_in_btb);

    // If the BTB has an entry for the given PC, use the FSM state associated with the BTB entry to predict the branch destination
    if (tag_in_btb == tag && flag) {
        return bp->btbTable[btb_index]->getFSMstate(pc, dst);
    } 
    
    // If the BTB does not have an entry for the given PC, predict that the branch will not be taken and return the incremented PC
    else { 
        *dst = pc + 4;
        return false;
    } 
}

void updateExistingLine(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, uint32_t* btb_index) {
	if (btb_index == nullptr) {
		return;
	}
	if (bp->btbTable[*btb_index] == nullptr) {
		return;
	}
	bp->btbTable[*btb_index]->updateTarget(targetPc);
	bp->btbTable[*btb_index]->changeFSMState(taken, pc);
	bp->btbTable[*btb_index]->changeHistory(taken);
	
	if (((targetPc != pred_dst) && (taken)) || ( (pred_dst != (pc + 4) ) && (!taken) ) ) {
		bp->flushes_counter++;
	}
}

void createNewLine(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, uint32_t* pctmp_tag, uint32_t* btb_index) {
	bp->btbTable[*btb_index] = new BP_row(*pctmp_tag, targetPc, bp->isGlobalHist, bp->isGlobalTable, bp->Shared, bp->fsmState, bp->historySize);
	bp->btbTable[*btb_index]->changeFSMState(taken, pc);
	bp->btbTable[*btb_index]->changeHistory(taken);
	
	if (taken && (targetPc != pc + 4) && (targetPc != pred_dst)) {
		bp->flushes_counter++;
	}
}


void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
    uint32_t tag, btb_index, tag_in_btb;
    bool flag = bp->getTagFromBTB(pc, &tag, &btb_index, &tag_in_btb);

    if (tag_in_btb == tag && flag) { 
        updateExistingLine(pc, targetPc, taken, pred_dst, &btb_index);
    } else { 
        createNewLine(pc, targetPc, taken, pred_dst, &tag, &btb_index);
    }
    
    bp->updates_counter++;
}

void BP_GetStats(SIM_stats *curStats) {
	curStats->br_num = bp->updates_counter;
	curStats->flush_num = bp->flushes_counter;
	int btb_size_term = bp->btbSize * (1 + bp->tagSize + 30);
	int history_size_term = 2 * (int(pow(2, bp->historySize)));
	if (bp->isGlobalTable && bp->isGlobalHist) {
		curStats->size = btb_size_term + bp->historySize + history_size_term;
	} else if (bp->isGlobalTable) {
		curStats->size = btb_size_term + bp->historySize + history_size_term + bp->historySize;
	} else if (bp->isGlobalHist) {
		curStats->size = btb_size_term + bp->historySize + history_size_term + history_size_term;
	} else {
		curStats->size = btb_size_term + bp->historySize + history_size_term + bp->historySize + history_size_term;
	}
	return;
}
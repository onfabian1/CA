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
 
/*
  Fsms class Bimodal 
*/
class FsmBimodal {
	private:
	unsigned curr_state;

	public:
	FsmBimodal(): curr_state(0) {}
	~FsmBimodal() = default;
	int getState();
	void moveState(bool t);
	void setState(unsigned tmp_currState);
};

int FsmBimodal:: getState() {
	return curr_state;
}

void FsmBimodal::moveState(bool it) {
	if (it && curr_state != ST) {
		curr_state = static_cast<state>(static_cast<int>(curr_state) + 1);
	} else if (!it && curr_state != SNT) {
		curr_state = static_cast<state>(static_cast<int>(curr_state) - 1);
	}
}


void FsmBimodal::setState(unsigned tmp_currState) {
	curr_state = tmp_currState;
} 
/*
Class indicates of rows in BP table
*/
class BP_row {
	private:
		bool isGlobalHist;
		bool isGlobalTable;
		int isShare; 
		unsigned fsmState;
		int history_size;
		uint32_t local_history;
		uint32_t tag;
		uint32_t target;
		vector<FsmBimodal> fsms_bimodal;
	public:
		static uint32_t globalHistory; 
		static vector<FsmBimodal> globalFsms;
		BP_row(uint32_t tmp_tag, uint32_t tmp_target, bool tmp_isGlobalHist, bool tmp_isGlobalTable, int tmp_shared, unsigned tmp_fsmState, int tmp_historySize);
		~BP_row() = default;
		
		uint32_t getTag(); 

		void updateTarget(uint32_t tmp_target);

		uint32_t move_bit_left(uint32_t var1, uint32_t var2);
		
		uint32_t move_bit_right(uint32_t var1, uint32_t var2);

		void changeHistory(bool flag);

		void changeFsmState(bool flag, uint32_t pc);

		bool getFsmState(uint32_t pc, uint32_t *dst);
};

BP_row::BP_row(uint32_t tmp_tag, uint32_t tmp_target, bool tmp_isGlobalHist, bool tmp_isGlobalTable, int tmp_shared, unsigned tmp_fsmState, int tmp_historySize) {
	tag = tmp_tag;
	target = tmp_target;
	isGlobalHist = tmp_isGlobalHist;
	isGlobalTable = tmp_isGlobalTable;
	isShare = tmp_shared;
	fsmState = tmp_fsmState;
	history_size = tmp_historySize;
	fsms_bimodal = vector<FsmBimodal>(int(pow(2, tmp_historySize)));

	if (isGlobalHist) {
		local_history = 0;
	}
	if (isGlobalTable) {
		fsms_bimodal.clear();
	}else {
		for(vector<FsmBimodal>::iterator it = fsms_bimodal.begin(); it != fsms_bimodal.end(); ++it) {
			it->setState(fsmState);
		}
	}
}

uint32_t BP_row::getTag() {
	return this->tag;
}

void BP_row::updateTarget(uint32_t tmp_target) {
	target = tmp_target;
}

uint32_t BP_row::move_bit_left(uint32_t var1, uint32_t var2){
	return var1<<var2;
}

uint32_t BP_row::move_bit_right(uint32_t var1, uint32_t var2){
	return var1>>var2;
}
/*
changeHistory - history is updated depends if local or global
flag - indicates if Taken or Not
*/
void BP_row::changeHistory(bool flag) {
	if (isGlobalHist) {
		globalHistory = move_bit_left(globalHistory,1) | flag;
		globalHistory &= move_bit_left(1,history_size) - 1;
	    } else {
		local_history = move_bit_left(local_history,1) | flag;
		local_history &= move_bit_left(1,history_size) - 1;
	    }
}


void BP_row::changeFsmState(bool decision, uint32_t pc) {
	uint32_t index = 0;
	uint32_t pc_masked = 0;
    
	if (isGlobalHist && isGlobalTable) {
    		if (isShare == NOT_USING_SHARE) {
                index = globalHistory;
        } else if (isShare == USING_SHARE_LSB) {
		pc_masked = move_bit_right(pc,2);
        } else if (isShare == USING_SHARE_MID) {
	        pc_masked = move_bit_right(pc,16);
        }
	index = pc_masked ^ globalHistory;
        index &= move_bit_left(1,history_size) - 1;
        globalFsms[index].moveState(decision);
    } else if (isGlobalTable) {
        if (isShare == NOT_USING_SHARE) {
     	       index = local_history;
        } else if (isShare == USING_SHARE_LSB) {
               pc_masked = move_bit_right(pc,2);
        } else if (isShare == USING_SHARE_MID) {
               pc_masked = move_bit_right(pc,16);
        }
	index = pc_masked ^ local_history;
        index &= move_bit_left(1,history_size) - 1;
        globalFsms[index].moveState(decision);
    } else if (isGlobalHist) {
	index = globalHistory & (move_bit_left(1,history_size) - 1);
        fsms_bimodal[index].moveState(decision);
    } else {
        index = local_history & (move_bit_left(1,history_size) - 1);
        fsms_bimodal[index].moveState(decision);
    }
}


bool BP_row::getFsmState(uint32_t pc, uint32_t *dst) {
	int index;
	uint32_t historyValue;
	switch (isShare) {

		case NOT_USING_SHARE: 
		    if (isGlobalHist && isGlobalTable) {
		    	index = globalHistory & (move_bit_left(1,history_size) - 1);
		    } 
		    else if (isGlobalTable) {
		    	index = local_history & (move_bit_left(1,history_size) - 1);
		    } 
		    else {
		    	index = local_history & (move_bit_left(1,history_size) - 1);
		    }
		    break;

		case USING_SHARE_LSB:
		    historyValue = isGlobalHist ? globalHistory : local_history;
		    index = ((move_bit_right(pc,2) ^ historyValue) & (move_bit_left(1,history_size) - 1));
		    break;

		case USING_SHARE_MID:
		    historyValue = isGlobalHist ? globalHistory : local_history;
		    index = ((move_bit_right(pc,16) ^ historyValue) & (move_bit_left(1,history_size) - 1));
		    break;

		default:
		    // Invalid shared value
		    return false;
	}

	if (index >= 0 && index < move_bit_left(1,history_size)) {
		if (globalFsms[index].getState() & 2) {
		    *(dst) = target; 
		    return true;
		} 
		else {
		    *(dst) = pc + 4;
		    return false;
		}
	} else if (index >= 0 && index < move_bit_left(1,history_size)) {
	 	if (fsms_bimodal[index].getState() & 2) {
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
		unsigned flushes;
		unsigned updates;
		vector<BP_row*> BtbTable;

		BP(unsigned tmp_btbSize, unsigned tmp_historySize, unsigned tmp_tagSize, unsigned tmp_fsmState, bool tmp_isGlobalHist, bool tmp_isGlobalTable, int tmp_shared):
			btbSize(tmp_btbSize), 
			historySize(tmp_historySize),
			tagSize(tmp_tagSize),
			fsmState(tmp_fsmState),
			isGlobalHist(tmp_isGlobalHist),
			isGlobalTable(tmp_isGlobalTable),
			Shared(tmp_shared),
			flushes(0),
			updates(0),
			BtbTable(vector<BP_row*>(btbSize))	{}

		~BP() {
			for (vector<BP_row*>::iterator it = BtbTable.begin(); it != BtbTable.end(); ++it) {
				if ((*it)) {
					delete (*it);
				}
			}
		}

		int indexSize(unsigned btbSize);

		bool getBtbTag(uint32_t pc, uint32_t* tmp_pc_tag, uint32_t* tmp_btb_index, uint32_t* tmp_tag_in_btb);

};

int BP::indexSize(unsigned btbSize) {
	return int(floor(log2(btbSize)));
}



int calcIndexSize(int btbSize) {
    return log2(btbSize);
}

uint32_t calcPcTag(uint32_t pc, int indexSize, int tagSize) {
    uint32_t btbIndex = pc >> 2;
    uint32_t pcTag = btbIndex;
    pcTag = (pcTag >> indexSize) & ((1 << tagSize) - 1);
    return pcTag;
}

uint32_t calcBtbIndex(uint32_t pc, int indexSize) {
    uint32_t btbIndex = pc >> 2;
    btbIndex = btbIndex & ((1 << indexSize) - 1);
    return btbIndex;
}

bool BP::getBtbTag(uint32_t pc, uint32_t* tmp_pc_tag, uint32_t* tmp_btb_index, uint32_t* tmp_tag_in_btb) {
    int indexSize = calcIndexSize(btbSize);
    uint32_t pcTag = calcPcTag(pc, indexSize, tagSize);
    uint32_t btbIndex = calcBtbIndex(pc, indexSize);
    *tmp_pc_tag = pcTag;
    *tmp_btb_index = btbIndex;
    if (BtbTable[btbIndex]) {
        *tmp_tag_in_btb = BtbTable[btbIndex]->getTag();
        return true;
    }
    return false;
}


BP* bp;
vector<FsmBimodal> BP_row::globalFsms = vector<FsmBimodal>(0);
uint32_t BP_row::globalHistory = 0;


void set_initial_fsm_state(unsigned historySize, unsigned fsmState) {
	BP_row::globalFsms.resize(int(pow(2, historySize)));
	for (auto& fsm : BP_row::globalFsms) {
		fsm.setState(fsmState);
	}
}

BP* create_BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared) {
	BP* new_bp = new BP(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	return new_bp;
}

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared) {
	try {
		bp = create_BP(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
		if (bp == NULL) {
			return -1;
		}
		set_initial_fsm_state(historySize, fsmState);
		return 0;
	} 
	catch (bad_alloc& e) {
		return -1;
	}
}


bool BP_predict(uint32_t pc, uint32_t* dst) { 
    uint32_t tag, btb_index, tag_in_btb;
    bool flag = bp->getBtbTag(pc, &tag, &btb_index, &tag_in_btb);
    if (tag_in_btb == tag && flag) {
        return bp->BtbTable[btb_index]->getFsmState(pc, dst);
    } 
    else { 
        *dst = pc + 4;
        return false;
    } 
}

void updateExistingLine(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, uint32_t* btb_index) {
	if (btb_index == nullptr) {
		return;
	}
	if (bp->BtbTable[*btb_index] == nullptr) {
		return;
	}
	bp->BtbTable[*btb_index]->updateTarget(targetPc);
	bp->BtbTable[*btb_index]->changeFsmState(taken, pc);
	bp->BtbTable[*btb_index]->changeHistory(taken);
	
	if (((targetPc != pred_dst) && (taken)) || ( (pred_dst != (pc + 4) ) && (!taken) ) ) {
		bp->flushes++;
	}
}

void createNewLine(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, uint32_t* pc_tag, uint32_t* btb_index) {
	bp->BtbTable[*btb_index] = new BP_row(*pc_tag, targetPc, bp->isGlobalHist, bp->isGlobalTable, bp->Shared, bp->fsmState, bp->historySize);
	bp->BtbTable[*btb_index]->changeFsmState(taken, pc);
	bp->BtbTable[*btb_index]->changeHistory(taken);
	
	if (taken && (targetPc != pc + 4) && (targetPc != pred_dst)) {
		bp->flushes++;
	}
}


void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
    uint32_t tag, btb_index, tag_in_btb;
    bool flag = bp->getBtbTag(pc, &tag, &btb_index, &tag_in_btb);

    if (tag_in_btb == tag && flag) { 
        updateExistingLine(pc, targetPc, taken, pred_dst, &btb_index);
    } else { 
        createNewLine(pc, targetPc, taken, pred_dst, &tag, &btb_index);
    }
    
    bp->updates++;
}

void BP_GetStats(SIM_stats *curStats) {
	curStats->br_num = bp->updates;
	curStats->flush_num = bp->flushes;
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

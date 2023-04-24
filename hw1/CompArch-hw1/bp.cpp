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

/* FSM class */
class FSM {
	private:
		unsigned m_p_state;

	public:
	/* FSM constructor */
	FSM(): m_p_state(0) {}
	/* FSM Distructor */
	~FSM() = default; 
	void set_state(unsigned p_state);
	void move_state(bool it);
	int get_state();
};

void FSM::set_state(unsigned p_state) {
		m_p_state = p_state;
}

void FSM::move_state(bool it) {
	if ((it) && (this->m_p_state != ST)) {
		this->m_p_state++;
	} else if ((!it) && (this->m_p_state != SNT)) {
		this->m_p_state--;
	}
}

int FSM::get_state() {
	return this->m_p_state;
}
/* End of FSM */


/* btbRow Class */
class btbRow{
	private:
		uint32_t m_tag;
		uint32_t m_target;
		bool m_isGlobalHist;
		bool m_isGlobalTable;
		int m_shared;
		unsigned m_fsmstate;
		int m_historySize;
		uint32_t m_history;
		std::vector<FSM> m_fsms;
	public:
		btbRow(uint32_t tag, uint32_t target, bool isGlobalHist, 
			   bool msGlobalTable, int shared, unsigned fsmstate, int historySize);
		static uint32_t GHR; 
		static std::vector<FSM> GFSM;
		uint32_t get_tag();
		void updateTarget(uint32_t target);
		void change_history(bool decision);
		void changem_fsmstate(uint32_t pc, bool decision);
		bool get_fsmstate(uint32_t pc, uint32_t* dst);
		~btbRow() = default;
};

btbRow::btbRow(uint32_t tag, uint32_t target, bool isGlobalHist, 
			   bool isGlobalTable, int shared, unsigned fsmstate, int historySize){
				this->m_tag = tag;
				this->m_target = target;
				this->m_shared = shared;	
				this->m_isGlobalHist = isGlobalHist;
				this->m_historySize = historySize;						
				this->m_isGlobalTable = isGlobalTable;									
				this->m_fsms = std::vector<FSM>(int(pow(2, historySize)));						
				this->m_fsmstate = fsmstate; 																		
														
				if ((this->m_isGlobalHist) && (this->m_isGlobalTable)) {
					this->m_history = 0;
					this->m_fsms = std::vector<FSM>(0);
				}	else if (this->m_isGlobalTable) {
					this->m_fsms = std::vector<FSM>(0);
				}	else if (this->m_isGlobalHist) {
					this->m_history = 0;
				} else {
					this->m_history = 0;
				}
				if (!this->m_isGlobalTable) {
					for(std::vector<FSM>::iterator it = m_fsms.begin(); it != m_fsms.end(); ++it) {
						it->set_state(this->m_fsmstate);
					}
				}
			}
			
uint32_t btbRow::get_tag() {
			return this->m_tag;
		}

void btbRow::updateTarget(uint32_t target) {
			this->m_target = target;
		}
		
void btbRow::change_history(bool decision){
    if (this->m_isGlobalHist) {
        this->GHR = (this->GHR << 1) | decision;
        this->GHR &= (1 << this->m_historySize) - 1;
    } else {
        this->m_history = (this->m_history << 1) | decision;
        this->m_history &= (1 << this->m_historySize) - 1;
    }
}


void btbRow::changem_fsmstate(uint32_t pc, bool decision) {
    int index;
    switch (m_shared) {
        case 0: // no sharing
            if (m_isGlobalHist && this->m_isGlobalTable) {
                GFSM[this->GHR].move_state(decision);
            } else if (m_isGlobalTable) {
                GFSM[this->m_history].move_state(decision);
            } else if (this->m_isGlobalHist) {
                index = GHR & ((1 << m_historySize) - 1);
                m_fsms[index].move_state(decision);
            } else {
                index = this->m_history & ((1 << m_historySize) - 1);
                m_fsms[index].move_state(decision);
            }
            break;
        case 1: // 8 LSB of PC XOR history
            index = (((pc >> 2) ^ (this->m_isGlobalHist ? this->GHR : this->m_history)) & ((1 << this->m_historySize) - 1));
            if (this->m_isGlobalHist && this->m_isGlobalTable) {
                GFSM[index].move_state(decision);
            } else if (m_isGlobalTable) {
                GFSM[index].move_state(decision);
            } else {
                m_fsms[index].move_state(decision);
            }
            break;
        case 2: // 16 MSB of PC XOR history
            index = (((pc >> 16) ^ (m_isGlobalHist ? GHR : m_history)) & ((1 << m_historySize) - 1));
            if (m_isGlobalHist && m_isGlobalTable) {
                GFSM[index].move_state(decision);
            } else if (m_isGlobalTable) {
                GFSM[index].move_state(decision);
            } else {
                m_fsms[index].move_state(decision);
            }
            break;
        default:
            // error handling
            break;
    }
}

/*
 * @brief getFSMstate - returns the state of the fsm,
 * and updates dst to be the appropriate value
 * @param pc - pc of the branch command
 * @param dst - where to jump
 * @return true - branch is taken
 * @return false - branch is not taken
 */
bool btbRow::get_fsmstate(uint32_t pc, uint32_t* dst) {
    int index;
    uint32_t historyValue;

    switch (m_shared) {
        case 0:
            if (m_isGlobalHist && m_isGlobalTable) {
                index = GHR & ((1 << m_historySize) - 1);
            } else if (m_isGlobalTable) {
                index = m_history & ((1 << m_historySize) - 1);
            } else {
                index = m_history & ((1 << m_historySize) - 1);
            }
            break;

        case 1:
            historyValue = m_isGlobalHist ? GHR : m_history;
            index = (((pc >> 2) ^ historyValue) & ((1 << m_historySize) - 1));
            break;

        case 2:
            historyValue = m_isGlobalHist ? GHR : m_history;
            index = (((pc >> 16) ^ historyValue) & ((1 << m_historySize) - 1));
            break;

        default:
            // Invalid shared value
            return false;
    }

    if (index >= 0 && index < (1 << m_historySize)) {
        if (GFSM[index].get_state() & 2) {
            *(dst) = m_target; 
            return true;
        } else {
            *(dst) = pc + 4;
            return false;
        }
    } else if (index >= 0 && index < (1 << m_historySize)) {
        if (m_fsms[index].get_state() & 2) {
            *(dst) = m_target; 
            return true;
        } else {
            *(dst) = pc + 4;
            return false;
        }
    } else {

        // Invalid index value
        return false;
    }
	return 0;
}
/* End tbtRow class */


/* BTB Table class */
class BTBTable{
private:
	unsigned m_btbSize;
	unsigned m_historySize;
	unsigned m_tagSize;
	unsigned m_fsmstate;
	bool m_isGlobalHist;
	bool m_isGlobalTable;
	int m_Shared;
	std::vector <btbRow*> btbList;
	unsigned num_of_flush;
	unsigned num_of_update;

public:
	
	/* BTB Constructor */
	BTBTable(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmstate, bool isGlobalHist,
	bool isGlobalTable, int Shared) : m_btbSize(btbSize), m_historySize(historySize), m_tagSize(tagSize), m_fsmstate(fsmstate),
	m_isGlobalTable(isGlobalTable), m_Shared(Shared), btbList(std::vector<btbRow*>(btbSize)), num_of_flush(0), num_of_update(0) {}

	/* BTB distructor */
	~BTBTable() {
		for (vector<btbRow*>::iterator it = btbList.begin(); it != btbList.end(); ++it) {
			if ((*it)) {
				delete (*it);
			}
		}
	}
	int entry_size();
	
};

int BTBTable::entry_size() {
	return int(floor(log2(this->m_btbSize)));
}
/*End of BTBTable */


// bp = pointer to BTB Table 
BTBTable *bp;
std::vector<FSM> btbRow::GFSM = std::vector<FSM>(0);
uint32_t btbRow::GHR = 0;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmstate,
			bool isGlobalHist, bool isGlobalTable, int Shared) {
				try {
					bp = new BTBTable(btbSize, historySize, tagSize, fsmstate, isGlobalHist, isGlobalTable, Shared);
					if (bp == NULL) {
						return -1;
					}
					btbRow::GFSM.resize(int(pow(2, historySize)));
					for(std::vector<FSM>::iterator it = btbRow::GFSM.begin(); it != btbRow::GFSM.end(); ++it) {
						it->set_state(fsmstate);
					}
					return 0;
				} 
				catch (std::bad_alloc& e) {
					return -1;
				}
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}


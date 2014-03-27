#include "pcb.h"
#include "sofmgr.h"
#include "machine.h"
#include "pof.h"

extern SOFMgr *sofMgr;

PCB::PCB(unsigned int inputPID) : 
		PID(inputPID), thisThread(NULL), parentPCB(NULL), 
		childrenPCB(), childExitValue(0), pofMap(POF_MAXNUM) {
	parentPCB = 0;
}

PCB::~PCB() {
	for (int i = 0; i < POF_MAXNUM; ++i) {
		if (pofMap.Test(i)) {
			pofMap.Clear(i);
			int sofIndex = pofArray[i]->sofIndex;
			sofMgr->RemoveSOF(sofIndex);
			delete pofArray[i];
		}

	}
}

void PCB::AppendChildPCB(PCB *inputPCB) {
	childrenPCB.SortedInsert((void *)inputPCB, inputPCB->PID);
}

void PCB::RemoveChildPCB(PCB *inputPCB) {
	int pid = inputPCB->PID;
	childrenPCB.SortedRemove(&pid);
}

void setParentPCBNULL(int input) {
	PCB *inputPCB = (PCB *)input;
	inputPCB->parentPCB = NULL;
}

void PCB::RemoveChildParentPCB() {	
	if (!childrenPCB.IsEmpty()) {
		DEBUG('s',"PCB [%d] Remove Chilren's Parent\n", PID);
		childrenPCB.Mapcar(setParentPCBNULL);
	}
	else
		DEBUG('s',"PCB [%d] Remove No Child's Parent\n", PID);
	DEBUG('s',"\n");
}

void PCB::RemoveParentPCBChild(int inputValue) {
	if (parentPCB != NULL) {
		DEBUG('s', "PCB [%d] Remove ParentPCB [%d]\n", PID, parentPCB->PID);
		parentPCB->RemoveChildPCB(this);
		parentPCB->childExitValue = inputValue;
	}
	else
		DEBUG('s',"PCB [%d] Remove No ParentPCB\n", PID);
	DEBUG('s',"\n");
}

void printChildPCB(int input) {
	PCB *inputPCB = (PCB *)input;
	DEBUG('s',"Child PCB: [0x%x], PID [%d], ParentPCB [0x%x]\t", 
			inputPCB, inputPCB->PID, inputPCB->parentPCB);
}

void PCB::PrintPCB() {
	DEBUG('s',"Print PCB: [0x%x], PID [%d], ", this, PID);
	if (thisThread)
		DEBUG('s', "Thread [0x%x], ", thisThread);
	else
		DEBUG('s', "Thread NULL, ");
	DEBUG('s', "ChildExitValue %d, ", childExitValue);
	if (parentPCB)
		DEBUG('s', "ParentPCB [0x%x] PID [%d], ", 
				parentPCB, parentPCB->PID);
	else
		DEBUG('s', "ParentPCB NULL, ");
	if (!childrenPCB.IsEmpty())
		childrenPCB.Mapcar(printChildPCB);
	else
		DEBUG('s',"Children No");
	DEBUG('s',"\n\n");
}

PCB *PCB::GetChildPCB(int childPID){
	PCB *childPCB = (PCB *)childrenPCB.Search(childPID);
	return childPCB;
}


int PCB::FindPOFByName(char *filename) {
	for (int i = 0; i < POF_MAXNUM; ++i)
		if (pofMap.Test(i) && strcmp(filename, pofArray[i]->filename) == 0) 
			return i;
	return -1;

}

int PCB::GetNumFreePOF() { 
	return pofMap.NumClear(); 
}

int PCB::CreatePOF(int sofIndex, char *filename) {
	int pofIndex = pofMap.Find();
	if (pofIndex == -1) {
		DEBUG('s',"PCB AddSOFIndex: Not enough POF space\n");
		return -1;
	}
	POF *newPOF = new POF(sofIndex, filename);
	pofArray[pofIndex] = newPOF;
	return pofIndex;
}

void PCB::PrintPOF(){
	DEBUG('s', "PCB PrintPOF:\n");
	for(int i = 0; i < POF_MAXNUM; ++i) {
		if (pofMap.Test(i))
			DEBUG('s',"POF %d: Filename %s, SOF Index %d, offset %d\t", 
					i, pofArray[i]->filename, pofArray[i]->sofIndex, pofArray[i]->offset);
	}
	DEBUG('s',"\n\n");
}

void PCB::RemovePOF(int pofIndex) {
	if (pofMap.Test(pofIndex)) {
		pofMap.Clear(pofIndex);
		delete pofArray[pofIndex];
		DEBUG('s',"PCB RemovePOF %d\n", pofIndex);
	}
	else
		DEBUG('s',"PCB RemovePOF: Error POFIndex %d\n", pofIndex);
}

bool PCB::TestPOF(int pofIndex) { 
	return pofMap.Test(pofIndex); 
}

void PCB::SetPOFoffset(int pofIndex, int of) { 
	pofArray[pofIndex]->offset = of; 
}

int PCB::GetPOFoffset(int pofIndex) { 
	return pofArray[pofIndex]->offset; 
}

int PCB::GetPOFsofIndex(int pofIndex) {
	return pofArray[pofIndex]->sofIndex; 
}

#include "pcb.h"

PCB::PCB(unsigned int inputPID) : 
		PID(inputPID), thisThread(NULL), parentPCB(NULL), 
		childrenPCB(), childExitValue(0) {
	parentPCB = 0;
}

PCB::~PCB() {
	
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

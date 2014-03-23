#include "processmgr.h"

ProcessMgr::ProcessMgr(int size):pidMap(size), pidLock("PID LOCK") {
	NumTotalPID = size;
	allPCB = new PCB*[size];
}

ProcessMgr::~ProcessMgr() {
	delete [] allPCB;
}

PCB *ProcessMgr::CreatePCB() {
	int numFreePID, newPID;
	PCB *newPCB;

	pidLock.Acquire();
	numFreePID = pidMap.NumClear();
	DEBUG('s',"ProcessMgr Free PCB %d\n", numFreePID);
	pidLock.Release();
	if (numFreePID == 0) {
		DEBUG('s', "ProcessMgr No Free PID numbers.\n");
		return NULL;
	}
	else {
		pidLock.Acquire();
		newPID = pidMap.Find();
		newPCB = new PCB(newPID);
		allPCB[newPID] = newPCB;
		pidLock.Release();
	}
	DEBUG('s',"ProcessMgr Create PCB [%d]\n", newPCB->PID);
	DEBUG('s',"\n");
	return newPCB;
}

void ProcessMgr::RemovePCB(PCB *inputPCB) {
	DEBUG('s',"ProcessMgr Remove PCB [%d]\n", inputPCB->PID);
	pidLock.Acquire();
	int pid = inputPCB->PID;
	pidMap.Clear(pid);
	allPCB[pid] = NULL;
	pidLock.Release();
	DEBUG('s',"\n");
}


int ProcessMgr::GetNumFreePCBs(){
	pidLock.Acquire();
	int numClear = pidMap.NumClear();
	pidLock.Release();
	return numClear;
}

bool ProcessMgr::IsSet(int inputPID) {
	pidLock.Acquire();
	bool t = pidMap.Test(inputPID);
	pidLock.Release();
	return t;
}

PCB *ProcessMgr::GetPCB(int inputPID) {
	pidLock.Acquire();
	if (pidMap.Test(inputPID)) {
		PCB *pcb = allPCB[inputPID];
		DEBUG('s', "ProcessMgr find PCB PID[%d]\n", inputPID);
		pidLock.Release();
		return pcb;
	}
	else {
		DEBUG('s', "ProcessMgr Not find PCB PID [%d]\n", inputPID);
		pidLock.Release();
		return NULL;
	}

}

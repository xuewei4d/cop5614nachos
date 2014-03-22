#include "processmgr.h"

ProcessMgr::ProcessMgr():pidMap(PIDMAP_SIZE), pidLock("PID LOCK") {
}

ProcessMgr::~ProcessMgr() {

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
	pidLock.Release();
	DEBUG('s',"\n");
}


int ProcessMgr::GetNumFreePCBs(){
	pidLock.Acquire();
	int numClear = pidMap.NumClear();
	pidLock.Release();
	return numClear;
}

#include "memorymgr.h"
#include "machine.h"
#include "system.h"

MemoryMgr::MemoryMgr(): 
  memoryMap(NumPhysPages), memoryLock("Memory Lock") {
	timestamp = 0;
	for (int i = 0; i < NumPhysPages; ++i) {
		frametime[i] = -1;
		for (int j = 0; j < PIDMAP_SIZE; ++j)
			framePID[i][j] = FALSE;
	}
}

MemoryMgr::~MemoryMgr() {

}

int MemoryMgr::GetPage(int pid, int sharedPage) {
	memoryLock.Acquire();
	int pageNumber;
	unsigned int numClearPages = memoryMap.NumClear();
	if (numClearPages == 0) {
		// page out
		int timemin = frametime[0];
		int outpage = 0;
		int i;
		for (i = 0; i < NumPhysPages; ++i)  {
			if (timemin > frametime[i] && i != sharedPage) { // skip the shared page for COW
				timemin = frametime[i];
				outpage = i;
			}
		}
		if (i == NumPhysPages)
			return -1; // No page to be paged out.

		for (int outPID = 0; outPID <= PIDMAP_SIZE; ++outPID) {
			if (framePID[outpage][outPID]) {
				processMgr->allPCB[outPID]->thisAddrSpace->PageOut(outpage);
				framePID[outpage][outPID] = FALSE;
			}
		}

		pageNumber = outpage;
		DEBUG('s', "MemoryMgr PageOut page %d\n", pageNumber);
	}
	else {
		pageNumber = memoryMap.Find();
		DEBUG('s', "MemoryMgr New page %d\n", pageNumber);
	}

	frametime[pageNumber] = timestamp;
	timestamp ++;
	framePID[pageNumber][pid] = TRUE;
   
	memoryLock.Release();
	return pageNumber;
  
}

void MemoryMgr::ClearPage(int pageNumber) {
  memoryLock.Acquire();
  memoryMap.Clear(pageNumber);
  memoryLock.Release();
}

unsigned int MemoryMgr::GetNumFreePages() {
  memoryLock.Acquire();
  unsigned int numClearPages = memoryMap.NumClear();
  memoryLock.Release();
  return numClearPages;
}

void MemoryMgr::UnsetShare(int physicalPage, int PID) {
	framePID[physicalPage][PID] = FALSE;
	DEBUG('s', "MemoryMgr UnsetShare: PhysicalPage %d, PID %d\n",
			physicalPage, PID);

	int count = 0;
	int pid = 0;

	// find the single PID
	for (int i = 0; i < PIDMAP_SIZE; ++i)  {
		if (framePID[physicalPage][i]) {
			pid = i;
			count ++;
		}
	}
	
	if (count == 1) {
		DEBUG('s', "MemoryMgr UnsetReadOnly: PID %d, PhysicalPage %d\n", 
				pid, physicalPage);
		processMgr->allPCB[pid]->thisAddrSpace->UnsetReadOnly(physicalPage);
	}

}

void MemoryMgr::SetShare(int physicalPage, int PID) {
	memoryLock.Acquire();
	framePID[physicalPage][PID] = TRUE;
	memoryLock.Release();
}

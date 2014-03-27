#include "memorymgr.h"
#include "machine.h"

MemoryMgr::MemoryMgr(): 
  memoryMap(NumPhysPages), memoryLock("Memory Lock") {
}

MemoryMgr::~MemoryMgr() {

}

int MemoryMgr::GetPage() {
  memoryLock.Acquire();
  int pageNumber = memoryMap.Find();
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

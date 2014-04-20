// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "pcb.h"

#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
  AddrSpace(OpenFile *executable, PCB *newPCB);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 

    void PrintPageTable();
    void ReleaseMemory();
    AddrSpace();
    AddrSpace* Fork(PCB *newPCB);
    void SaveUserRegisters();
    void RestoreUserRegisters();
    void PrintUserRegisters();
    void userReadWrite(char *buffer, int virtAddr, int size, char opt);
    bool ReadString(int stringAddr, char *stringBuffer, int size);
    bool Translate(int virtAddr, int * physAddr, int size);
    void ReadFile(OpenFile* file, int virtAddr, int size, int fileAddr);
    bool ReplaceMemory(OpenFile *executable);
    unsigned int getNumPages() { return numPages;}
    void AllocateSwapFile(OpenFile *openFile);
    void PageIn(int virtAddr);
    void PageOut(int physicalPage);
    void COW(int virtAddr);
    void UnsetReadOnly(int physicalPage);

    PCB *thisPCB;
    int userRegisters[NumTotalRegs];

    unsigned int CodeDataSize;
    int CodeDataVirtAddr;
    int CodeDataFileAddr;
    OpenFile *swapfile;
    unsigned int SwapFileSize;
  
  private:
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
};

#endif // ADDRSPACE_H

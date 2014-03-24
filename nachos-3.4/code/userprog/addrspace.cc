// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= memoryMgr->GetNumFreePages());		// CHANGED check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('s', "AddrSpace Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = memoryMgr->GetPage();  // CHANGED
		DEBUG('s',"Create New Page %d\t", pageTable[i].physicalPage);
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
		                                // a separate page, we could set its 
                                		// pages to be read-only
		// CHANGED
		bzero(machine->mainMemory + pageTable[i].physicalPage*PageSize, PageSize);
    }
	DEBUG('s',"\n\n");
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
//    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('s', "AddrSpace Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
//        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
//			noffH.code.size, noffH.code.inFileAddr);

		ReadFile(executable, noffH.code.virtualAddr, noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('s', "AddrSpace Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
//        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
//			noffH.initData.size, noffH.initData.inFileAddr);
		ReadFile(executable, noffH.initData.virtualAddr, noffH.initData.size, noffH.initData.inFileAddr);
    }

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('s', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

void AddrSpace::PrintPageTable() {
	DEBUG('s',"PID [%d] AddrSpace Print PageTable\n", thisPCB->PID);
	for (unsigned int i = 0; i < numPages; ++i) {
		DEBUG('s',"PageTable %d: Physical Page %d\t", i, pageTable[i].physicalPage);
	}
	DEBUG('s',"\n\n");
}

void AddrSpace::ReleaseMemory() {
	DEBUG('s', "PID [%d] AddrSpace Release %d pages\n", thisPCB->PID, numPages);
	for (unsigned int i = 0; i < numPages; ++i) {
		DEBUG('s', "Clear page %d\t", 
				pageTable[i].physicalPage);
		memoryMgr->ClearPage(pageTable[i].physicalPage);
	}
	DEBUG('s',"\n\n");
}

AddrSpace::AddrSpace() {

}

AddrSpace* AddrSpace::Fork() {
	DEBUG('s',"PID [%d] AddrSpace Fork\n", thisPCB->PID);
	if (numPages > memoryMgr->GetNumFreePages()) {
	    DEBUG('s', "AddrSpace Not Enough Space!\n");
		DEBUG('s', "ProcessMgr has %d pages\n", memoryMgr->GetNumFreePages());
		return NULL;
	}
	
	AddrSpace * newAddrSpace = new AddrSpace();
	newAddrSpace->numPages = numPages;
	newAddrSpace->pageTable = new TranslationEntry[numPages];
	for (unsigned int i = 0; i < numPages; i++) {
		newAddrSpace->pageTable[i].virtualPage = pageTable[i].virtualPage;
		newAddrSpace->pageTable[i].physicalPage = memoryMgr->GetPage();    // allocate memory space
		DEBUG('s', "Fork New Page %d\t", 
				newAddrSpace->pageTable[i].physicalPage);
		newAddrSpace->pageTable[i].valid = pageTable[i].valid;
		newAddrSpace->pageTable[i].use = pageTable[i].use;
		newAddrSpace->pageTable[i].dirty = pageTable[i].dirty;
		newAddrSpace->pageTable[i].readOnly = pageTable[i].readOnly;
		bcopy(machine->mainMemory + pageTable[i].physicalPage*PageSize, 
				machine->mainMemory + newAddrSpace->pageTable[i].physicalPage*PageSize, PageSize);
	}
	DEBUG('s',"\n\n");
	return newAddrSpace;
}

void AddrSpace::SaveUserRegisters(){
	DEBUG('s',"PID [%d] AddrSpace Save Registers\n", thisPCB->PID);
	for (int i = 0; i < NumTotalRegs; ++i) {
		userRegisters[i] = machine->ReadRegister(i);
	}
	DEBUG('s',"\n");
}

void AddrSpace::RestoreUserRegisters() {
	DEBUG('s',"PID [%d] AddrSpace Restore Registers\n", thisPCB->PID);
	for (int i = 0; i < NumTotalRegs; ++i)
		machine->WriteRegister(i, userRegisters[i]);
	DEBUG('s',"\n");
}

void AddrSpace::PrintUserRegisters() {
	DEBUG('s',"PID [%d] AddrSpace Print Registers\n", thisPCB->PID);
	for (int i = 0; i < NumTotalRegs; ++i)
		DEBUG('s', "%d: %d\t", i, userRegisters[i]);
	DEBUG('s',"\n\n");
}

bool AddrSpace::Translate(int virtAddr, int * physAddr, int size) {
	DEBUG('s',"AddrSpace Translate 0x%x, size %d = ", virtAddr, size);
	unsigned int vpn, offset, pageFrame;
	if (virtAddr < 0) {
		DEBUG('s',"AddrSpace Translate virtAddr < 0\n");
		return false;
	}
	vpn = (unsigned)virtAddr / PageSize;
	offset = (unsigned)virtAddr % PageSize;
	if (vpn >= numPages) {
		DEBUG('s', "AddrSpace Translate virtual page %d too large for numPages %d!\n",
				virtAddr, numPages);
		return false;
	}
	else if(!pageTable[vpn].valid) {
		DEBUG('s',"AddrSpace Translate virtual page %d not valid!\n",
				virtAddr);
		return false;
	}
	pageFrame = pageTable[vpn].physicalPage;
	if (pageFrame >= NumPhysPages) {
		DEBUG('s', "AddrSpace Translate Frame > NumPhysPages!\n", pageFrame);
		return false;
	}
	pageTable[vpn].use = TRUE;
	*physAddr = pageFrame * PageSize + offset;
	ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
	DEBUG('s', "Phys Addr 0x%x\n", *physAddr);
	return true;
}


int AddrSpace::ReadFile(OpenFile* file, int virtAddr, 
		int size, int fileAddr) {
	DEBUG('s', "AddrSpace ReadFile\n");

	int physAddr = 0;

	int curVirtAddr = virtAddr;
	int curBuffer = 0;
	int leftSize = size;
	int copySize = 0;
	char *buffer = new char[size];
	file->ReadAt(buffer, size, fileAddr);

	if (virtAddr % PageSize != 0) {
		copySize = min(size, PageSize - virtAddr % PageSize);
		if (Translate(curVirtAddr, &physAddr, copySize)) {
			DEBUG('s',"AddrSpace ReadFile Head Partial Copy: virtAddr 0x%x, physAddr 0x%x, size %d\n", 
					curVirtAddr, physAddr, copySize);
			bcopy(buffer + curBuffer, machine->mainMemory + physAddr, copySize);
			leftSize -= copySize;
			curBuffer += copySize;
			curVirtAddr += copySize;
		}
		else
			DEBUG('s',"AddrSpace ReadFile Head Partial Copy Error: virtAddr 0x%x\n", curVirtAddr);
	}

	while (leftSize >= PageSize) {
		if (Translate(curVirtAddr, &physAddr, PageSize)) {
			DEBUG('s',"AddrSpace ReadFile Whole Page Copy: virtAddr 0x%x, physAddr 0x%x, size %d\n", 
					curVirtAddr, physAddr, PageSize);
			bcopy(buffer + curBuffer, machine->mainMemory + physAddr, PageSize);
			leftSize -= PageSize;
			curBuffer += PageSize;
			curVirtAddr += PageSize;
		}
		else
			DEBUG('s',"AddrSpace ReadFile Whole Page Copy Error: virtAddr 0x%x\n", curVirtAddr);
	}
	
	if (leftSize != 0) {
		if (Translate(curVirtAddr, &physAddr, leftSize)) {
			DEBUG('s',"AddrSpace ReadFile Last Partial Page Copy: virtAddr 0x%x, physAddr 0x%x, size %d\n", 
					curVirtAddr, physAddr, leftSize);
			bcopy(buffer + curBuffer, machine->mainMemory + physAddr, leftSize);
			curBuffer += leftSize;
			curVirtAddr += leftSize;
		}
		else
			DEBUG('s', "AddrSpace ReadFile Last Partial Page Copy Error: vitAddr 0x%x\n", curVirtAddr);
	}
	DEBUG('s', "AddrSpace ReadFile curBuffer %d\n", curBuffer);
	ASSERT(curBuffer == size);
	delete []buffer;
}

bool AddrSpace::ReplaceMemory(OpenFile *executable) {
	NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
			(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    unsigned int newNumPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

	DEBUG('s', "PID [%d] AddressSpace ReplaceMemory: Require %d pages, Free %d pages, Old %d pages\n",
			thisPCB->PID, newNumPages, memoryMgr->GetNumFreePages(), numPages);
	
	if (int(newNumPages) - int(numPages) > int(memoryMgr->GetNumFreePages())) {
		DEBUG('s', "PID [%d] AddressSpace ReplaceMemory: Not enough free pages\n",
				thisPCB->PID);
		return false;
	}

	ReleaseMemory();
	delete pageTable;

	pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = memoryMgr->GetPage();  // CHANGED
		DEBUG('s',"Create New Page %d\t", pageTable[i].physicalPage);
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
		                                // a separate page, we could set its 
                                		// pages to be read-only
		bzero(machine->mainMemory + pageTable[i].physicalPage*PageSize, PageSize);
    }
	DEBUG('s',"\n\n");
    
    if (noffH.code.size > 0) {
        DEBUG('s', "AddrSpace Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
		ReadFile(executable, noffH.code.virtualAddr, noffH.code.size, noffH.code.inFileAddr);
    }

    if (noffH.initData.size > 0) {
        DEBUG('s', "AddrSpace Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
		ReadFile(executable, noffH.initData.virtualAddr, noffH.initData.size, noffH.initData.inFileAddr);
    }
	return true;
}

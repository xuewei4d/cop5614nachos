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

AddrSpace::AddrSpace(OpenFile *executable, PCB *newPCB)
{
	thisPCB = newPCB;
	thisPCB->thisAddrSpace = this;

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

	CodeDataSize = noffH.code.size + noffH.initData.size;
	CodeDataVirtAddr = noffH.code.virtualAddr;
	CodeDataFileAddr = noffH.code.inFileAddr;
	SwapFileSize = size;
	AllocateSwapFile(executable);

    // ASSERT(numPages <= memoryMgr->GetNumFreePages());		// CHANGED check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('s', "PID [%d] AddrSpace Initializing address space, num pages %d, size %d\n", thisPCB->PID,
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = -1; // CHANGED for project 4
		pageTable[i].valid = FALSE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
		                                // a separate page, we could set its 
                                		// pages to be read-only
		pageTable[i].zeroFilled = FALSE;

		// For project before Project 4
        // bzero(machine->mainMemory + pageTable[i].physicalPage*PageSize, PageSize);
    }
	DEBUG('s', "PID [%d] AddrSpace code virtualAddr 0x%x, inFileAddr 0x%x\n", thisPCB->PID, noffH.code.virtualAddr, noffH.code.inFileAddr);
	DEBUG('s', "PID [%d] AddrSpace data virtualAddr 0x%x, inFileAddr 0x%x\n", thisPCB->PID, noffH.initData.virtualAddr, noffH.initData.inFileAddr);
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
//    bzero(machine->mainMemory, size);
// CHANGED for project 4
// then, copy in the code and data segments into memory
//     if (noffH.code.size > 0) {
//         DEBUG('s', "PID [%d] AddrSpace Initializing code segment, at 0x%x, size %d\n", thisPCB->PID,
// 			noffH.code.virtualAddr, noffH.code.size);
// //        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
// //			noffH.code.size, noffH.code.inFileAddr);

// 		ReadFile(executable, noffH.code.virtualAddr, noffH.code.size, noffH.code.inFileAddr);
//     }
//     if (noffH.initData.size > 0) {
//         DEBUG('s', "PID [%d] AddrSpace Initializing data segment, at 0x%x, size %d\n", thisPCB->PID,
// 			noffH.initData.virtualAddr, noffH.initData.size);
// //        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
// //			noffH.initData.size, noffH.initData.inFileAddr);
// 		ReadFile(executable, noffH.initData.virtualAddr, noffH.initData.size, noffH.initData.inFileAddr);
//     }
// 	printf("Loaded Program: [%d] code |  [%d] data | [%d] bss\n", noffH.code.size, 
// 			noffH.initData.size, noffH.uninitData.size);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	delete swapfile;
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
    DEBUG('s', "PID [%d] Initializing stack register to %d\n", thisPCB->PID, numPages * PageSize - 16);
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
		if (pageTable[i].physicalPage != -1 && !pageTable[i].readOnly) {
			memoryMgr->ClearPage(pageTable[i].physicalPage);
			DEBUG('s', "ClearPage %d\t", pageTable[i].physicalPage);
		}
		else if (pageTable[i].physicalPage != -1 && pageTable[i].readOnly) {
			memoryMgr->UnsetShare(pageTable[i].physicalPage, thisPCB->PID);
			DEBUG('s', "UnSharePage %d\t", pageTable[i].physicalPage);
		}
		else
			DEBUG('s', "SkipPage %d\t", pageTable[i].physicalPage);
	}
	DEBUG('s',"\n\n");
}

AddrSpace::AddrSpace() {

}

// CHANGED for project 4
AddrSpace* AddrSpace::Fork(PCB *newPCB) {
	DEBUG('s',"PID [%d] AddrSpace Fork\n", thisPCB->PID);
	if (numPages > memoryMgr->GetNumFreePages()) {
	    printf("AddrSpace Not Enough Space!\n");
		printf("ProcessMgr has %d pages\n", memoryMgr->GetNumFreePages());
		return NULL;
	}
	
	AddrSpace * newAddrSpace = new AddrSpace();
	newAddrSpace->thisPCB = newPCB;
	newPCB->thisAddrSpace = newAddrSpace;

	newAddrSpace->numPages = numPages;
	newAddrSpace->pageTable = new TranslationEntry[numPages];
	for (unsigned int i = 0; i < numPages; i++) {
		newAddrSpace->pageTable[i].virtualPage = pageTable[i].virtualPage;

		// COW implementation
//		newAddrSpace->pageTable[i].physicalPage = memoryMgr->GetPage(newPCB->PID);    // allocate memory space
		newAddrSpace->pageTable[i].physicalPage = pageTable[i].physicalPage;    // share pages
		memoryMgr->SetShare(pageTable[i].physicalPage, newAddrSpace->thisPCB->PID);
		DEBUG('s', "Fork New COW Page %d\t", 
				newAddrSpace->pageTable[i].physicalPage);
		newAddrSpace->pageTable[i].valid = pageTable[i].valid;
		newAddrSpace->pageTable[i].use = pageTable[i].use;
		newAddrSpace->pageTable[i].dirty = pageTable[i].dirty;
		
		if (newAddrSpace->pageTable[i].physicalPage != -1) {
			newAddrSpace->pageTable[i].readOnly = TRUE;
			pageTable[i].readOnly = TRUE;
			
		}
		else
			newAddrSpace->pageTable[i].readOnly = pageTable[i].readOnly;

		newAddrSpace->pageTable[i].zeroFilled = pageTable[i].zeroFilled;

// For projects before project 4
//		bcopy(machine->mainMemory + pageTable[i].physicalPage*PageSize, 
//				machine->mainMemory + newAddrSpace->pageTable[i].physicalPage*PageSize, PageSize);

	}
	
	newAddrSpace->CodeDataSize = CodeDataSize;
	newAddrSpace->CodeDataVirtAddr = CodeDataVirtAddr;
	newAddrSpace->CodeDataFileAddr = CodeDataFileAddr;
	newAddrSpace->SwapFileSize = SwapFileSize;
	newAddrSpace->AllocateSwapFile(swapfile);

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
//	DEBUG('s',"AddrSpace Translate 0x%x, size %d = ", virtAddr, size);
	unsigned int vpn, offset, pageFrame;
	if (virtAddr < 0) {
		printf("AddrSpace Translate virtAddr < 0\n");
		return false;
	}
	vpn = (unsigned)virtAddr / PageSize;
	offset = (unsigned)virtAddr % PageSize;
	if (vpn >= numPages) {
		printf("AddrSpace Translate virtual page %d too large for numPages %d!\n",
				virtAddr, numPages);
		return false;
	}
	else if(!pageTable[vpn].valid) {
		printf("AddrSpace Translate virtual page %d not valid!\n",
				virtAddr);
		return false;
	}
	pageFrame = pageTable[vpn].physicalPage;
	if (pageFrame >= NumPhysPages) {
		printf("AddrSpace Translate Frame > NumPhysPages!\n");
		return false;
	}
	pageTable[vpn].use = TRUE;
	*physAddr = pageFrame * PageSize + offset;
	ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
//	DEBUG('s', "Phys Addr 0x%x\n", *physAddr);
	return true;
}


bool AddrSpace::ReadString(int stringAddr, char *stringBuffer, int size) {
	char c;
	int physAddr;
	int curBuffer = 0;
	int virtAddr = stringAddr;
	do{
		if (Translate(virtAddr, &physAddr, 1)) {
			bcopy(machine->mainMemory + physAddr, &c, 1);
			stringBuffer[curBuffer] = c;
			curBuffer ++;
			virtAddr ++;
		}
		else {
			printf("PID [%d] AddrSpace ReadString: Translate Error\n", thisPCB->PID);
			return false;
		}
		
	}while(c != '\0' && curBuffer < size);
	
	DEBUG('s',"PID [%d] AddrSpace ReadString: Reads %s\n", thisPCB->PID, stringBuffer);
	return true;
	
}

void AddrSpace::userReadWrite(char *buffer, int virtAddr, int size, char opt){
	int physAddr = 0;
	int curVirtAddr = virtAddr;
	int curBuffer = 0;
	int leftSize = size;
	int copySize = 0;

	if (virtAddr % PageSize != 0) {
		copySize = min(size, PageSize - virtAddr % PageSize);
		if (Translate(curVirtAddr, &physAddr, copySize)) {
//			DEBUG('s',"TPRW  Head Partial Copy: virtAddr 0x%x, physAddr 0x%x, size %d\n", 
//					curVirtAddr, physAddr, copySize);
			if (opt == 'r') {
				bzero(machine->mainMemory + physAddr, copySize);
				bcopy(buffer + curBuffer, machine->mainMemory + physAddr, copySize);
			}
			else {
				bzero(buffer + curBuffer, copySize);
				bcopy(machine->mainMemory + physAddr, buffer + curBuffer, copySize);
			}
			leftSize -= copySize;
			curBuffer += copySize;
			curVirtAddr += copySize;
		}
		else
			printf("PID [%d] TPRW Head Partial Copy Error: virtAddr 0x%x\n", thisPCB->PID, curVirtAddr);
	}

	while (leftSize >= PageSize) {
		if (Translate(curVirtAddr, &physAddr, PageSize)) {
//			DEBUG('s',"TPRW Whole Page Copy: virtAddr 0x%x, physAddr 0x%x, size %d\n", 
//					curVirtAddr, physAddr, PageSize);
			if (opt == 'r') {
				bzero(machine->mainMemory + physAddr, PageSize);
				bcopy(buffer + curBuffer, machine->mainMemory + physAddr, PageSize);
			}
			else {
				bzero(buffer + curBuffer, PageSize);
				bcopy(machine->mainMemory + physAddr, buffer + curBuffer, PageSize);
			}
			leftSize -= PageSize;
			curBuffer += PageSize;
			curVirtAddr += PageSize;
		}
		else
			printf("PID [%d] TPRW Whole Page Copy Error: virtAddr 0x%x\n", thisPCB->PID, curVirtAddr);
	}
	
	if (leftSize != 0) {
		if (Translate(curVirtAddr, &physAddr, leftSize)) {
//			DEBUG('s',"TPRW Last Partial Page Copy: virtAddr 0x%x, physAddr 0x%x, size %d\n", 
//					curVirtAddr, physAddr, leftSize);
			if (opt == 'r') {
				bzero(machine->mainMemory + physAddr, leftSize);
				bcopy(buffer + curBuffer, machine->mainMemory + physAddr, leftSize);
			}
			else {
				bzero(buffer + curBuffer, leftSize);
				bcopy(machine->mainMemory + physAddr, buffer + curBuffer, leftSize);
			}
			curBuffer += leftSize;
			curVirtAddr += leftSize;
		}
		else
			printf("PID [%d] TPRW Last Partial Page Copy Error: vitAddr 0x%x\n", thisPCB->PID, curVirtAddr);
	}
	ASSERT(curBuffer == size);
}

void AddrSpace::ReadFile(OpenFile* file, int virtAddr, 
		int size, int fileAddr) {
	DEBUG('s', "PID [%d] AddrSpace ReadFile\n", thisPCB->PID);
	char *buffer = new char[size];
	file->ReadAt(buffer, size, fileAddr);
	userReadWrite(buffer, virtAddr, size, 'r');
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

	CodeDataSize = noffH.code.size + noffH.initData.size;
	CodeDataVirtAddr = noffH.code.virtualAddr;
	CodeDataFileAddr = noffH.code.inFileAddr;
	SwapFileSize = size;
	AllocateSwapFile(executable);

	DEBUG('s', "PID [%d] AddressSpace ReplaceMemory: Require %d pages, Free %d pages, Old %d pages\n",
			thisPCB->PID, newNumPages, memoryMgr->GetNumFreePages(), numPages);
	
// 	if (int(newNumPages) - int(numPages) > int(memoryMgr->GetNumFreePages())) {
// 		DEBUG('s', "PID [%d] AddressSpace ReplaceMemory: Not enough free pages\n",
// 				thisPCB->PID);
// 		return false;
// 	}

	ReleaseMemory();
	delete pageTable;

	pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
//		pageTable[i].physicalPage = memoryMgr->GetPage(thisPCB->PID);  // CHANGED

		pageTable[i].physicalPage = -1;
		DEBUG('s',"PID [%d] Create New Page %d\t", thisPCB->PID, pageTable[i].physicalPage);
		pageTable[i].valid = FALSE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
		                                // a separate page, we could set its 
                                		// pages to be read-only
//		bzero(machine->mainMemory + pageTable[i].physicalPage*PageSize, PageSize);
		
    }
	DEBUG('s',"\n\n");
	
//     if (noffH.code.size > 0) {
//         DEBUG('s', "PID [%d] AddrSpace Initializing code segment, at 0x%x, size %d\n", thisPCB->PID,
// 			noffH.code.virtualAddr, noffH.code.size);
// 		ReadFile(executable, noffH.code.virtualAddr, noffH.code.size, noffH.code.inFileAddr);
//     }

//     if (noffH.initData.size > 0) {
//         DEBUG('s', "PID [%d] AddrSpace Initializing data segment, at 0x%x, size %d\n", thisPCB->PID,
// 			noffH.initData.virtualAddr, noffH.initData.size);
// 		ReadFile(executable, noffH.initData.virtualAddr, noffH.initData.size, noffH.initData.inFileAddr);
//     }
	printf("Loaded Program: [%d] code |  [%d] data | [%d] bss\n", noffH.code.size, 
			noffH.initData.size, noffH.uninitData.size);
	return true;
}

void AddrSpace::AllocateSwapFile(OpenFile *openFile) {
	char swapfilename[128];
	sprintf(swapfilename, "./test/");
	char pidchar[16];
	sprintf(pidchar, "%d.swap", thisPCB->PID);
	strcat(swapfilename, pidchar);
	DEBUG('s',"PID [%d] AddrSpace AllocateSwapFile: swap file %s\n", thisPCB->PID, swapfilename);

	fileSystem->Create(swapfilename, 0);
	swapfile = fileSystem->Open(swapfilename);
	DEBUG('s', "PID [%d] AddrSpace AllocateSwapFile: 0x%x\n", thisPCB->PID, swapfile);

	// bruteforce
	char *buffer = new char[SwapFileSize];
	bzero(buffer, SwapFileSize);
	int rcount = openFile->ReadAt(buffer, CodeDataSize, CodeDataFileAddr);
	int wrount = swapfile->WriteAt(buffer, SwapFileSize, 0);

	DEBUG('s', "PID [%d] AddrSpace AllocateSwapFile: CodeDataSize %d, SwapFileSize %d, ReadCount %d, WriteCount %d\n", thisPCB->PID, 
			CodeDataSize, SwapFileSize, rcount, wrount);
	delete []buffer;

}

void AddrSpace::PageIn(int virtAddr) {
	unsigned int vpn;
	vpn = (unsigned) virtAddr / PageSize;
	
	// allocate page
	int ppn = memoryMgr->GetPage(thisPCB->PID);
	if (ppn != -1) {
		pageTable[vpn].virtualPage = vpn;
		pageTable[vpn].physicalPage = ppn;
		pageTable[vpn].valid = TRUE;
		pageTable[vpn].readOnly = FALSE;
		
		// zero-filled?
 		if (virtAddr >= (CodeDataSize + CodeDataVirtAddr) && 
 				!pageTable[vpn].zeroFilled) {
 			bzero(machine->mainMemory + ppn*PageSize, PageSize);
 			pageTable[vpn].zeroFilled = TRUE;
 			DEBUG('s', "PID [%d] AddrSpace PageIn: zero-filled VPN %d\n", thisPCB->PID, vpn);
 			printf("Z [%d]: [%d]\n", thisPCB->PID, vpn);
 		}
 		else {
			int pageStart = vpn*PageSize;
			ReadFile(swapfile, pageStart, PageSize, pageStart);
			DEBUG('s', "PID [%d] AddrSpace PageIn: VPN %d to PPN %d\n", thisPCB->PID, vpn, ppn);
			printf("L [%d]: [%d] -> [%d]\n", thisPCB->PID, vpn, ppn);
		}

	}
	else {
		printf("PID [%d] AddrSpace PageIn: fail to get new frame\n", thisPCB->PID);
	}
	DEBUG('s',"\n");
}

void AddrSpace::PageOut(int physicalPage) {
	unsigned int virtPage = 0;
	for(virtPage = 0; virtPage < numPages; ++virtPage) 
		if (pageTable[virtPage].physicalPage == physicalPage)
			break;
	if (pageTable[virtPage].dirty) {
		swapfile->WriteAt(machine->mainMemory + physicalPage*PageSize, PageSize, virtPage*PageSize);
		DEBUG('s',"PID [%d] AddrSpace PageOut: dirty virt %d, physical %d\n", thisPCB->PID, virtPage, physicalPage);
		printf("S [%d]: [%d]\n", thisPCB->PID, physicalPage);
	}
	else {
		DEBUG('s',"PID [%d] AddrSpace PageOut: not dirty virt %d\n", thisPCB->PID, virtPage);
		printf("E [%d]: [%d]\n", thisPCB->PID, physicalPage);
	}
	pageTable[virtPage].physicalPage = -1;
	pageTable[virtPage].valid = FALSE;
	pageTable[virtPage].readOnly = FALSE;
}

void AddrSpace::COW(int virtAddr) {
	unsigned int vpn, ppn;
	vpn = (unsigned) virtAddr / PageSize;
	ppn = pageTable[vpn].physicalPage;
	ppn = memoryMgr->GetPage(thisPCB->PID, ppn);
	if (ppn == -1) {
		printf("PID [%d] AddrSpace COW: cannot find new free frame\n", thisPCB->PID);
	}
	// copy
	int oppn = pageTable[vpn].physicalPage;
	pageTable[vpn].physicalPage = ppn;
	pageTable[vpn].readOnly = FALSE;
	
	bcopy(machine->mainMemory + oppn*PageSize, machine->mainMemory + ppn*PageSize, PageSize);

	memoryMgr->UnsetShare(oppn, thisPCB->PID);
	DEBUG('s',"PID [%d] AddrSpace COW: Old PPN %d, New PPN %d, VPN %d\n", thisPCB->PID, oppn, ppn, vpn);
	printf("D [%d]: [%d]\n", thisPCB->PID, vpn);
}

void AddrSpace::UnsetReadOnly(int physicalPage) {
	int virtPage = 0;
	for (virtPage = 0; virtPage < numPages; ++virtPage) {
		if (pageTable[virtPage].physicalPage == physicalPage) {
			pageTable[virtPage].readOnly = FALSE;
			break;
		}
	}
	DEBUG('s', "PID [%d] UnsetReadOnly: virtPage %d, PhysicalPage %d\n", 
			thisPCB->PID, virtPage, physicalPage);
}

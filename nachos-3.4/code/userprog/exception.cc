// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void PrintMachineRegisters() {
	DEBUG('s',"Machine Registers: \n");
	for (int i = 0; i < NumTotalRegs; ++i)
		DEBUG('s',"%d: %d\t", i, machine->ReadRegister(i));
	DEBUG('s',"\n\n");
}

void ExitSystemcall() {
	AddrSpace *currentAddrSpace = currentThread->space;
	PCB *currentPCB = currentAddrSpace->thisPCB;
	int val = machine->ReadRegister(4);
	
	printf("System Call: [%d] invoked Exit\n", currentPCB->PID);
	printf("Process [%d] exited with status [%d]\n", currentPCB->PID, val);
	DEBUG('s',"Systemcall Exit: PID [%d], Value %d\n", 
			currentAddrSpace->thisPCB->PID, val);
	PrintMachineRegisters();
	
	currentPCB->RemoveChildParentPCB();
	currentPCB->RemoveParentPCBChild(val);
	processMgr->RemovePCB(currentPCB);
	DEBUG('s',"Systemcall Exit: ProcessMgr has %d PIDs\n\n", processMgr->GetNumFreePCBs());
	
	currentAddrSpace->PrintPageTable();
	currentAddrSpace->ReleaseMemory();
	DEBUG('s',"Systemcall Exit: MemoryMgr has %d pages\n\n", memoryMgr->GetNumFreePages());
	delete currentAddrSpace;
	delete currentPCB;
	currentThread->Finish();
}

void UpdatePCRegs() {
	DEBUG('s',"Update PCRegs\n");
	DEBUG('s',"\n");
	int pcreg = machine->ReadRegister(PCReg);
	int prevpcreg = machine->ReadRegister(PrevPCReg);
	int nextpcreg = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, pcreg + 4);
	machine->WriteRegister(PrevPCReg, prevpcreg + 4);
	machine->WriteRegister(NextPCReg, nextpcreg + 4);
}

void Dummy(int d) {
	DEBUG('s', "Dummy Function\n\n");
	currentThread->space->RestoreUserRegisters();
	currentThread->space->RestoreState();
	machine->Run();
}

void ForkSystemcall() {
	AddrSpace *currentAddrSpace = currentThread->space;
	PCB *currentPCB = currentAddrSpace->thisPCB;
	printf("System Call: [%d] invoked Fork\n", currentPCB->PID);
	DEBUG('s',"Systemcall Fork: PID [%d]\n", 
			currentPCB->PID);
	PrintMachineRegisters();

	// 1. save old process registers
	currentAddrSpace->SaveUserRegisters();
	currentAddrSpace->PrintUserRegisters();

	// 2. create a PCB and associate
	PCB *childPCB = processMgr->CreatePCB();
	if (childPCB == NULL) {
		machine->WriteRegister(2, -1);

		UpdatePCRegs();
		DEBUG('s',"Systemcall Fork: PID [%d] After Update PCRegs\n", currentPCB->PID);
		PrintMachineRegisters();
		return;
	}
	AddrSpace *childAddrSpace = currentAddrSpace->Fork();
	if (childAddrSpace == NULL) {
		machine->WriteRegister(2, -1);
		processMgr->RemovePCB(childPCB);

		UpdatePCRegs();
		DEBUG('s',"Systemcall Fork: PID [%d] After Update PCRegs\n", currentPCB->PID);
		PrintMachineRegisters();
		return;
	}
	childAddrSpace->thisPCB = childPCB;
	childAddrSpace->PrintPageTable();
	
	// 3. create a new Thread and associate
	Thread *childThread = new Thread("Forked Thread");
	
	childThread->space = childAddrSpace;
	childPCB->thisThread = childThread;
	childPCB->parentPCB = currentPCB;
	currentPCB->AppendChildPCB(childPCB);

	currentPCB->PrintPCB();
	childPCB->PrintPCB();

	// 4. 
	int childPCReg = machine->ReadRegister(4);
	printf("Process [%d] Fork: start at address [0x%x] with [%d] pages memory\n", currentPCB->PID, childPCReg, childAddrSpace->getNumPages());
	DEBUG('s',"Systemcall Fork: PID [%d] Fork Child New PCReg %d\n", 
			currentPCB->PID, childPCReg);
	PrintMachineRegisters();
	machine->WriteRegister(PCReg, childPCReg);
	machine->WriteRegister(NextPCReg, childPCReg + 4);
	machine->WriteRegister(PrevPCReg, childPCReg - 4);
	PrintMachineRegisters();
	childAddrSpace->SaveUserRegisters();
	childAddrSpace->PrintUserRegisters();
	
	// 5. Fork
	childThread->Fork(Dummy, 0);

	// 6. Restore Machine Registers
	currentAddrSpace->RestoreUserRegisters();
	
	// 7. Write Register 2
	machine->WriteRegister(2, childPCB->PID);

	UpdatePCRegs();
	DEBUG('s',"Systemcall Fork: PID [%d] After Update PCRegs\n", currentPCB->PID);
	PrintMachineRegisters();
}

bool ReadString(int stringAddr, char *stringBuffer, int size, AddrSpace *currentAddrSpace) {
	DEBUG('s', "ReadString\n");
	char c;
	int physAddr;
	int curBuffer = 0;
	int virtAddr = stringAddr;
	do{
		if (currentAddrSpace->Translate(virtAddr, &physAddr, 1)) {
			bcopy(machine->mainMemory + physAddr, &c, 1);
			stringBuffer[curBuffer] = c;
			curBuffer ++;
			virtAddr ++;
		}
		else {
			DEBUG('s', "ReadString Translate Error\n");
			return false;
		}
		
	}while(c != '\0' && curBuffer < size);
	
	DEBUG('s',"ReadString Reads %s\n", stringBuffer);
	return true;
	
}

/*
  We dont use Readmem function which is forbidden.
 */
void ExecSystemcall() {
	AddrSpace *currentAddrSpace = currentThread->space;
	PCB *currentPCB = currentAddrSpace->thisPCB;
	printf("System Call: [%d] invoked Exec\n", currentPCB->PID);
	DEBUG('s',"Systemcall Exec: PID [%d]\n", currentPCB->PID);
	PrintMachineRegisters();
	
	// 1. read path
	int filenameAddr = machine->ReadRegister(4);
	char *filename = new char[256];
	ReadString(filenameAddr, filename, 256, currentAddrSpace);
	printf("Exec Program: [%d] loading [%s]\n", currentPCB->PID, filename);
	OpenFile *executable = fileSystem->Open(filename);
	if (executable == NULL) {
		DEBUG('s', "Systemcall Exec: PID [%d] Unable to open file %s\n", currentPCB->PID, filename);
		machine->WriteRegister(2, -1);
		delete []filename;
		
		UpdatePCRegs();
		DEBUG('s',"Systemcall Exec: PID [%d] After Update PCRegs\n", currentPCB->PID);
		PrintMachineRegisters();
		return ;
    }

	// Replace Memory;
	bool replaceSuccess = currentAddrSpace->ReplaceMemory(executable);
	if (!replaceSuccess) {
		machine->WriteRegister(2, -1);
		delete []filename;
		
		UpdatePCRegs();
		DEBUG('s',"Systemcall Exec: PID [%d] After Update PCRegs\n", currentPCB->PID);
		PrintMachineRegisters();
		return;
	}
	delete executable;			// close file
	currentAddrSpace->InitRegisters();		// set the initial register values
	currentAddrSpace->RestoreState();		// load page table register
	
	delete []filename;
	
	machine->WriteRegister(2, 1);
	DEBUG('s',"Systemcall Exec: PID [%d] After Update PCRegs\n", currentPCB->PID);
	PrintMachineRegisters();
}

void YieldSystemcall() {
	AddrSpace *currentAddrSpace = currentThread->space;
	PCB *currentPCB = currentAddrSpace->thisPCB;
	printf("System call: [%d] invoked Yield\n", currentPCB->PID);
	DEBUG('s',"Systemcall Yield: PID [%d]\n", 
			currentAddrSpace->thisPCB->PID);
	PrintMachineRegisters();
	
	currentThread->Yield();
	
	UpdatePCRegs();
	DEBUG('s',"Systemcall Yield: PID [%d] After Update PCRegs\n", currentPCB->PID);
	PrintMachineRegisters();
}

void JoinSystemcall() {
	AddrSpace *currentAddrSpace = currentThread->space;
	PCB *currentPCB = currentAddrSpace->thisPCB;
	int joinPID = machine->ReadRegister(4);
	printf("System Call: [%d] invoked Join\n", currentPCB->PID);
	DEBUG('s',"Systemcall Join: PID [%d] join [%d]\n", 
			currentPCB->PID, joinPID);
	PrintMachineRegisters();
	
	// 1. check child
	if (joinPID < 0 || joinPID >= processMgr->NumTotalPID) {
		DEBUG('s', "Systemcall Join: Invalid joinPID\n");
		machine->WriteRegister(2, -1);

		UpdatePCRegs();
		DEBUG('s',"Systemcall Join: PID [%d] After Update PCRegs\n", currentPCB->PID);
		PrintMachineRegisters();
		return ;
	}

	currentPCB->PrintPCB();
	PCB *childPCB = currentPCB->GetChildPCB(joinPID);

	if (childPCB == NULL) {
		DEBUG('s', "Systemcall Join: ChildPCB Cannot Find\n");
		machine->WriteRegister(2, -1);

		UpdatePCRegs();
		DEBUG('s',"Systemcall Join: PID [%d] After Update PCRegs\n", currentPCB->PID);
		PrintMachineRegisters();
		return;
	}
	DEBUG('s', "Systemcall Join: currentPCB\n");
	currentPCB->PrintPCB();
	DEBUG('s', "Systemcall Join: childPCB\n");
	childPCB->PrintPCB();
	if (childPCB->parentPCB->PID != currentPCB->PID) {
		DEBUG('s', "Systemcall Join: currentPCB [%d] != childParentPCB [%d]\n",
				currentPCB->PID, childPCB->parentPCB->PID);
		machine->WriteRegister(2, -1);
		ASSERT(childPCB->parentPCB->PID == currentPCB->PID);
	}

	while(processMgr->IsSet(joinPID)) 
		currentThread->Yield();
	DEBUG('s',"Systemcall Join: currentPCB After child exit\n");
	currentPCB->PrintPCB();
	machine->WriteRegister(2, currentPCB->childExitValue);
	
	UpdatePCRegs();
	DEBUG('s',"Systemcall Join: PID [%d] After Update PCRegs\n", currentPCB->PID);
	PrintMachineRegisters();
}

void KillSystemcall() {
	AddrSpace *currentAddrSpace = currentThread->space;
	PCB *currentPCB = currentAddrSpace->thisPCB;
	int killPID = machine->ReadRegister(4);
	printf("System Call: [%d] invoked Kill\n", currentPCB->PID);
	DEBUG('s',"Systemcall Kill: PID [%d] kill [%d]\n", 
			currentPCB->PID, killPID);
	PrintMachineRegisters();
	PCB *killPCB = processMgr->GetPCB(killPID);
	if (killPCB != NULL) {
		printf("Process [%d] killed process [%d]\n", currentPCB->PID, killPID);
		if (killPID == currentPCB->PID) {
			DEBUG('s', "Systemcall Kill: PID [%d] kill self\n", currentPCB->PID);
			ExitSystemcall();
			machine->WriteRegister(2, 0);
			return ;
		}

		DEBUG('s', "Systemcall Kill: Kill PCB\n");
		killPCB->PrintPCB();
		Thread *killThread = killPCB->thisThread;
		AddrSpace *killAddrSpace = killThread->space;
		killPCB->RemoveChildParentPCB();
		killPCB->RemoveParentPCBChild(-1);
		processMgr->RemovePCB(killPCB);
		DEBUG('s',"Systemcall Kill: ProcessMgr has %d PIDs\n\n", processMgr->GetNumFreePCBs());
		
		killAddrSpace->ReleaseMemory();
		DEBUG('s',"Systemcall Kill: MemoryMgr has %d pages\n\n", memoryMgr->GetNumFreePages());
		
		delete killAddrSpace;
		delete killPCB;
		
		// Remove killThread from Scheduler 
		scheduler->RemoveThread(killThread);
		machine->WriteRegister(2, 0);
		
		UpdatePCRegs();
		DEBUG('s',"Systemcall Kill: PID [%d] After Update PCRegs\n", currentPCB->PID);
		PrintMachineRegisters();
		return;
	}
	printf("Process [%d] cannot kill process [%d]: does not exist\n", currentPCB->PID, killPID);
	DEBUG('s', "Systemcall Kill:  PID [%d] Not Exist\n");
	machine->WriteRegister(2, -1);
	
	UpdatePCRegs();
	DEBUG('s',"Systemcall Kill: PID [%d] After Update PCRegs\n", currentPCB->PID);
	PrintMachineRegisters();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
		DEBUG('s', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
	}
	else if ((which == SyscallException) && (type == SC_Exit)) {
		ExitSystemcall();
	}
	else if ((which == SyscallException) && (type == SC_Fork)) {
		ForkSystemcall();
	}
	else if ((which == SyscallException) && (type == SC_Exec)) {
		ExecSystemcall();
	}
	else if ((which == SyscallException) && (type == SC_Yield)) {
		YieldSystemcall();
	}
	else if ((which == SyscallException) && (type == SC_Join)) {
		JoinSystemcall();
	}
	else if ((which == SyscallException) && (type == SC_Kill)) {
		KillSystemcall();
	}
    else {
		DEBUG('s', "SyscallException %d, This Exception %d, Tye %d\n", SyscallException, which, type);
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}

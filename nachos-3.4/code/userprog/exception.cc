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

	DEBUG('s',"Systemcall Exit: PID [%d], Value %d\n", 
			currentAddrSpace->thisPCB->PID, val);
	PrintMachineRegisters();
	
	currentPCB->RemoveChildParentPCB();
	currentPCB->RemoveParentPCBChild(val);
	processMgr->RemovePCB(currentPCB);
	DEBUG('s',"System %d PIDs\n", processMgr->GetNumFreePCBs());
	
	currentAddrSpace->PrintPageTable();
	currentAddrSpace->ReleaseMemory();
	DEBUG('s',"System memory %d pages\n", memoryMgr->GetNumFreePages());
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
	DEBUG('s',"Systemcall Fork: PID [%d]\n", 
			currentAddrSpace->thisPCB->PID);
	PrintMachineRegisters();

	// 1. save old process registers
	currentAddrSpace->SaveUserRegisters();
	currentAddrSpace->PrintUserRegisters();

	// 2. create a PCB and associate
	PCB *childPCB = processMgr->CreatePCB();
	if (childPCB == NULL) {
		machine->WriteRegister(2, -1);
		return;
	}
	AddrSpace *childAddrSpace = currentAddrSpace->Fork();
	if (childAddrSpace == NULL) {
		machine->WriteRegister(2, -1);
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

	// 4. // Lock????
	int childPCReg = machine->ReadRegister(4);
	DEBUG('s',"PID [%d] Fork Child New PCReg %d\n", 
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

	PrintMachineRegisters();
	UpdatePCRegs();
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
    else {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}

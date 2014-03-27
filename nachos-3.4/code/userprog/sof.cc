#include "sof.h"
#include "synch.h"
#include "filesys.h"
#include "openfile.h"

extern FileSystem fileSystem;

SOF::SOF(char *inputFilename, OpenFile *inputOpenFile) : 
		openFile(inputOpenFile), counter(0), fileLock("File Lock") {
	unsigned int lenFilename = strlen(inputFilename);
	filename = new char[lenFilename];
	strcpy(filename, inputFilename);
}

SOF::~SOF() {
	delete filename;
}

int SOF::Write(char *buffer, int size, int offset) {
	int numWrite = 0;
	fileLock.Acquire();
	numWrite = openFile->WriteAt(buffer, size, offset);
	fileLock.Release();
	return numWrite;
}

int SOF::Read(char *buffer, int size, int offset) {
	int numRead = 0;
	fileLock.Acquire();
	numRead = openFile->ReadAt(buffer, size, offset);
	fileLock.Release();
	return numRead;
}

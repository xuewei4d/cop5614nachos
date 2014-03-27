#include "sofmgr.h"
#include "sof.h"
#include "filesys.h"
extern FileSystem *fileSystem;

SOFMgr::SOFMgr() : sofMap(SOF_MAXNUM) {

}

int SOFMgr::FindSOFByName(char *filename) {
	for (int i = 0; i < SOF_MAXNUM; ++i) 
		if (sofMap.Test(i) && strcmp(filename, sofArray[i]->filename) == 0)
			return i;
	return -1;
}

SOFMgr::~SOFMgr() {
	
}

void SOFMgr::PrintSOF(){
	DEBUG('s', "SOFMgr PrintSOF:\n");
	for (int i = 0; i < SOF_MAXNUM; ++i) {
		if (sofMap.Test(i)) {
			DEBUG('s', "SOF %d: Filename %s, OpenFile 0x%x, Counter %d\t", 
					i, sofArray[i]->filename, sofArray[i]->openFile,
					sofArray[i]->counter);
		}
	}
	DEBUG('s',"\n\n");
}

int SOFMgr::CreateSOF(char *filename) {
	OpenFile *newOpenFile = fileSystem->Open(filename);
	if (newOpenFile == NULL) {
		DEBUG('s', "SOFMgr CreateSOF: Cannot Open File %s\n", filename);
		return -1;
	}
	int sofIndex = sofMap.Find();
	if (sofIndex == -1) {
		DEBUG('s', "SOFMgr CreateSOF: Not enough space\n");
		return -1;
	}
	sofArray[sofIndex] = new SOF(filename, newOpenFile);
	return sofIndex;
}

void SOFMgr::RemoveSOF(int sofIndex) {
	if (sofMap.Test(sofIndex)) {
		sofArray[sofIndex]->counter --;
		DEBUG('s', "SOFMgr RemoveSOF: SOF %d, counter %d\n",
				sofIndex, sofArray[sofIndex]->counter);
		if (sofArray[sofIndex]->counter == 0) {
			sofMap.Clear(sofIndex);
			delete sofArray[sofIndex];
			DEBUG('s',"SOFMgr RemoveSOF: SOF %d remove\n", sofIndex);
		}
	}
	else
		DEBUG('s',"SOFMgr RemoveSOF: Error SOFIndex %d\n", sofIndex);
}

int SOFMgr::WriteSOF(char *buffer, int size, int offset, int sofIndex) {
	return sofArray[sofIndex]->Write(buffer, size, offset);
}

int SOFMgr::ReadSOF(char *buffer, int size, int offset, int sofIndex) {
	return sofArray[sofIndex]->Read(buffer, size, offset);
}

void SOFMgr::IncSOFCounter(int sofIndex) {
	sofArray[sofIndex]->counter ++;
}

void SOFMgr::DecSOFCounter(int sofIndex) {
	sofArray[sofIndex]->counter --;
}

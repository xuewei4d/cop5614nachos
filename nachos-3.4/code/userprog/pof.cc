#include "pof.h"
#include "string.h"

POF::POF(unsigned int inputIndex, char *inputFilename) : 
		sofIndex(inputIndex), offset(0) {
	unsigned int lenFilename = strlen(inputFilename);
	filename = new char[lenFilename];
	strncpy(filename, inputFilename, lenFilename);
}

POF::~POF(){

	delete filename;
}

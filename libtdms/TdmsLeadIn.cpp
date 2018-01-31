#include <string>
#include <sstream>

#include "TdmsLeadIn.h"
#include "TdmsParser.h"

using namespace std;

TdmsLeadIn::TdmsLeadIn(TdmsParser *parser, bool verbose)
{
	d_parser = parser;
	read(parser->fileStream(), verbose);
}

void TdmsLeadIn::read(std::iendianfstream& file, const bool verbose)
{
	char buffer[4];
	file.read(buffer, 4);

	std::string tdmsString(buffer, 4);

	if ((tdmsString.at(0) == 0) || (tdmsString.compare("TDSm") != 0)){
		if (d_parser)
			nextSegmentOffset = d_parser->fileSize();
		flagHasMetaData = false;
		flagHasObjectList = false;
		flagHasRawData = false;
		flagHasDAQmxData = false;
		if (verbose)
			printf("\nInvalid header tag: '%s' read from file, should be 'TDSm'!\n", tdmsString.c_str());
		return;
	}

	unsigned int tocMask = 0;
	file >> tocMask;

	flagHasMetaData   = ((tocMask &   2) != 0);
	flagHasObjectList = ((tocMask &   4) != 0);
	flagHasRawData    = ((tocMask &   8) != 0);
	flagIsInterleaved = ((tocMask &  32) != 0);
	flagIsBigEndian   = ((tocMask &  64) != 0);
	flagHasDAQmxData  = ((tocMask & 128) != 0);

	file >> versionNumber;
	file >> nextSegmentOffset;
	file >> dataOffset;

	if (verbose && flagHasMetaData){
		std::cout << "\nRead lead-in data" << std::endl;
		std::cout << "  hasMetaData:         " << flagHasMetaData << std::endl;
		std::cout << "  hasObjectList:       " << flagHasObjectList << std::endl;
		std::cout << "  hasRawData:          " << flagHasRawData << std::endl;
		std::cout << "  isInterleaved:       " << flagIsInterleaved << std::endl;
		std::cout << "  isBigEndian:         " << flagIsBigEndian << std::endl;
		std::cout << "  hasDAQmxData:        " << flagHasDAQmxData << std::endl;
		std::cout << "  Version number:      " << versionNumber << std::endl;
		std::cout << "  Next segment offset: " << nextSegmentOffset << std::endl;
		std::cout << "  Data offset:         " << dataOffset << std::endl;
		printf ("\tPOS: 0x%X\n", (unsigned int)file.tellg());
	}
}

#ifndef TDMS_LEAD_IN_H
#define TDMS_LEAD_IN_H

class TdmsParser;
#include "endianfstream.hh"

class TdmsLeadIn
{
public:
	TdmsLeadIn(TdmsParser *, bool = false);
	void read(std::iendianfstream&, const bool verbose);

	bool hasMetaData() const {return flagHasMetaData;}
	bool hasRawData() const {return flagHasRawData;}
	bool hasDAQmxData() const {return flagHasDAQmxData;}

	long long getNextSegmentOffset() const {return nextSegmentOffset;}
	unsigned long long getDataOffset() const {return dataOffset;}

private:
	bool flagHasMetaData;
	bool flagHasObjectList;
	bool flagHasRawData;
	bool flagIsInterleaved;
	bool flagIsBigEndian;
	bool flagHasDAQmxData;
	unsigned int versionNumber;
	long long nextSegmentOffset;
	unsigned long long dataOffset;
	TdmsParser *d_parser;
};

#endif

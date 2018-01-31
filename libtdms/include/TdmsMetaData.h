#ifndef TDMS_META_DATA_H
#define TDMS_META_DATA_H

#include "endianfstream.hh"
#include <list>

class TdmsParser;
class TdmsObject;
class TdmsChannel;

class TdmsMetaData
{
public:
	typedef std::list<TdmsObject*> TdmsObjectList;
	TdmsMetaData(TdmsParser *, bool = false);
	void read(std::iendianfstream &infile, bool verbose);
	TdmsObject* readRawData(unsigned long long, TdmsObject *);
	const TdmsObjectList& getObjectList() const {return objects;}
	long long getRawDataChunkSize();

private:
	TdmsChannel* getChannel(TdmsObject *);
	void readObject(std::iendianfstream&, const bool);
	void print() const;
	unsigned int objectCount;
	TdmsObjectList objects;
	TdmsParser* d_parser;
	bool d_verbose;
};

#endif

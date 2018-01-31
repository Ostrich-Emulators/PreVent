#ifndef TDMS_OBJECT_H
#define TDMS_OBJECT_H

#include "endianfstream.hh"

#include <string>
#include <fstream>
#include <map>
#include <vector>

class TdmsChannel;

struct FormatChangingScaler
{
	unsigned int DAQmxDataType;
	unsigned int rawBufferIndex;
	unsigned int rawByteOffset;
	unsigned int sampleFormatBitmap;
	unsigned int scaleID;
};

class TdmsObject
{
public:
	TdmsObject(std::iendianfstream&, bool verbose);
	~TdmsObject();

	void readPath();
	void readRawDataInfo();
	void readRawData(unsigned long long, TdmsChannel*);
	void readDAQmxData(TdmsChannel*);
	void readFormatChangingScalers();
	void readPropertyCount();
	bool hasRawData() const;
	bool hasDAQmxData() const;
	bool isGroup() const;
	bool isRoot() const {return (path == "/");}
	std::string getChannelName() const;
	const std::string& getPath() const {return path;}

	unsigned int getDataType() const {return rawDataType;}
	void setRawDataInfo(TdmsObject *);

	unsigned int getRawDataIndex() const {return rawDataIndex;}
	unsigned long long getValuesCount() const {return nvalue;}
	unsigned long long getBytesCount() const {return nbytes;}
	unsigned int getDimension() const {return dimension;}

	long long getChannelSize() const;

	std::map<std::string, std::string> getProperties(){return properties;}
	static std::string timestamp(long long, unsigned long long);

	TdmsChannel *getChannel(){return d_channel;}
	void setChannel(TdmsChannel*);

	static unsigned int dataTypeSize(unsigned int);

private:
	void readProperty(unsigned int);
	std::string readValue(unsigned int, TdmsChannel* = 0);

	std::iendianfstream& file;
	bool d_verbose;

	std::string path;
	std::map<std::string, std::string> properties;
	unsigned int rawDataIndex;
	unsigned int rawDataType;
	bool flagHasRawData;
	unsigned int propertyCount;
	unsigned int dimension;
	unsigned long long nvalue;
	unsigned long long nbytes;
	TdmsChannel *d_channel;

	std::vector<FormatChangingScaler> d_format_scalers_vector;
	std::vector<unsigned int> d_raw_data_width_vector;
};

#endif

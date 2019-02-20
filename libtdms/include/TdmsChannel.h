#ifndef TDMS_CHANNEL_H
#define TDMS_CHANNEL_H

#include "TdmsObject.h"
#include "endianfstream.hh"
#include "TdmsParser.h"

#include <map>
#include <string>
#include <vector>
#include <list>

class TdmsChannel
{
public:
	typedef enum {
		tdsTypeVoid,
		tdsTypeI8,
		tdsTypeI16,
		tdsTypeI32,
		tdsTypeI64,
		tdsTypeU8,
		tdsTypeU16,
		tdsTypeU32,
		tdsTypeU64,
		tdsTypeSingleFloat,
		tdsTypeDoubleFloat,
		tdsTypeExtendedFloat,
		tdsTypeSingleFloatWithUnit = 0x19,
		tdsTypeDoubleFloatWithUnit,
		tdsTypeExtendedFloatWithUnit,
		tdsTypeString = 0x20,
		tdsTypeBoolean = 0x21,
		tdsTypeTimeStamp = 0x44,
		tdsTypeFixedPoint = 0x4F,
		tdsTypeComplexSingleFloat = 0x08000c,
		tdsTypeComplexDoubleFloat = 0x10000d,
		tdsTypeDAQmxRawData = 0xFFFFFFFF
	} tdsDataType;

	TdmsChannel(const std::string& name, std::iendianfstream &f, TdmsParser * p);
	~TdmsChannel();

	const std::string& getName() const {return name;}

	std::string getUnit() const;
	std::string getProperty(const std::string& name) const;
	std::string propertiesToString() const;

	unsigned long long getChannelSize() const;
	unsigned int getDataCount() const {return dataVector.size();}
	unsigned int getStringCount() const {return stringVector.size();}

	unsigned int getTypeSize() const {return typeSize;}
	void setTypeSize(unsigned int);

	unsigned int getDataType() const {return dataType;}
	void setDataType(unsigned int);

	void appendValue(double val){dataVector.push_back(val);}
	const std::vector<double>& getDataVector() {return dataVector;}

	void appendImaginaryValue(double val){imagDataVector.push_back(val);}
	std::vector<double> getImaginaryDataVector() {return imagDataVector;}

	void appendString(std::string s){stringVector.push_back(s);}
	std::vector<std::string> getStringVector() {return stringVector;}
	void freeMemory();

	void addProperties(std::map<std::string, std::string>);
	void setProperties(std::map<std::string, std::string> props){properties = props;}
	std::map<std::string, std::string> getProperties(){return properties;}

	unsigned int getDimension() const {return dimension;}
	void setDimension(unsigned int d){dimension = d;}

	unsigned int getValuesCount() const {return nvalues;}
	void setValuesCount(unsigned int);

	void readRawData(unsigned long long, bool);
	std::string readValue(unsigned int, bool = false);

	void readDAQmxData(std::vector<FormatChangingScaler>, std::vector<unsigned int>);
	void readDAQmxValue(unsigned int type, double slope, double intercept, bool verbose = false);

private:
	void readStrings();

	const std::string name;
	std::iendianfstream& file;
	unsigned int dataType;
	unsigned int typeSize;
	unsigned int dimension;
	unsigned long long nvalues;

	std::map<std::string, std::string> properties;
	std::vector<double> dataVector;
	std::vector<double> imagDataVector;
	std::vector<std::string> stringVector;
	TdmsParser * d_parser;
};
#endif

#include <string>
#include <algorithm>
#include <math.h>
#include <time.h>

#include "TdmsObject.h"
#include "TdmsChannel.h"

using namespace std;

TdmsObject::TdmsObject(std::iendianfstream& f, bool verbose)
: file(f),
d_verbose(verbose),
flagHasRawData(false),
nvalue(0),
nbytes(0),
d_channel(0)
{
	readPath();
	readRawDataInfo();
	readPropertyCount();

	if (verbose)
		printf("	Properties (%d):\n", propertyCount);

	for (unsigned int i = 0; i < propertyCount; ++i)
		readProperty(i);

	if (verbose)
		printf ("\t\tPOS: 0x%X\n", (unsigned int)file.tellg());
}

void TdmsObject::readProperty(unsigned int i)
{
	unsigned int size;
	file >> size;

	string name(size, 0);
	file >> name;

	unsigned int itype;
	file >> itype;

	if (d_verbose)
		printf("	%d %s: ", i + 1, name.c_str());

	string val = readValue(itype);
	if (!val.empty())
		properties.insert(std::pair<string, string>(name, val));
}

string TdmsObject::timestamp(long long secs, unsigned long long fractionSecs)
{
	time_t t = secs - 2082844800;//substract secs until 1970

	struct tm *pt = gmtime(&t);
	if (!pt)
		return " ";

	char buffer[80];
	sprintf(buffer, "%d.%02d.%d %02d:%02d:%02d,%f",
			pt->tm_mday, pt->tm_mon + 1, 1900 + pt->tm_year, pt->tm_hour, pt->tm_min, pt->tm_sec, fractionSecs*pow(2, -64));
	return string(buffer);
}

string TdmsObject::readValue(unsigned int itype, TdmsChannel* channel)
{
	int size = 100;
	char output [size];

	if (channel)
		channel->setDataType(itype);

	switch (itype){
		case 1: //INT8
		{
			char val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%d (type = %d)\n", (int)val, itype);
		}
		break;

		case 2: //INT16
		{
			short val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			snprintf(output, size, "%d", val);
		}
		break;

		case 3: //INT32
		{
			int val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%d (type = %d)\n", val, itype);

			snprintf(output, size, "%d", val);
		}
		break;

		case 4: //INT64
		{
			long long val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			snprintf(output, size, "%d", (int)val);
		}
		break;

		case 5: //UINT8
		{
			unsigned char val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			snprintf(output, size, "%d", val);
		}
		break;


		case 6: //UINT16
		{
			unsigned short val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			snprintf(output, size, "%d", val);
		}
		break;

		case 7: //UINT32
		{
			unsigned int val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%u (type = %d)\n", val, itype);
			snprintf(output, size, "%u", val);
		}
		break;

		case 8: //UINT64
		{
			unsigned long long val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%u (type = %d)\n", (unsigned int)val, itype);
			snprintf(output, size, "%u", (unsigned int)val);
		}
		break;

		case 9: //FLOAT32
		case TdmsChannel::tdsTypeSingleFloatWithUnit:
		{
			float val;
			file >> val;
			if (channel)
				channel->appendValue((float)val);
			if (d_verbose)
				printf("%g (type = %d)\n", val, itype);
			snprintf(output, size, "%g", val);
		}
		break;

		case 10: //FLOAT64
		case TdmsChannel::tdsTypeDoubleFloatWithUnit:
		{
			double val;
			file >> val;
			if (channel)
				channel->appendValue(val);
			if (d_verbose)
				printf("%g (type = %d)\n", val, itype);

			snprintf(output, size, "%g", val);
		}
		break;

		case 11: //FLOAT128
		case TdmsChannel::tdsTypeExtendedFloatWithUnit:
		{
			long double val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%f (type = %d)\n", (double)val, itype);
			snprintf(output, size, "%f", (double)val);
		}
		break;

		case 32: //string
		{
			unsigned int size;
			file >> size;
			string s(size, 0);
			file >> s;
			if (d_verbose)
				printf("%s (type = %d)\n", s.c_str(), itype);
			return s;
		}
		break;

		case 33: //bool
		{
			bool val;
			file >> val;
			if (channel)
				channel->appendValue((double)val);
			if (d_verbose)
				printf("%d (type = %d)\n", val, itype);
			snprintf(output, size, "%d", val);
		}
		break;

		case 68: //time stamp
		{
			unsigned long long fractionsSecond;
			file >> fractionsSecond;
			long long secondsSince;
			file >> secondsSince;
			string ts = timestamp(secondsSince, fractionsSecond);
			if (d_verbose)
				printf("%s (type = %d)\n", ts.c_str(), itype);
			return ts;
		}
		break;

		case TdmsChannel::tdsTypeComplexSingleFloat:
		{
			float rval, ival;
			file >> rval;
			if (channel)
				channel->appendValue((float)rval);
			file >> ival;
			if (channel)
				channel->appendImaginaryValue((float)ival);
			if (d_verbose)
				printf("%g+i*%g (type = 0x%X)\n", rval, ival, itype);
			snprintf(output, size, "%g+i*%g", rval, ival);
		}
		break;

		case TdmsChannel::tdsTypeComplexDoubleFloat:
		{
			double rval, ival;
			file >> rval;
			if (channel)
				channel->appendValue(rval);
			file >> ival;
			if (channel)
				channel->appendImaginaryValue(ival);
			if (d_verbose)
				printf("%g+i*%g (type = 0x%X)\n", rval, ival, itype);
			snprintf(output, size, "%g+i*%g", rval, ival);
		}
		break;

		default:
		{
			if (d_verbose)
				printf(" (unknown type = %d)\n", itype);
		}
		break;
	}

	return string(output);
}

unsigned int TdmsObject::dataTypeSize(unsigned int itype)
{
	switch (itype){
		case 1: //INT8
		{
			char val;
			return sizeof(val);
		}
		break;

		case 2: //INT16
		{
			short val;
			return sizeof(val);
		}
		break;

		case 3: //INT32
		{
			int val;
			return sizeof(val);
		}
		break;

		case 4: //INT64
		{
			long long val;
			return sizeof(val);
		}
		break;

		case 5: //UINT8
		{
			unsigned char val;
			return sizeof(val);
		}
		break;

		case 6: //UINT16
		{
			unsigned short val;
			return sizeof(val);
		}
		break;

		case 7: //UINT32
		{
			unsigned int val;
			return sizeof(val);
		}

		case 8: //UINT64
		{
			unsigned long long val;
			return sizeof(val);
		}
		break;

		case 9: //FLOAT32
		case TdmsChannel::tdsTypeSingleFloatWithUnit:
		{
			float val;
			return sizeof(val);
		}
		break;

		case 10: //FLOAT64
		case TdmsChannel::tdsTypeDoubleFloatWithUnit:
		{
			double val;
			return sizeof(val);
		}
		break;

		case 11: //FLOAT128
		case TdmsChannel::tdsTypeExtendedFloatWithUnit:
		{
			long double val;
			return sizeof(val);
		}
		break;

		case 32: //string
		break;

		case 33: //bool
		{
			bool val;
			return sizeof(val);
		}
		break;

		case 68: //time stamp
		{
			long long secondsSince;
			unsigned long long fractionsSecond;
			return sizeof(secondsSince) + sizeof(fractionsSecond);
		}
		break;

		case TdmsChannel::tdsTypeComplexSingleFloat:
		{
			float val;
			return 2*sizeof(val);
		}
		break;

		case TdmsChannel::tdsTypeComplexDoubleFloat:
		{
			double val;
			return 2*sizeof(val);
		}
		break;

		default:
		break;
	}
	return 0;
}

void TdmsObject::readPath()
{
	unsigned int size;
	file >> size;
	file >> path.assign(size, 0);

	if (d_verbose){
		printf("OBJECT PATH: %s", path.c_str());
		if (isRoot())
			printf(" is root!\n");
		else if (isGroup())
			printf(" is a group!\n");
		else {
			printf("\n Channel name: %s\n", getChannelName().c_str());
		}
	}
}

void TdmsObject::setRawDataInfo(TdmsObject *obj)
{
	if (!obj)
		return;

	rawDataType = obj->getDataType();
}

void TdmsObject::readRawDataInfo()
{
	file >> rawDataIndex;
	if (d_verbose)
		printf("\tRaw data index: %d @ 0x%X\n", rawDataIndex, (unsigned int)file.tellg());

	if (rawDataIndex == 0){
		if (d_verbose)
			printf("\t\tObject in this segment exactly matches the raw data index the same object had in the previous segment!\n");
	}


	flagHasRawData = (rawDataIndex != 0xFFFFFFFF);

	if (flagHasRawData && rawDataIndex > 0){
		file >> rawDataType;
		file >> dimension;
		file >> nvalue;

		if (rawDataType == TdmsChannel::tdsTypeString)
			file >> nbytes;

		if (d_verbose){
			if (rawDataType == TdmsChannel::tdsTypeString)
				printf("\tHas raw data: type=0x%X dimension=%d values=%d nbytes=%d\n", rawDataType, dimension, (unsigned int)nvalue, (unsigned int)nbytes);
			else if (rawDataType == TdmsChannel::tdsTypeDAQmxRawData)
				printf("\tHas DAQmx raw data: type=0x%X dimension=%d values=%d\n", rawDataType, dimension, (unsigned int)nvalue);
			else
				printf("\tHas raw data: type=0x%X dimension=%d values=%d\n", rawDataType, dimension, (unsigned int)nvalue);
		}

		if (rawDataType == TdmsChannel::tdsTypeDAQmxRawData)
			readFormatChangingScalers();
	}
}

void TdmsObject::readPropertyCount()
{
	file >> propertyCount;
}

long long TdmsObject::getChannelSize() const
{
	if (!nvalue)
		return 0;

	if (rawDataType == TdmsChannel::tdsTypeString)
		return nbytes;
	else if (rawDataType == TdmsChannel::tdsTypeDAQmxRawData){
		if (d_format_scalers_vector.empty())
			return 0;
		return (long long)dataTypeSize(d_format_scalers_vector.front().DAQmxDataType)*dimension*nvalue;
	}

	return dataTypeSize(rawDataType)*dimension*nvalue;
}

string TdmsObject::getChannelName() const
{
	int islash = path.find("'/'", 1) + 1;
	std::string channelName = path.substr(islash);
	std::string groupName = path.substr(0, islash);
	return channelName;
}

bool TdmsObject::hasRawData() const
{
	if (rawDataIndex == 0)
		return (nvalue > 0);

	return flagHasRawData;
}

bool TdmsObject::hasDAQmxData() const
{
	return (rawDataType == TdmsChannel::tdsTypeDAQmxRawData);
}

void TdmsObject::setChannel(TdmsChannel *channel)
{
	d_channel = channel;

	if (d_channel && (rawDataIndex == 0)){
		rawDataType = d_channel->getDataType();
		dimension = d_channel->getDimension();
		nvalue = d_channel->getValuesCount();

		if (rawDataType == TdmsChannel::tdsTypeString)
			nbytes = d_channel->getTypeSize();
	}
}

bool TdmsObject::isGroup() const
{
	if (isRoot())
		return false;

	return (path.find("'/'") == std::string::npos);
}

void TdmsObject::readRawData(unsigned long long total_chunk_size, TdmsChannel* channel)
{
	if (nvalue == 0){
		unsigned int typeSize = (rawDataType == TdmsChannel::tdsTypeString) ? nbytes : dataTypeSize(rawDataType);
		if (typeSize != 0)
			nvalue = total_chunk_size/typeSize;

		if (d_verbose)
			printf("\tReading %d data value(s) of type %d and type size %d bytes (total size %d bytes).\n", (unsigned int)nvalue,
			rawDataType, (unsigned int)typeSize, (unsigned int)total_chunk_size);
	} //else if (d_verbose)
		//printf("\tReading %d raw data value(s) of type %d.\n", (unsigned int)nvalue, rawDataType);

	if (channel){
		channel->setDataType(rawDataType);
		channel->readRawData(total_chunk_size,d_verbose);
	}
}

void TdmsObject::readDAQmxData(TdmsChannel* channel)
{
	if (channel)
		channel->readDAQmxData(d_format_scalers_vector, d_raw_data_width_vector);
}

void TdmsObject::readFormatChangingScalers()
{
	unsigned int scalersCount;
	file >> scalersCount;
	if (d_verbose)
		printf("\tFormat changing scalers vector size: %d @ 0x%X\n", scalersCount, (unsigned int)file.tellg());

	for (unsigned int i = 0; i < scalersCount; i++){
		if (d_verbose & (scalersCount > 1))
			printf("\t\ti = %d\n", i);

		FormatChangingScaler formatScaler;
		file >> formatScaler.DAQmxDataType;
		if (d_verbose)
			printf("\t\tDAQmx data type: %d @ 0x%X\n", formatScaler.DAQmxDataType, (unsigned int)file.tellg());

		file >> formatScaler.rawBufferIndex;
		if (d_verbose)
			printf("\t\tRaw buffer index: %d @ 0x%X\n", formatScaler.rawBufferIndex, (unsigned int)file.tellg());

		file >> formatScaler.rawByteOffset;
		if (d_verbose)
			printf("\t\tRaw byte offset within the stride: %d @ 0x%X\n", formatScaler.rawByteOffset, (unsigned int)file.tellg());

		file >> formatScaler.sampleFormatBitmap;
		if (d_verbose)
			printf("\t\tSample format bitmap: %d @ 0x%X\n", formatScaler.sampleFormatBitmap, (unsigned int)file.tellg());

		file >> formatScaler.scaleID;
		if (d_verbose)
			printf("\t\tScale ID: %d @ 0x%X\n", formatScaler.scaleID, (unsigned int)file.tellg());

		d_format_scalers_vector.push_back(formatScaler);
	}

	unsigned int vectorSize;
	file >> vectorSize;
	if (d_verbose)
		printf("\tRaw data width vector size: %d @ 0x%X\n", vectorSize, (unsigned int)file.tellg());

	for (unsigned int i = 0; i < vectorSize; i++){
		unsigned int val;
		file >> val;

		d_raw_data_width_vector.push_back(val);
		if (d_verbose){
			if (vectorSize > 1)
				printf("\ti = %d", i);
			printf("\tData width: %d @ 0x%X\n", val, (unsigned int)file.tellg());
		}
	}
}

TdmsObject::~TdmsObject()
{
	d_format_scalers_vector.clear();
	d_raw_data_width_vector.clear();
}

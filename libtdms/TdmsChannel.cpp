#include "TdmsChannel.h"
#include <cstring>
#include <stdlib.h>

#include "TdmsParser.h"
#include "TdmsListener.h"

using namespace std;

TdmsChannel::TdmsChannel(const std::string& name, std::iendianfstream& f, TdmsParser * p)
  : name(name), file(f), dataType(0), typeSize(0), dimension(0), nvalues(0),
    d_parser(p)
{
}

TdmsChannel::~TdmsChannel()
{
	freeMemory();
}

std::string TdmsChannel::getUnit() const
{
	return getProperty("unit_string");
}

std::string TdmsChannel::getProperty(const std::string& name) const
{
	map<string, string>::const_iterator it = properties.find(name);
	if (it != properties.end())
		return it->second;

	return "";
}

string TdmsChannel::propertiesToString() const
{
	string s;
	for (map<string, string>::const_iterator it = properties.begin(); it != properties.end(); ++it){
		s.append(it->first + ": ");
		s.append(it->second + "\n");
	}
	return s;
}

void TdmsChannel::addProperties(std::map<std::string, std::string> props)
{
	for (map<string, string>::const_iterator it = props.begin(); it != props.end(); ++it)
		properties.insert(std::pair<string, string>(it->first, it->second));
}

void TdmsChannel::freeMemory()
{
	dataVector.clear();
	imagDataVector.clear();
	stringVector.clear();
	properties.clear();
}

void TdmsChannel::setTypeSize(unsigned int size)
{
	typeSize = size;
}

void TdmsChannel::setDataType(unsigned int type)
{
	dataType = type;
}

void TdmsChannel::setValuesCount(unsigned int n)
{
	if (!n)
		return;

	nvalues = n;
}

unsigned long long TdmsChannel::getChannelSize() const
{
	return typeSize*dimension*nvalues;
}

void TdmsChannel::readRawData(unsigned long long total_chunk_size, bool verbose)
{
	if (nvalues == 0 && typeSize != 0)
		nvalues = total_chunk_size/typeSize;

	if (verbose)
		printf("\tChannel %s: reading %d raw data value(s) of type %d.", name.c_str(), (unsigned int)nvalues, dataType);

	if (dataType == TdmsChannel::tdsTypeString)
		readStrings();
	else {
    if( d_parser->hasListeners() ) {
      dataVector.reserve( nvalues );
    }
		for (unsigned int i = 0; i < nvalues; ++i)
			readValue(dataType, false);
    if( d_parser->hasListeners() ){
      for( auto& l : d_parser->listeners() ){
        l->newValueChunk( this, dataVector );
      }
      dataVector.clear();
    }
	}

	if (verbose)
		printf(" Finished reading raw data (POS: 0x%X).\n", (unsigned int)file.tellg());
}

void TdmsChannel::readDAQmxData(std::vector<FormatChangingScaler> formatScalers, std::vector<unsigned int> dataWidths)
{
	if (formatScalers.empty() || dataWidths.empty())
		return;

	FormatChangingScaler formatScaler = formatScalers.front();
	unsigned int type = formatScaler.DAQmxDataType;
	unsigned int dataWidth = dataWidths.front();
	unsigned int formatTypeSize = TdmsObject::dataTypeSize(type)*dataWidth;

	if (formatTypeSize == TdmsObject::dataTypeSize(tdsTypeI64))
		type = tdsTypeI64;
	else if (formatTypeSize == TdmsObject::dataTypeSize(tdsTypeI32))
		type = tdsTypeI32;
	else if (formatTypeSize == TdmsObject::dataTypeSize(tdsTypeI16))
		type = tdsTypeI16;

	if (type != formatScaler.DAQmxDataType){
		std::string slopeString = getProperty("NI_Scale[1]_Linear_Slope");
		double slope = slopeString.empty() ? 1.0 : atof(slopeString.c_str());

		std::string interceptString = getProperty("NI_Scale[1]_Linear_Y_Intercept");
		double intercept = interceptString.empty() ? 0.0 : atof(interceptString.c_str());

		unsigned int values = nvalues/dataWidth;
		for (unsigned int i = 0; i < values; ++i)
			readDAQmxValue(type, slope, intercept);
	} else {
		for (unsigned int i = 0; i < nvalues; ++i)
			readValue(type);
	}
}

void TdmsChannel::readDAQmxValue(unsigned int type, double slope, double intercept, bool verbose)
{
	if (verbose)
		printf("\tRead DAQmx value for channel: %s\n", name.c_str());

	switch (type)
	{
		case tdsTypeI32:
		{
			int val;
			file >> val;
			if (verbose)
				printf("\t%d -> %g (type = %d)\n", val, val*slope + intercept, type);
			appendValue(val*slope + intercept);
		}
		break;

		default:
		{
			if (verbose)
				printf("\t(unknown type = %d)\n", type);
		}
		break;
	}
}

void TdmsChannel::readStrings()
{
	vector<unsigned int> offsets;
	offsets.push_back(0);
	for (unsigned int i = 0; i < nvalues; ++i){
		unsigned int offset;
		file >> offset;
		//printf("i: %d offset = %d POS @ 0x%X\n", i, offset, (unsigned int)file.tellg());
		offsets.push_back(offset);
	}

	unsigned int POS = offsets.at(0);
	for (unsigned int i = 1; i <= nvalues; ++i){
		unsigned int offset = offsets.at(i);
		unsigned int size = offset - POS;
		string s(size, 0);
		file >> s;
    stringVector.push_back( s );
		//printf("i: %d offset: %d size = %d s: %s POS %d @ 0x%X\n", i, offset, size, s.c_str(), POS, (unsigned int)file.tellg());
		POS = offset;
	}

  if( d_parser->hasListeners() ){
    // if we have listeners, provide what we just read, and clear the results
    for( auto& l : d_parser->listeners() ){
      l->newValueChunk(this, stringVector);
    }
    stringVector.clear();
  }
}


string TdmsChannel::readValue(unsigned int itype, bool verbose)
{

	if (verbose)
		printf("	Read value for channel: %s\n", name.c_str());

	switch (itype){
		case 1: //INT8
		{
			char val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%d (type = %d)\n", (int)val, itype);
		}
		break;

		case 2: //INT16
		{
			short val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			//snprintf(output, size, "%d", val);
		}
		break;

		case 3: //INT32
		{
			int val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%d (type = %d)\n", val, itype);

			//snprintf(output, size, "%d", val);
		}
		break;

		case 4: //INT64
		{
			long long val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			//snprintf(output, size, "%d", (int)val);
		}
		break;

		case 5: //UINT8
		{
			unsigned char val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			//snprintf(output, size, "%d", val);
		}
		break;


		case 6: //UINT16
		{
			unsigned short val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%d (type = %d)\n", (int)val, itype);
			//snprintf(output, size, "%d", val);
		}
		break;

		case 7: //UINT32
		{
			unsigned int val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%u (type = %d)\n", val, itype);
			//snprintf(output, size, "%u", val);
		}
		break;

		case 8: //UINT64
		{
			unsigned long long val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%u (type = %d)\n", (unsigned int)val, itype);
			//snprintf(output, size, "%u", (unsigned int)val);
		}
		break;

		case 9: //FLOAT32
		case TdmsChannel::tdsTypeSingleFloatWithUnit:
		{
			float val;
			file >> val;
			appendValue((float)val);
			if (verbose)
				printf("%f (type = %d)\n", val, itype);
			//snprintf(output, size, "%f", val);
		}
		break;

		case 10: //FLOAT64
		case TdmsChannel::tdsTypeDoubleFloatWithUnit:
		{
			double val;
			file >> val;
			appendValue(val);
			if (verbose)
				printf("%f (type = %d)\n", val, itype);

			//snprintf(output, size, "%f", val);
		}
		break;

		case 11: //FLOAT128
		case TdmsChannel::tdsTypeExtendedFloatWithUnit:
		{
			long double val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%f (type = %d)\n", (double)val, itype);
			//snprintf(output, size, "%f", (double)val);
		}
		break;

		case 32: //string values are read in readRawData function directly
		break;

		case 33: //bool
		{
			bool val;
			file >> val;
			appendValue((double)val);
			if (verbose)
				printf("%d (type = %d)\n", val, itype);
			//snprintf(output, size, "%d", val);
		}
		break;

		case tdsTypeTimeStamp: //time stamp
		{
			unsigned long long fractionsSecond;
			file >> fractionsSecond;
			long long secondsSince;
			file >> secondsSince;
			string ts = TdmsObject::timestamp(secondsSince, fractionsSecond);
			appendString(ts);
			if (verbose)
				printf("%s (type = %d)\n", ts.c_str(), itype);
			return ts;
		}
		break;

		case TdmsChannel::tdsTypeComplexSingleFloat:
		{
			float rval, ival;
			file >> rval;
			appendValue((float)rval);
			file >> ival;
			appendImaginaryValue((float)ival);
			if (verbose)
				printf("%g+i*%g (type = 0x%X)\n", rval, ival, itype);
			//snprintf(output, size, "%g+i*%g", rval, ival);
		}
		break;

		case TdmsChannel::tdsTypeComplexDoubleFloat:
		{
			double rval, ival;
			file >> rval;
			appendValue(rval);
			file >> ival;
			appendImaginaryValue(ival);
			if (verbose)
				printf("%g+i*%g (type = 0x%X)\n", rval, ival, itype);
			//snprintf(output, size, "%g+i*%g", rval, ival);
		}
		break;

		default:
		{
			if (verbose)
				printf(" (unknown type = %d)\n", itype);
		}
		break;
	}

	return "";//string(output);
}

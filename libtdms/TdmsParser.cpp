#include "TdmsParser.h"
#include "TdmsLeadIn.h"
#include "TdmsMetaData.h"
#include "TdmsGroup.h"
#include "TdmsChannel.h"
#include "TdmsObject.h"

using namespace std;

TdmsParser::TdmsParser(char *fileName)
:	file(fileName, ios::binary), d_prev_object(0)
{
}

TdmsParser::TdmsParser(const std::string& fileName)
:	file(fileName.c_str(), ios::binary), d_prev_object(0)
{
}

TdmsParser::~TdmsParser()
{
}

void TdmsParser::close( ) {
  file.close( );
  file.clear( );

  for ( TdmsGroup * c : groups ) {
    for ( size_t i = 0; i < c->getGroupSize( ); i++ ) {
      c->getChannel( i )->freeMemory( );
    }
    delete c;
  }

  groups.clear( );
}

void TdmsParser::init( ) {
  file.seekg( 0, std::ios::end );
  d_file_size = file.tellg( );
  file.seekg( 0, std::ios::beg );
}

bool TdmsParser::nextSegment( const bool verbose ) {
  if ( file.tellg( ) < (unsigned int) d_file_size ) {
    unsigned long long nextSegmentOffset = 0;
    bool atEnd = false;

    nextSegmentOffset = readSegment( verbose, &atEnd );

    if ( verbose ) {
      printf( "\nPOS after segment: 0x%X\n", (unsigned int) file.tellg( ) );
      if ( atEnd )
        printf( "Should skip to the end of file...File format error?!\n" );
    }

    if ( nextSegmentOffset >= (unsigned long long) d_file_size ) {
      if ( verbose ) printf( "\tEnd of file is reached after segment!\n" );
    }

    // if we're not at the end, we can read more
    return !atEnd;
  }

  // didn't read anything, so we're done reading
  return false;
}

void TdmsParser::read(const bool verbose)
{
  init();
	if (verbose)
		printf("File size is: %d bytes (0x%X).\n", (unsigned int)d_file_size, (unsigned int)d_file_size);

	int seg = 0;
	unsigned long long nextSegmentOffset = 0;
	bool atEnd = false;
	while (file.tellg() < (unsigned int)d_file_size){
		nextSegmentOffset = readSegment(verbose, &atEnd);
		seg++;

		if (verbose){
			printf("\nPOS after segment %d: 0x%X\n", seg, (unsigned int)file.tellg());
			if (atEnd)
				 printf("Should skip to the end of file...File format error?!\n");
		}

		if (nextSegmentOffset >= (unsigned long long)d_file_size){
			if (verbose) printf("\tEnd of file is reached after segment %d!\n", seg);
			break;
		}
	}

	if (!atEnd && (file.tellg() < (unsigned int)d_file_size)){
		if (verbose)
			printf("\nFile contains raw data at the end!\n");

		readRawData((unsigned long long)(nextSegmentOffset - file.tellg()), verbose);
	}

	if (verbose)
		printf("\nNumber of segments: %d\n", seg);

	file.close();
}

void TdmsParser::addListener( TdmsListener * l ) {
  listenees.push_back( l );
}

std::vector<TdmsListener *> TdmsParser::listeners( ) const {
  return listenees;
}

bool TdmsParser::hasListeners( ) const {
  return !listenees.empty( );
}

unsigned long long TdmsParser::readSegment(const bool verbose, bool *atEnd)
{
	TdmsLeadIn leadIn(this, verbose);//read Lead In;
	unsigned long long posAfterLeadIn = (unsigned long long)file.tellg();

	long long nextSegmentOffset = leadIn.getNextSegmentOffset();
	if (nextSegmentOffset == -1)
		nextSegmentOffset = d_file_size;

	*atEnd = (nextSegmentOffset >= (long long)d_file_size);
	long long nextOffset = (*atEnd) ? d_file_size : nextSegmentOffset + (long long)file.tellg();
	if (verbose)
		printf("NEXT OFFSET: %d (0x%X)\n", (unsigned int)nextOffset, (unsigned int)nextOffset);

	unsigned long long offset = leadIn.getDataOffset();

	if (leadIn.hasMetaData()){
		TdmsMetaData metaData(this, verbose);//read Meta Data;
		if (leadIn.hasRawData()){
			file.seekg(posAfterLeadIn + offset, ios_base::beg);
			if (verbose)
				printf("\tRaw data starts at POS: 0x%X\n", (unsigned int)file.tellg());

			unsigned long long total_chunk_size = nextSegmentOffset - offset;
			d_prev_object = metaData.readRawData(total_chunk_size, d_prev_object);
			if (verbose)
				printf("\tPOS after metadata: 0x%X\n", (unsigned int)file.tellg());
		}
	} else if (leadIn.hasRawData()){
		unsigned long long total_chunk_size = nextSegmentOffset - offset;
		if (verbose)
			printf("\tSegment without metadata!\n");

		file.seekg(posAfterLeadIn + offset, ios_base::beg);
		if (verbose)
			printf("\tRaw data starts at POS: 0x%X\n", (unsigned int)file.tellg());

		readRawData(total_chunk_size, verbose);
	} else if (verbose)
		printf("\tSegment without metadata or raw data!\n");

	return nextOffset;
}

void TdmsParser::readRawData(unsigned long long total_chunk_size, const bool verbose)
{
	if (verbose)
		printf("\tShould read %d rawdata bytes\n", (unsigned int)total_chunk_size);

	unsigned int groupCount = groups.size();
	for (unsigned int i = 0; i < groupCount; i++){
		TdmsGroup *group = getGroup(i);
		if (!group)
			continue;

		unsigned int channels = group->getGroupSize();
		unsigned long long chunkSize = 0;
		for (unsigned int j = 0; j < channels; j++){
			TdmsChannel *channel = group->getChannel(j);
			if (channel){
				unsigned long long channelSize = channel->getChannelSize();
				if (verbose)
					printf("\tChannel %s size is: %d\n", channel->getName().c_str(), (unsigned int)channelSize);

				chunkSize += channelSize;
			}
		}

		unsigned int chunks = total_chunk_size/chunkSize;
		if (verbose)
			printf("Total: %d chunks of raw data.\n", chunks);

		for (unsigned int k = 0; k < chunks; k++){
			for (unsigned int j = 0; j < channels; j++){
				TdmsChannel *channel = group->getChannel(j);
				if (channel)
					channel->readRawData(total_chunk_size, false);
			}
		}
	}
}

TdmsGroup* TdmsParser::getGroup(const std::string &name)
{
	for (TdmsGroupSet::iterator iter = groups.begin(); iter != groups.end(); ++iter){
		if ((*iter)->getName() == name)
			return *iter;
	}
	return 0;
}

TdmsGroup* TdmsParser::getGroup(unsigned int index) const
{
	if (index >= groups.size())
		return 0;

	return groups.at(index);
}

string TdmsParser::propertiesToString() const
{
	string s;
	for (map<string, string>::const_iterator it = properties.begin(); it != properties.end(); ++it){
		s.append(it->first + ": ");
		s.append(it->second + "\n");
	}
	return s;
}

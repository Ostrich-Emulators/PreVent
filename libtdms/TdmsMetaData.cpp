#include "TdmsMetaData.h"
#include "TdmsParser.h"
#include "TdmsObject.h"
#include "TdmsGroup.h"
#include "TdmsChannel.h"
#include "TdmsListener.h"

using namespace std;

TdmsMetaData::TdmsMetaData(TdmsParser* parser, bool verbose):
d_parser(parser),
d_verbose(verbose)
{
	read(parser->fileStream(), verbose);
}

void TdmsMetaData::read(std::iendianfstream& file, const bool verbose)
{
	file >> objectCount;
	if (verbose)
		print();

	for (unsigned int i = 0; i < objectCount; i++)
		readObject(file, verbose);

	if (verbose)
		printf ("\tRaw data chunk size: %d\n", (unsigned int)getRawDataChunkSize());
}

void TdmsMetaData::readObject(std::iendianfstream& file, const bool verbose)
{
	TdmsObject *o = new TdmsObject(file, verbose);
	objects.push_back(o);

	if (!o)
		return;

	if (o->isRoot()){
		d_parser->setProperties(o->getProperties());
		return;
	}

	std::string path = o->getPath();

	if (o->isGroup()){
		TdmsGroup *group = d_parser->getGroup(path);
		if (!group){
      group = new TdmsGroup(path);
			d_parser->addGroup(group);
			if (verbose)
				printf("NEW GROUP: %s\n", path.c_str());

      for( auto&x : d_parser->listeners() ){
        x->newGroup( group );
      }
		}
	} else {
		int islash = path.find("'/'", 1) + 1;
		std::string channelName = path.substr(islash);
		std::string groupName = path.substr(0, islash);

		TdmsGroup *group = d_parser->getGroup(groupName);
		if (!group){
			group = new TdmsGroup(groupName);
			d_parser->addGroup(group);
			if (verbose)
				printf("NEW GROUP: %s\n", path.c_str());

      for( auto&x : d_parser->listeners() ){
        x->newGroup( group );
      }
		}

		TdmsChannel *channel = group->getChannel(channelName);
		if (channel == 0){
			channel = new TdmsChannel(channelName, file, d_parser);
			channel->setProperties(o->getProperties());
			channel->setDimension(o->getDimension());

			unsigned int type = o->getDataType();
			channel->setDataType(type);
			channel->setTypeSize((type == TdmsChannel::tdsTypeString) ? (unsigned int)o->getBytesCount() : TdmsObject::dataTypeSize(type));

			group->addChannel(channel);
			if (verbose)
				printf("NEW CHANNEL: %s\n", channelName.c_str());

      for( auto&x : d_parser->listeners() ){
        x->newChannel( channel );
      }
		}

		std::map<std::string, std::string> properties = o->getProperties();
		if (!properties.empty())
			channel->addProperties(properties);

		channel->setValuesCount(o->getValuesCount());
		o->setChannel(channel);
	}
}

TdmsChannel* TdmsMetaData::getChannel(TdmsObject *obj)
{
	if (!obj || obj->isRoot() || obj->isGroup())
		return 0;

	std::string path = obj->getPath();
	int islash = path.find("'/'", 1) + 1;
	std::string channelName = path.substr(islash);
	std::string groupName = path.substr(0, islash);

	TdmsGroup *group = d_parser->getGroup(groupName);
	if (!group)
		return 0;

	return group->getChannel(channelName);
}

long long TdmsMetaData::getRawDataChunkSize()
{
	long long chunk_size = 0;
	for (TdmsObjectList::iterator object = objects.begin();  object != objects.end(); ++object){
		TdmsObject *obj = (*object);
		if (!obj)
			continue;

		if (obj->hasRawData())
			chunk_size += obj->getChannelSize();
	}
	return chunk_size;
}

TdmsObject* TdmsMetaData::readRawData(unsigned long long total_chunk_size, TdmsObject *prevObject)
{
	TdmsObject *lastObj = 0;
	unsigned long long fileSize = d_parser->fileSize();
	std::iendianfstream& file = d_parser->fileStream();

	long long chunk_size = getRawDataChunkSize();
	if (!chunk_size)
		return 0;

	unsigned int chunks = total_chunk_size/chunk_size;
	if (d_verbose)
		printf ("\tNumber of chunks: %d\n", (unsigned int)chunks);

	for (unsigned int i = 0; i < chunks; i++){
		for (TdmsObjectList::iterator object = objects.begin();  object != objects.end(); ++object){
			TdmsObject *obj = (*object);
			if (!obj)
				continue;

			if (obj->hasDAQmxData()){
				TdmsChannel *channel = obj->getChannel();
				if (!channel)
					channel = getChannel(obj);
				obj->readDAQmxData(channel);
			} else if (obj->hasRawData()){
				unsigned int index = obj->getRawDataIndex();
				if (index == 0)
					obj->setRawDataInfo(prevObject);
				else
					lastObj = obj;

				TdmsChannel *channel = obj->getChannel();
				if (!channel)
					channel = getChannel(obj);

				obj->readRawData(total_chunk_size, channel);
			}

			if ((unsigned long long)file.tellg() >= fileSize)
				break;
		}
	}
	return lastObj;
}

void TdmsMetaData::print() const
{
	std::cout << "\nRead meta data" << std::endl;
	std::cout << "  Contains " << objectCount << " objects." << std::endl;
}

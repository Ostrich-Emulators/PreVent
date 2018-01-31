#include "TdmsGroup.h"
#include "TdmsChannel.h"

TdmsGroup::TdmsGroup(const std::string & name)
:   name(name)
{
}

void TdmsGroup::addChannel(TdmsChannel* channel)
{
	channels.push_back(channel);
}

TdmsChannel* TdmsGroup::getChannel(const std::string &name)
{
	for (TdmsChannelSet::iterator iter = channels.begin(); iter != channels.end(); ++iter){
		if ((*iter)->getName() == name)
			return *iter;
	}
	return 0;
}

TdmsChannel* TdmsGroup::getChannel(unsigned int index) const
{
	if (index >= channels.size())
		return 0;

	return channels.at(index);
}

unsigned int TdmsGroup::getMaxValuesCount() const
{
	unsigned int rows = 0, channelsCount = channels.size();
	for (unsigned int i = 0; i < channelsCount; i++){
		TdmsChannel *ch = getChannel(i);
		if (!ch)
			continue;
		unsigned int dataType = ch->getDataType();
		unsigned int valuesCount = ((dataType == TdmsChannel::tdsTypeString) || (dataType == TdmsChannel::tdsTypeTimeStamp)) ?
										ch->getStringCount() : ch->getDataCount();
		if (valuesCount > rows)
			rows = valuesCount;
	}
	return rows;
}

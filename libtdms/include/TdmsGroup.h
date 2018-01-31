#ifndef TDMS_GROUP_H
#define TDMS_GROUP_H

#include <string>
#include <vector>
class TdmsChannel;

class TdmsGroup
{
public:
	typedef std::vector<TdmsChannel*> TdmsChannelSet;

	TdmsGroup(const std::string&);
	void addChannel(TdmsChannel*);

	const std::string& getName() const{return name;}
	unsigned int getGroupSize() const {return channels.size();}
	unsigned int getMaxValuesCount() const;
	const TdmsChannelSet& getChannels() const{return channels;}
	TdmsChannel* getChannel(const std::string &);
	TdmsChannel* getChannel(unsigned int) const;

private:
	const std::string name;
	TdmsChannelSet channels;
};
#endif

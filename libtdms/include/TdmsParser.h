#ifndef TDMS_PARSER_H
#define TDMS_PARSER_H

#include "endianfstream.hh"
#include <vector>
#include <map>

using namespace std;

class TdmsGroup;
class TdmsObject;
class TdmsChannel;

class TdmsParser
{
public:
	typedef vector<TdmsGroup*> TdmsGroupSet;

	TdmsParser(const std::string &);
	TdmsParser(char *);
	~TdmsParser();

	int fileOpeningError(){return file.is_open() ? 0 : 1;}

	void read(const bool verbose = false);

	std::iendianfstream& fileStream(){return file;}
	unsigned long long fileSize() const {return d_file_size;}

	TdmsGroup* getGroup(unsigned int) const;
	TdmsGroup* getGroup(const std::string &);
	void addGroup(TdmsGroup* group){groups.push_back(group);}

	unsigned int getGroupCount() const {return groups.size();}

	void setProperties(std::map<std::string, std::string> props){properties = props;}
	std::map<std::string, std::string> getProperties(){return properties;}
	string propertiesToString() const;

private:
	unsigned long long readSegment(const bool, bool *atEnd);
	void readRawData(unsigned long long, const bool);

	std::iendianfstream file;
	TdmsGroupSet groups;
	TdmsObject *d_prev_object;
	unsigned long long d_file_size;
	std::map<std::string, std::string> properties;
};

#endif
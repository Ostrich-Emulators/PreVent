/***************************************************************************
File                 : endianfstream.hh
--------------------------------------------------------------------
Copyright            : (C) 2008 Alex Kargovsky
					   Email (use @ for *)  : ion_vasilief*yahoo.fr
Description          : Endianless file stream class
***************************************************************************/

#ifndef IENDIAN_FSTREAM_H
#define IENDIAN_FSTREAM_H

#include <fstream>
#include <iostream>

namespace std
{
	class iendianfstream : public ifstream
	{
	public:
		iendianfstream(const char *_Filename, ios_base::openmode _Mode = ios_base::in)
			:	ifstream(_Filename, _Mode)
		{
			short word = 0x4321;
			bigEndian = (*(char*)& word) != 0x21;
		};

		iendianfstream& operator>>(bool& value)
		{
			char c;
			get(c);
			value = (c != 0);
			return *this;
		}

		iendianfstream& operator>>(char& value)
		{
			get(value);
			return *this;
		}

		iendianfstream& operator>>(unsigned char& value)
		{
			get(reinterpret_cast<char&>(value));
			return *this;
		}

		iendianfstream& operator>>(short& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(unsigned short& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(int& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(unsigned int& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(long& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(unsigned long& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(long long& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(unsigned long long& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(float& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(double& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(long double& value)
		{
			read(reinterpret_cast<char*>(&value), sizeof(value));
			if(bigEndian)
				swap_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));

			return *this;
		}

		iendianfstream& operator>>(string& value)
		{
			read(reinterpret_cast<char*>(&value[0]), value.size());
			string::size_type pos = value.find_first_of('\0');
			if(pos != string::npos)
				value.resize(pos);

			return *this;
		}

	private:
		bool bigEndian;
		void swap_bytes(unsigned char* data, int size)
		{
			int i = 0, j = size - 1;
			while(i < j)
			{
				std::swap(data[i], data[j]);
				++i, --j;
			}
		}
	};
}

#endif // ENDIAN_FSTREAM_H

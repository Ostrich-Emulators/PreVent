/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ZlReader2.h
 * Author: ryan
 *
 */

#ifndef STPREADER_H
#define STPREADER_H

#include "Reader.h"
#include "StreamChunkReader.h"
#include <map>
#include <string>
#include <memory>
#include <istream>
#include "zstr.hpp"
#include "CircularBuffer.h"

namespace FormatConverter {
	class SignalData;
	class StreamChunkReader;

	/**
	 * A reader for the HSDI signal files that UVA is producing
	 */
	class StpReader : public Reader {
	public:
		StpReader( );
		StpReader( const std::string& name );
		virtual ~StpReader( );

		static bool waveIsOk( const std::string& wavedata );
	protected:
		ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;
		int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
		void finish( ) override;

	private:
		StpReader( const StpReader& orig );
		ReadResult processOneChunk( std::unique_ptr<SignalSet>& );
		dr_time readTime( );
		std::string readString( size_t length );
		int readInt16( );
		int readInt8( );

		void readHrBlock( std::unique_ptr<SignalSet>& info );

		bool firstread;
		dr_time currentTime;
		zstr::ifstream * filestream;
		CircularBuffer<unsigned char> work;
		std::vector<unsigned char> decodebuffer;
	};
}
#endif /* STPREADER_H */


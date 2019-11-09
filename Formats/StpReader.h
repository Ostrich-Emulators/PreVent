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

		class BlockConfig {
		public:
			const bool isskip;
			const std::string label;
			const bool divBy10;
			const size_t readcount;
			const bool unsign;
			const std::string uom;

			static BlockConfig skip( size_t count = 1 ) {
				return BlockConfig( count );
			}

			static BlockConfig vital( const std::string& lbl, const std::string& uom,
					size_t read = 2, bool unsign = true ) {
				return BlockConfig( lbl, read, false, unsign, uom );
			}

			static BlockConfig div10( const std::string& lbl, const std::string& uom,
					size_t read = 2, bool unsign = true ) {
				return BlockConfig( lbl, read, true, unsign, uom );
			}

		private:

			BlockConfig( int read = 1 )
			: isskip( true ), label( "SKIP" ), divBy10( false ), readcount( read ), unsign( false ),
			uom( "Uncalib" ) {
			}

			BlockConfig( const std::string& lbl, size_t read = 2, bool div = false, bool unsign = true, const std::string& uom = "" )
			: isskip( false ), label( lbl ), divBy10( div ), readcount( read ), unsign( unsign ),
			uom( uom ) {
			}
		};

		static const BlockConfig SKIP;
		static const BlockConfig SKIP2;
		static const BlockConfig SKIP4;
		static const BlockConfig SKIP5;
		static const BlockConfig SKIP6;
		static const BlockConfig HR;
		static const BlockConfig PVC;
		static const BlockConfig STI;
		static const BlockConfig STII;
		static const BlockConfig STIII;
		static const BlockConfig STAVR;
		static const BlockConfig STAVL;
		static const BlockConfig STAVF;
		static const BlockConfig STV;
		static const BlockConfig BT;
		static const BlockConfig IT;
		static const BlockConfig RESP;
		static const BlockConfig APNEA;
		static const BlockConfig NBP_M;
		static const BlockConfig NBP_S;
		static const BlockConfig NBP_D;
		static const BlockConfig CUFF;
		static const BlockConfig AR1_M;
		static const BlockConfig AR1_S;
		static const BlockConfig AR1_D;
		static const BlockConfig AR1_R;
		static const BlockConfig AR2_M;
		static const BlockConfig AR2_S;
		static const BlockConfig AR2_D;
		static const BlockConfig AR2_R;
		static const BlockConfig AR3_M;
		static const BlockConfig AR3_S;
		static const BlockConfig AR3_D;
		static const BlockConfig AR3_R;
		static const BlockConfig AR4_M;
		static const BlockConfig AR4_S;
		static const BlockConfig AR4_D;
		static const BlockConfig AR4_R;

		static const BlockConfig SPO2_P;
		static const BlockConfig SPO2_R;
		static const BlockConfig VENT;
		static const BlockConfig IN_HLD;
		static const BlockConfig PRS_SUP;
		static const BlockConfig INSP_TM;
		static const BlockConfig INSP_PC;
		static const BlockConfig I_E;
		static const BlockConfig SET_PCP;
		static const BlockConfig SET_IE;
		static const BlockConfig APRV_LO_T;
		static const BlockConfig APRV_HI_T;
		static const BlockConfig APRV_LO;
		static const BlockConfig APRV_HI;
		static const BlockConfig RESIS;
		static const BlockConfig MEAS_PEEP;
		static const BlockConfig INTR_PEEP;
		static const BlockConfig INSP_TV;
		static const BlockConfig COMP;

		static const BlockConfig HF_FLW;
		static const BlockConfig B_FLW;
		static const BlockConfig FLW_TRIG;
		static const BlockConfig FLW_R;
		static const BlockConfig HF_R;
		static const BlockConfig HF_PRS;
		static const BlockConfig TMP_1;
		static const BlockConfig TMP_2;
		static const BlockConfig DELTA_TMP;
		static const BlockConfig LA1;
		static const BlockConfig CVP1;
		static const BlockConfig ICP1;
		static const BlockConfig CPP1;
		static const BlockConfig SP1;
		static const BlockConfig PA1_M;
		static const BlockConfig PA1_S;
		static const BlockConfig PA1_D;
		static const BlockConfig PA1_R;
		static const BlockConfig PA2_M;
		static const BlockConfig PA2_S;
		static const BlockConfig PA2_D;
		static const BlockConfig PA2_R;
		static const BlockConfig PA3_M;
		static const BlockConfig PA3_S;
		static const BlockConfig PA3_D;
		static const BlockConfig PA3_R;
		static const BlockConfig PA4_M;
		static const BlockConfig PA4_S;
		static const BlockConfig PA4_D;
		static const BlockConfig PA4_R;

		static const BlockConfig PT_RR;
		static const BlockConfig PEEP;
		static const BlockConfig MV;
		static const BlockConfig Fi02;
		static const BlockConfig TV;
		static const BlockConfig PIP;
		static const BlockConfig PPLAT;
		static const BlockConfig MAWP;
		static const BlockConfig SENS;
		static const BlockConfig SPONT_MV;
		static const BlockConfig SPONT_R;
		static const BlockConfig SET_TV;


		StpReader( const StpReader& orig );
		ReadResult processOneChunk( std::unique_ptr<SignalSet>& );
		dr_time readTime( );
		std::string readString( size_t length );
		int readInt16( );
		int readInt8( );
		unsigned int readUInt8( );
		unsigned int readUInt16( );

		void readDataBlock( std::unique_ptr<SignalSet>& info, const std::vector<BlockConfig>& vitals, size_t blocksize = 68 );
		static std::string div10s( int val );

		void unhandledBlockType( unsigned int type, unsigned int fmt ) const;

		bool firstread;
		dr_time currentTime;
		zstr::ifstream * filestream;
		CircularBuffer<unsigned char> work;
		std::vector<unsigned char> decodebuffer;
	};
}
#endif /* STPREADER_H */


#ifndef READER_H
#define READER_H

#include "Formats.h"

#include <map>
#include <string>
#include <memory>
#include <sstream>

#include "dr_time.h"
#include "SignalSet.h"
#include "TimeModifier.h"

namespace FormatConverter {
	class SignalData;
	class DataRow;

	enum ReadResult {
		FIRST_READ = 0,
		NORMAL = 1,
		END_OF_PATIENT = 2,
		END_OF_DAY = 3,
		END_OF_FILE = 4,
		ERROR = 5,
	};

	class Reader {
	public:
		Reader( const std::string& name );
		virtual ~Reader( );

		static std::unique_ptr<Reader> get( const Format& fmt );

		/**
		 * Prepares for reading a new input file/stream.
		 * @param input the file
		 * @param data reset this ReadInfo as well
		 * @return 0 (success), -1 (error), -2 (fatal)
		 */
		virtual int prepare( const std::string& input, std::unique_ptr<SignalSet>& info );

		/**
		 * Closes the current file/stream. This function must be called when
		 * the caller is finished with a file
		 */
		virtual void finish( );

		/**
		 * Fills the given ReadInfo with the next chunk of data from the input.
		 * A chunk is defined as one patient-day.
		 * @param read the data structure to populate with the newly-read data
		 * @param lastresult the outcome of the previous call to fill()
		 * @return the result code
		 */
		virtual ReadResult fill( std::unique_ptr<SignalSet>& read,
				const ReadResult& lastresult = ReadResult::FIRST_READ ) = 0;

		/**
		 *  Gets a user-friendly name for this reader
		 * @return a name to put in the attributes of any SignalSet created by
		 * this reader
		 */
		virtual std::string name( ) const;

		void setQuiet( bool quiet = true );
		void setNonbreaking( bool nb = false );
		void localizeTime( bool local = true );
		bool localizingTime( ) const;
		void timeModifier( const TimeModifier& mod );
		const TimeModifier& timeModifier( ) const;

		static bool strptime2( const std::string& input, const std::string& format,
				std::tm * tm );

		/**
		 * Decides if now is in a new day from then.  If then is 0, this function
		 * always returns false
		 * @param then the old time
		 * @param now the time to check
		 * @return true, if now and then are in different days
		 */
		bool isRollover(const dr_time& then, const dr_time& now) const;

		/**
		 * Gets root attributes from the given input. If this can be accomplished
		 * without reading the whole file, do it. else set the ok parameter to false
		 * @param inputfile
		 * @param map the map to fill with attributes
		 * @return true, if the read occurred
		 */
		virtual bool getAttributes( const std::string& inputfile, std::map<std::string, std::string>& map );

		/**
		 * Reads any data available in the given file between the given dates for the
		 * given signal, and writes it to the given SignalData. This function will
		 * set any properties in the SignalData to match the data.
		 * @param inputfile
		 * @param path
		 * @param from
		 * @param to
		 * @return
		 */
		virtual void splice( const std::string& inputfile, const std::string& path,
				dr_time from, dr_time to, std::unique_ptr<SignalData>& signalToFill );

	protected:
		Reader( const Reader& );

		bool nonbreaking( ) const;

		std::ostream& output( ) const;

		virtual
		dr_time modtime( const dr_time& time );

	private:
		bool largefile;
		const std::string rdrname;
		bool quiet;
		bool anon;
		bool onefile;
		bool local_time;
		std::stringstream ss;
		TimeModifier timemod;
	};
}
#endif /* READER_H */

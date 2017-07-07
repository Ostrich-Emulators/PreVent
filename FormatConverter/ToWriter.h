
#ifndef TO_WRITER_H
#define TO_WRITER_H

#include <utility>
#include <memory>

#include "Formats.h"

class ToWriter {
public:
	ToWriter( );
	virtual ~ToWriter( );

	static std::unique_ptr<ToWriter> get( const Format& fmt );

private:
	ToWriter( const ToWriter& );
};

#endif /* TO_WRITER_H */

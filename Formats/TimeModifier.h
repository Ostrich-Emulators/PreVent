/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TimeModifier.h
 * Author: ryan
 *
 * Created on August 6, 2019, 9:10 AM
 */

#ifndef TIMEMODIFIER_H
#define TIMEMODIFIER_H

#include "dr_time.h"

namespace FormatConverter {

	class TimeModifier {
	public:
		TimeModifier( );
		TimeModifier( const TimeModifier& model );

		virtual ~TimeModifier( );

		TimeModifier& operator=( const TimeModifier& orig );

		/**
		 * Modifies the given time with the appropriate offset.
		 * @param orig
		 * @return the converted time
		 */
		virtual dr_time convert( const dr_time& orig );

		virtual dr_time offset() const;
		/**
		 * Gets the first time converted
		 * @return
		 */
		virtual dr_time firsttime() const;

		/**
		 * Creates a TimeModifier that does not modify the time
		 * @return
		 */
		static TimeModifier passthru();

		/**
		 * Creates a TimeModifier that converts time such that its first converted
		 * time will be desiredFirsttime
		 * @param desiredFirstTime
		 * @return
		 */
		static TimeModifier time( const dr_time& desiredFirstTime );

		/**
		 * Creataes a TimeModifier with a pre-calculated offset.
		 * @param offset
		 * @return
		 */
		static TimeModifier offset( const dr_time& offset );

	private:
		/**
		 * Creates a TimeModifier with a pre-calculated offset
		 * @param inited if true, then offset is a pre-calculated offset. else this
		 * the same as TimeModifier( desiredFirstTime )
		 * @param offset the offset, or the desired first time, depending on
		 * the value of inited.
		 */
		TimeModifier( bool inited, dr_time offset = 0 );

		bool initialized;
		bool used;
		dr_time _offset;
		dr_time _first;
	};
}

#endif /* TIMEMODIFIER_H */


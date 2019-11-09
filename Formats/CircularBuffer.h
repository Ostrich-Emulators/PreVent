/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CircularBuffer.h
 * Author: ryan
 * mostly from
 * https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
 * with slight modifications
 * Created on November 8, 2019, 10:04 AM
 */

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <vector>
#include <stdexcept>

namespace FormatConverter {

	template <class T>
	class CircularBuffer {
	public:

		explicit CircularBuffer( size_t size ) :
		buffer( std::unique_ptr<T[]>( new T[size] ) ),
		maxsize( size ), head( 0 ), tail( 0 ), _full( false ), _mark( 0 ) {
		}

		void push( T item ) {
			if ( _full ) {
				throw std::runtime_error( "no room at the inn!" );
			}

			buffer[head] = item;
			head = ( head + 1 ) % maxsize;
			_full = ( head == tail );
		}

		T pop( ) {
			if ( empty( ) ) {
				throw std::runtime_error( "cannot get water from a stone!" );
			}

			auto val = buffer[tail];
			_full = false;
			tail = ( tail + 1 ) % maxsize;
			_mark++;
			return val;
		}

		T read( size_t offsetFromTail = 0 ) const {
			if ( offsetFromTail > size( ) ) {
				throw std::runtime_error( "cannot read a book that hasn't been written" );
			}

			return buffer[( tail + offsetFromTail ) % maxsize];
		}

		std::vector<T> popvec( size_t size = 1 ) {
			std::vector<T> data;
			data.reserve( size );
			for ( size_t i = 0; i < size; i++ ) {
				data.push_back( pop( ) );
			}
			return data;
		}

		void skip( size_t skipped ) {
			for ( size_t i = 0; i < skipped; i++ ) {
				pop( );
			}
		}

		void rewind( size_t steps = 1 ) {
			if ( steps > size( ) ) {
				throw std::runtime_error( "rewind will exceed buffer size" );
			}

			tail = ( tail + maxsize - steps ) % maxsize;
			_full = ( head == tail );
			_mark -= steps;
		}

		void rewindToMark( ) {
			rewind( _mark );
		}

		void mark( ) {
			_mark = 0;
		}

		size_t readSinceMark( ) const {
			return _mark;
		}

		void reset( ) {
			head = tail;
			_full = false;
		}

		bool empty( ) const {
			return ( !_full && head == tail );
		}

		bool full( ) const {
			return _full;
		}

		size_t capacity( ) const {
			return maxsize;
		}

		size_t size( ) const {
			size_t size = maxsize;
			if ( !_full ) {
				size = ( head >= tail
						? head - tail
						: maxsize + head - tail );
			}
			return size;
		}

	private:
		std::unique_ptr < T[] > buffer;
		const size_t maxsize;
		size_t head;
		size_t tail;
		bool _full;
		size_t _mark;
	};
}
#endif /* CIRCULARBUFFER_H */


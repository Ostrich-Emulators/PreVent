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
#include <memory>
#include <stdexcept>

namespace FormatConverter {

  template <class T>
  class CircularBuffer {
  public:

    explicit CircularBuffer( size_t size ) :
        buffer( std::make_unique<T[]>( size ) ),
        maxsize( size ), head( 0 ), tail( 0 ), _full( false ), _mark( 0 ),
        _pushed( 0 ), _popped( 0 ) { }

    void push( T item ) {
      if ( _full ) {
        throw std::runtime_error( "no room at the inn!" );
      }

      buffer[head] = item;
      head = ( head + 1 ) % maxsize;
      _full = ( head == tail );
      _pushed++;
    }

    T pop( ) {
      if ( empty( ) ) {
        throw std::runtime_error( "cannot get water from a stone!" );
      }

      auto val = buffer[tail];
      _full = false;
      tail = ( tail + 1 ) % maxsize;
      _mark++;
      _popped++;
      return val;
    }

    T read( int offsetFromTail = 0 ) const {
      if ( offsetFromTail > 0 && (unsigned int) offsetFromTail > size( ) ) {
        throw std::runtime_error( "cannot read a book that hasn't been written" );
      }
      else if ( offsetFromTail < 0 && (unsigned int) ( -offsetFromTail ) > available( ) ) {
        throw std::runtime_error( "too much water under that bridge" );
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

    void skip( size_t skipped = 1 ) {
      for ( size_t i = 0; i < skipped; i++ ) {
        pop( );
      }
    }

    void rewind( size_t steps = 1 ) {
      if ( steps > available( ) ) {
        throw std::runtime_error( "rewind will exceed buffer size" );
      }

      tail = ( tail + maxsize - steps ) % maxsize;
      _full = ( head == tail );
      _mark -= steps;
      _popped -= steps;
    }

    void rewindToMark( ) {
      rewind( _mark );
    }

    void mark( ) {
      _mark = 0;
    }

    size_t poppedSinceMark( ) const {
      return _mark;
    }

    void reset( ) {
      head = tail;
      _full = false;
      _popped = 0;
      _pushed = 0;
      _mark = 0;
    }

    bool empty( ) const {
      return ( !_full && head == tail );
    }

    bool full( ) const {
      return _full;
    }

    size_t pushed( ) const {
      return _pushed;
    }

    size_t popped( ) const {
      return _popped;
    }

    size_t capacity( ) const {
      return maxsize;
    }

    size_t available( ) const {
      return capacity( ) - size( );
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
    size_t _pushed;
    size_t _popped;
  };
}
#endif /* CIRCULARBUFFER_H */


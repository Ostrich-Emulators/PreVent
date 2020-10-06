/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TimeRange.h
 * Author: ryan
 *
 * Created on June 2, 2020, 8:54 AM
 */

#ifndef FILECACHINGVECTOR_H
#define FILECACHINGVECTOR_H

#include <cstdio>
#include <iterator>
#include <vector>

#include "SignalUtils.h"
#include "Options.h"

namespace FormatConverter {

  template <class T> class FileCachingVector {
  public:

    class iterator {
    public:
      using difference_type = T;
      using value_type = T;
      using pointer = const T*;
      using reference = const T&;
      using iterator_category = std::bidirectional_iterator_tag;
      //using iterator_category = std::forward_iterator_tag;

      iterator( FileCachingVector * owner, size_t pos ) : owner( owner ), idx( pos ) { }

      iterator& operator=(const iterator& orig ) {
        if ( this != &orig ) {
          this->owner = orig.owner;
          this->idx = orig.idx;
        }
        return *this;
      }

      virtual ~iterator( ) { }

      iterator& operator++( ) {
        idx++;
        return *this;
      }

      iterator operator++(int) {
        auto tmp = iterator( this->owner, idx );
        idx++;
        return tmp;
      }

      iterator& operator--( ) {
        idx--;
        return *this;
      }

      iterator operator--(int) {
        auto tmp = iterator( this->owner, idx );
        idx--;
        return tmp;
      }

      bool operator==( iterator& other ) const {
        return ( this->owner == other.owner && this->idx == other.idx );
      }

      bool operator!=( iterator& other ) const {
        return !operator==( other );
      }

      T operator*( ) {
        return owner->value_at( idx );
      }

    private:
      FileCachingVector * owner;
      size_t idx;
    };

    static const inline size_t DEFAULT_CACHE_LIMIT = 1024 * 384;

    FileCachingVector( ) : cachedata( Options::asBool( OptionsKey::NOCACHE ) ? nullptr : SignalUtils::tmpf( ) ),
        sizer( 0 ), memrange( 0, 0 ), dirty( false ), maxelements( DEFAULT_CACHE_LIMIT ) { }

    virtual ~FileCachingVector( ) { }

    iterator begin( ) {
      return iterator( this, 0 );
    }

    iterator end( ) {
      return iterator( this, sizer );
    }

    void push_back( const std::vector<T>& vec ) {
      values.insert( values.end( ), vec.begin( ), vec.end( ) );
      sizer += vec.size( );
      memrange.second += vec.size( );
      dirty = true;
      cache_if_needed( );
    }

    void push_back( T t ) {
      values.push_back( t );
      sizer++;
      memrange.second++;
      dirty = true;
      cache_if_needed( );
    }

    size_t size( ) const {
      return sizer;
    }

    /**
     * Appends the values from this time range to the given vector. The vector
     * is not otherwise altered during this function
     * @param vec the vector to fill with values
     * @param startidx the first time index to add (inclusive)
     * @param stop the index to stop at (exclusive)
     */
    void fill( std::vector<T>& vec, size_t startidx, size_t stopidx ) {
      if ( stopidx >= sizer ) {
        stopidx = sizer;
      }
      const auto diff = stopidx - startidx;
      vec.reserve( vec.size( ) + diff );

      for (; startidx < stopidx; startidx++ ) {
        vec.push_back( value_at( startidx ) );
      }
    }
  protected:

    /**
     * Extension point
     * @param values
     * @param file
     * @return
     */
    virtual int _cache( std::vector<T>& values, FILE * file ) {
      return std::fwrite( values.data( ), sizeof ( T ), values.size( ), file );
    }

    /**
     * Extension point
     * @param values
     * @param file
     * @return
     */
    virtual int _uncache( std::vector<T>& values, size_t maxreads, size_t elementsize, FILE * file ) {
      return std::fread( values.data( ), elementsize, maxreads, file );
    }


  private:

    T value_at( size_t idx ) {
      if ( idx > sizer ) {
        return std::numeric_limits<T>::max( );
      }

      if ( idx < memrange.first || idx >= memrange.second ) {
        // time isn't in the memory cache, so we need to uncache it
        uncache( idx );
      }

      return values[idx - memrange.first];
    }

    virtual bool cache_if_needed( bool force = false ) {
      if ( cachedata && ( values.size( ) >= maxelements || force ) ) {
        _cache( values, cachedata->file );
        values.clear( );
        memrange.first = sizer;
        memrange.second = sizer;
        dirty = false;
      }

      return true;
    }

    virtual void uncache( size_t fromidx ) {
      if ( dirty ) {
        cache_if_needed( true );
      }

      const auto SIZE = sizeof ( T );
      auto filepos = fromidx * SIZE;
      values.clear( );
      std::fseek( cachedata->file, filepos, SEEK_SET );
      auto reads = _uncache( values, maxelements, SIZE, cachedata->file );
      memrange.first = fromidx;
      memrange.second = fromidx + reads;
    }

    std::unique_ptr<CachefileData> cachedata;
    size_t sizer;
    std::vector<T> values;
    std::pair<size_t, size_t> memrange;
    bool dirty;
    size_t maxelements;
  };
}
#endif /* FILECACHINGVECTOR_H */


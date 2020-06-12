/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BSICache.h
 * Author: ryan
 *
 * Created on June 2, 2020, 8:54 AM
 */

#ifndef BSICache_H
#define BSICache_H

#include <cstdio>
#include <iterator>
#include <vector>

namespace FormatConverter {

  class BSICache {
  public:
    static const size_t DEFAULT_CACHE_LIMIT;

    class BSICacheIterator {
    public:
      using difference_type = double;
      using value_type = double;
      using pointer = const double*;
      using reference = const double&;
      //using iterator_category = std::bidirectional_iterator_tag;
      using iterator_category = std::forward_iterator_tag;

      BSICacheIterator( BSICache * owner, size_t pos );
      BSICacheIterator& operator=(const BSICacheIterator&);
      virtual ~BSICacheIterator( );

      BSICacheIterator& operator++( );
      BSICacheIterator operator++(int);
      bool operator==( BSICacheIterator& other ) const;
      bool operator!=( BSICacheIterator& other ) const;
      double operator*( );

    private:
      BSICache * owner;
      size_t idx;
    };

    BSICache( const std::string& name = "" );
    virtual ~BSICache( );

    BSICacheIterator begin( );
    BSICacheIterator end( );

    void push_back( const std::vector<double>& vec );
    void push_back( double t );

    size_t size( ) const;
    /**
     * Appends the values from this time range to the given vector. The vector
     * is not otherwise altered during this function
     * @param vec the vector to fill with values
     * @param startidx the first time index to add (inclusive)
     * @param stop the index to stop at (exclusive)
     */
    void fill( std::vector<double>& vec, size_t startidx, size_t stopidx );

    void name( const std::string& name );
    const std::string& name( ) const;

  private:
    double value_at( size_t idx );
    bool cache_if_needed( bool force = false );
    void uncache( size_t fromidx );

    BSICache& operator=(const BSICache&);
    FILE * cache;
    size_t sizer;
    std::vector<double> values;
    std::pair<size_t, size_t> memrange;
    bool dirty;
    std::string _name;
  };
}
#endif /* BSICache_H */


/*
 * std::unique_ptr<SignalData>o change this license header, choose License Headers in Project Properties.
 * std::unique_ptr<SignalData>o change this template file, choose std::unique_ptr<SignalData>ools | std::unique_ptr<SignalData>emplates
 * and open the template in the editor.
 */

/* 
 * File:   SignalDataIterator.h
 * Author: ryan
 *
 * Created on July 3, 2018, 3:40 PM
 */

#ifndef SIGNALDATAITERATOR_H
#define SIGNALDATAITERATOR_H

#include <vector>
#include <memory>

#include "SignalData.h"

class SignalDataIterator : public std::iterator<std::random_access_iterator_tag, SignalData> {
public:

  SignalDataIterator( std::vector<std::unique_ptr<SignalData>> *vec, int idx = -1,
      std::vector<std::unique_ptr<SignalData>> *vec2 = nullptr )
  : loc( idx ) {
    for ( auto& x : *vec ) {
      vector.push_back( std::ref( x ) );
    }
    if ( nullptr != vec2 ) {
      for ( auto& x : *vec2 ) {
        vector.push_back( std::ref( x ) );
      }
    }
  }

  SignalDataIterator( std::vector<std::reference_wrapper<std::unique_ptr<SignalData>>> *vec, int idx = -1,
      std::vector<std::reference_wrapper<std::unique_ptr<SignalData>>> *vec2 = nullptr )
  : loc( idx ) {
    for ( auto x : *vec ) {
      vector.push_back( x );
    }
    if ( nullptr != vec2 ) {
      for ( auto x : *vec2 ) {
        vector.push_back( x );
      }
    }
  }

  ~SignalDataIterator( ) {
  }

  SignalDataIterator& operator=(const SignalDataIterator& rawIterator ) {
    if( this != &rawIterator ){
      loc = rawIterator.loc;
      vector = rawIterator.vector;
    }
    return *this;
  }

  operator bool( ) const {
    return ( loc >-1 && loc < vector.size( ) );
  }

  bool operator==(const SignalDataIterator& rawIterator )const {
    //return (loc == rawIterator.loc && vector == rawIterator.vector );
    return (loc == rawIterator.loc );
  }

  bool operator!=(const SignalDataIterator& rawIterator )const {
    //return !( loc == rawIterator.loc && vector == rawIterator.vector );
    return !( loc == rawIterator.loc );
  }

  SignalDataIterator& operator+=(const int& movement ) {
    loc += movement;
    return (*this );
  }

  SignalDataIterator& operator-=(const int& movement ) {
    loc -= movement;
    return (*this );
  }

  SignalDataIterator& operator++( ) {
    ++loc;
    return (*this );
  }

  SignalDataIterator& operator--( ) {
    --loc;
    return (*this );
  }

  SignalDataIterator operator++(int) {
    auto temp( *this );
    ++loc;
    return temp;
  }

  SignalDataIterator operator--(int) {
    auto temp( *this );
    --loc;
    return temp;
  }

  SignalDataIterator operator+(const int& movement ) {
    auto oldPtr = loc;
    loc += movement;
    auto temp( *this );
    loc = oldPtr;
    return temp;
  }

  SignalDataIterator operator-(const int& movement ) {
    auto oldPtr = loc;
    loc -= movement;
    auto temp( *this );
    loc = oldPtr;
    return temp;
  }

  int operator-(const SignalDataIterator& rawIterator ) {
    return rawIterator.loc - loc;
  }

  std::unique_ptr<SignalData>& operator*( ) {
    return vector.at( loc ).get( );
  }

  const std::unique_ptr<SignalData>& operator*( ) const {
    return vector.at( loc ).get( );
  }

  std::unique_ptr<SignalData>* operator->( ) {
    return &( vector.at( loc ).get( ) );
  }

protected:
  int loc;
  std::vector<std::reference_wrapper<std::unique_ptr<SignalData>>> vector;
};

class PartionedSignalData {
public:
  PartionedSignalData( std::vector<std::unique_ptr<SignalData>>&vec ) : vector( vec ) {
  }

  ~PartionedSignalData( ) {
  }

  SignalDataIterator begin( ) {
    return SignalDataIterator( &vector );
  }

  SignalDataIterator end( ) {
    return SignalDataIterator( &vector, vector.size( ) );
  }

private:
  std::vector<std::unique_ptr<SignalData>>&vector;
};
#endif /* SIGNALDATAITERATOR_H */


// *****************************************************************************
/*!
  \file      tests/unit/Control/Options/TestMKLGaussianMethod.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Unit tests for Control/Options/MKLGaussianMethod
  \details   Unit tests for Control/Options/MKLGaussianMethod
*/
// *****************************************************************************

#include "NoWarning/tut.hpp"

#include "TUTConfig.hpp"
#include "Options/MKLGaussianMethod.hpp"

#ifndef DOXYGEN_GENERATING_OUTPUT

namespace tut {

//! All tests in group inherited from this base
struct MKLGaussianMethod_common {
  MKLGaussianMethod_common() : m() {}
  const tk::ctr::MKLGaussianMethod m;
};

//! Test group shortcuts
using MKLGaussianMethod_group =
  test_group< MKLGaussianMethod_common, MAX_TESTS_IN_GROUP >;
using MKLGaussianMethod_object = MKLGaussianMethod_group::object;

//! Define test group
static MKLGaussianMethod_group
 MKLGaussianMethod( "Control/Options/MKLGaussianMethod" );

//! Test definitions for group

//! Test that member function param() finds MKL parameter for method type
template<> template<>
void MKLGaussianMethod_object::test< 1 >() {
  set_test_name( "param() finds MKL param" );
  ensure( "cannot find parameter",
          m.param( tk::ctr::MKLGaussianMethodType::BOXMULLER2 ) ==
            VSL_RNG_METHOD_GAUSSIAN_BOXMULLER2 );
}

//! Test that member function param() throws in DEBUG mode if can't find param
template<> template<>
void MKLGaussianMethod_object::test< 2 >() {
  set_test_name( "param() throws" );

  try {

    m.param( static_cast< tk::ctr::MKLGaussianMethodType >( 234 ) );
    #ifndef NDEBUG
    fail( "should throw exception in DEBUG mode" );
    #endif

  } catch ( tk::Exception& e ) {
    // exception thrown in DEBUG mode, test ok, Assert skipped in RELEASE mode
    // if any other type of exception is thrown, test fails with except
    ensure( std::string("wrong exception thrown: ") + e.what(),
            std::string( e.what() ).find( "Cannot find parameter" ) !=
              std::string::npos );
  }
}

//! Test copy constructor
template<> template<>
void MKLGaussianMethod_object::test< 3 >() {
  set_test_name( "copy constructor" );

  tk::ctr::MKLGaussianMethod c( m );
  std::vector< tk::ctr::MKLGaussianMethod > v;
  v.push_back( c );
  ensure( "copy constructor used to push_back a MKLGaussianMethod object to a "
          "std::vector",
          v[0].param( tk::ctr::MKLGaussianMethodType::BOXMULLER ) ==
            VSL_RNG_METHOD_GAUSSIAN_BOXMULLER );
}

//! Test move constructor
template<> template<>
void MKLGaussianMethod_object::test< 4 >() {
  set_test_name( "move constructor" );

  tk::ctr::MKLGaussianMethod c( m );
  std::vector< tk::ctr::MKLGaussianMethod > v;
  v.emplace_back( std::move(c) );
  ensure( "move constructor used to emplace_back a MKLGaussianMethod object to "
          "a std::vector",
           v[0].param( tk::ctr::MKLGaussianMethodType::BOXMULLER ) ==
             VSL_RNG_METHOD_GAUSSIAN_BOXMULLER );
}

//! Test copy assignment
template<> template<>
void MKLGaussianMethod_object::test< 5 >() {
  set_test_name( "copy assignment" );

  tk::ctr::MKLGaussianMethod c;
  c = m;
  ensure( "find param of copy-assigned MKLGaussianMethod",
          c.param( tk::ctr::MKLGaussianMethodType::ICDF ) ==
             VSL_RNG_METHOD_GAUSSIAN_ICDF );
}

//! Test move assignment
template<> template<>
void MKLGaussianMethod_object::test< 6 >() {
  set_test_name( "move assignment" );

  tk::ctr::MKLGaussianMethod c;
  c = std::move( m );
  ensure( "find param of move-assigned MKLGaussianMethod",
          c.param( tk::ctr::MKLGaussianMethodType::ICDF ) ==
             VSL_RNG_METHOD_GAUSSIAN_ICDF );
}

} // tut::

#endif  // DOXYGEN_GENERATING_OUTPUT

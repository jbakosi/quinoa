// *****************************************************************************
/*!
  \file      tests/unit/LoadBalance/TestLinearMap.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Unit tests for LoadBalance/LinearMap
  \details   Unit tests for LoadBalance/LinearMap
*/
// *****************************************************************************

#include "NoWarning/tut.hpp"

#include "TUTConfig.hpp"
#include "LinearMap.hpp"
#include "TestArray.hpp"

namespace unittest {

extern std::string g_executable;

} // unittest::

#ifndef DOXYGEN_GENERATING_OUTPUT

namespace tut {

//! All tests in group inherited from this base
struct LinearMap_common {};

//! Test group shortcuts
using LinearMap_group =
  test_group< LinearMap_common, MAX_TESTS_IN_GROUP >;
using LinearMap_object = LinearMap_group::object;

//! Define test group
static LinearMap_group LinearMap( "LoadBalance/LinearMap" );

//! Test definitions for group

//! Test if constructor does not throw on positive number of elements
template<> template<>
void LinearMap_object::test< 1 >() {
  set_test_name( "ctor doesn't throw on positive nelem" );
  tk::CProxy_LinearMap::ckNew( 2 );
}

//! Test use of LinearMap creating an array with nchare <= numpes
template<> template<>
void LinearMap_object::test< 2 >() {
  int nchare = CkNumPes() > 1 ? CkNumPes()/2 : 1;
  set_test_name( "use with nchare (" + std::to_string(nchare) + ") <= numpes ("
                 + std::to_string(CkNumPes()) + ")" );

  // Create linear map chare
  auto map = tk::CProxy_LinearMap::ckNew( nchare );

  // Create array options object for use with linear map chare
  CkArrayOptions opts( nchare );
  opts.setMap( map );

  // Create chare array using linear map
  CProxy_TestArray arrayproxy = CProxy_TestArray::ckNew( opts );
  arrayproxy.doneInserting();

  // If this test fails it will spew errors on the screen...
}

//! Test use of LinearMap creating an array with nchare > numpes
template<> template<>
void LinearMap_object::test< 3 >() {
  int nchare = 2 * CkNumPes();
  set_test_name( "use with nchare (" + std::to_string(nchare) + ") > numpes ("
                 + std::to_string(CkNumPes()) + ")" );

  // Create linear map chare
  auto map = tk::CProxy_LinearMap::ckNew( nchare );

  // Create array options object for use with linear map chare
  CkArrayOptions opts( nchare );
  opts.setMap( map );

  // Create chare array using linear map
  CProxy_TestArray arrayproxy = CProxy_TestArray::ckNew( opts );
  arrayproxy.doneInserting();

  // If this test fails it will spew errors on the screen...
}

} // tut::

#endif  // DOXYGEN_GENERATING_OUTPUT

#include "NoWarning/testarray.def.h"

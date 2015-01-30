//******************************************************************************
/*!
  \file      src/UnitTest/tests/Base/MigratedTypes.h
  \author    J. Bakosi
  \date      Tue 16 Sep 2014 08:14:00 AM MDT
  \copyright 2012-2015, Jozsef Bakosi.
  \brief     Type definitions for unit tests of Charm++ migrated data
  \details   Type definitions for unit tests of Charm++ migrated data
*/
//******************************************************************************
#ifndef test_MigratedTypes_h
#define test_MigratedTypes_h

#include <PUPUtil.h>

//! Unit test declarations and definitions
namespace tut {
//! Unit test declarations and definitions for testing Charm++ chares
namespace charm {

// Tested types migrated
enum class Enum_default { F1=32, F2 };
enum class Enum_uint8_t : uint8_t { F1=14, F2 };
enum Enum_cstyle { F1=54, F2 };
using Pair = std::pair< int, double >;
using Vector = std::vector< std::string >;
using Tuple = std::tuple< int, double, std::vector< std::string >,
                          Enum_default, std::map< Enum_uint8_t, std::string > >;

//! Pack/Unpack: delegate to tk::
inline void operator|( PUP::er& p, Enum_default& e ) { PUP::pup( p, e ); }
inline void operator|( PUP::er& p, Enum_uint8_t& e ) { PUP::pup( p, e ); }
inline void operator|( PUP::er& p, Enum_cstyle& e ) { PUP::pup( p, e ); }
inline void operator|( PUP::er& p, Tuple& t ) { PUP::pup( p, t ); }

} // charm::
} // tut::

#endif // test_MigratedTypes_h

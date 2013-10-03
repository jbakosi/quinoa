//******************************************************************************
/*!
  \file      src/Control/Quinoa/Options/Position.h
  \author    J. Bakosi
  \date      Thu Oct  3 17:40:53 2013
  \copyright Copyright 2005-2012, Jozsef Bakosi, All rights reserved.
  \brief     Position model options and associations
  \details   Position model options and associations
*/
//******************************************************************************
#ifndef QuinoaPositionOptions_h
#define QuinoaPositionOptions_h

#include <map>

#include <Toggle.h>
#include <Quinoa/InputDeck/Keywords.h>

namespace quinoa {
namespace sel {

//! Position model types
enum class PositionType : uint8_t { NO_POSITION=0,
                                    INVISCID,
                                    VISCOUS };

//! Class with base templated on the above enum class with associations
class Position : public Toggle<PositionType> {

  public:
    //! Constructor: pass associations references to base, which will handle
    //! class-user interactions
    explicit Position() : Toggle<PositionType>("Position", names, values) {}

  private:
    //! Don't permit copy constructor
    Position(const Position&) = delete;
    //! Don't permit copy assigment
    Position& operator=(const Position&) = delete;
    //! Don't permit move constructor
    Position(Position&&) = delete;
    //! Don't permit move assigment
    Position& operator=(Position&&) = delete;

    //! Get access to position keywords
    const kw::pos_inviscid inviscid {};
    const kw::pos_viscous viscous {};

    //! Enums -> names
    const std::map<PositionType, std::string> names {
      { PositionType::NO_POSITION, "n/a" },
      { PositionType::INVISCID, inviscid.name() },
      { PositionType::VISCOUS, viscous.name() }
    };

    //! keywords -> Enums
    const std::map<std::string, PositionType> values {
      { "no_position", PositionType::NO_POSITION },
      { inviscid.string(), PositionType::INVISCID },
      { viscous.string(), PositionType::INVISCID }
    };
};

} // sel::
} // quinoa:::

#endif // QuinoaPositionOptions_h
//******************************************************************************
/*!
  \file      src/Control/Quinoa/Options/Mix.h
  \author    J. Bakosi
  \date      Thu Oct  3 17:40:32 2013
  \copyright Copyright 2005-2012, Jozsef Bakosi, All rights reserved.
  \brief     Mix model options and associations
  \details   Mix model options and associations
*/
//******************************************************************************
#ifndef QuinoaMixOptions_h
#define QuinoaMixOptions_h

#include <map>

#include <Toggle.h>
#include <Quinoa/InputDeck/Keywords.h>

namespace quinoa {
namespace sel {

//! Mix model types
enum class MixType : uint8_t { NO_MIX=0,
                               IEM,
                               IECM,
                               DIRICHLET,
                               GENERALIZED_DIRICHLET };

//! Class with base templated on the above enum class with associations
class Mix : public Toggle<MixType> {

  public:
    //! Constructor: pass associations references to base, which will handle
    //! class-user interactions
    explicit Mix() : Toggle<MixType>("Material mix", names, values) {}

  private:
    //! Don't permit copy constructor
    Mix(const Mix&) = delete;
    //! Don't permit copy assigment
    Mix& operator=(const Mix&) = delete;
    //! Don't permit move constructor
    Mix(Mix&&) = delete;
    //! Don't permit move assigment
    Mix& operator=(Mix&&) = delete;

    //! Get access to mix keywords
    const kw::mix_iem iem {};
    const kw::mix_iecm iecm {};
    const kw::mix_dir dir {};
    const kw::mix_gendir gendir {};

    //! Enums -> names
    const std::map<MixType, std::string> names {
      { MixType::NO_MIX, "n/a" },
      { MixType::IEM, iem.name() },
      { MixType::IECM, iecm.name() },
      { MixType::DIRICHLET, dir.name() },
      { MixType::GENERALIZED_DIRICHLET, gendir.name() }
    };

    //! keywords -> Enums
    const std::map<std::string, MixType> values {
      { "no_mix", MixType::NO_MIX },
      { iem.string(), MixType::IEM },
      { iecm.string(), MixType::IECM },
      { dir.string(), MixType::DIRICHLET },
      { gendir.string(), MixType::GENERALIZED_DIRICHLET }
    };
};

} // sel::
} // quinoa::

#endif // QuinoaMixOptions_h
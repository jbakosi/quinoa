//******************************************************************************
/*!
  \file      src/Control/Options/RNGSSESeqLen.h
  \author    J. Bakosi
  \date      Thu 16 Jan 2014 08:58:24 PM MST
  \copyright 2005-2014, Jozsef Bakosi.
  \brief     RNGSSE sequence length options
  \details   RNGSSE sequence length options
*/
//******************************************************************************
#ifndef RNGSSESeqLenOptions_h
#define RNGSSESeqLenOptions_h

#include <map>

#include <Toggle.h>
#include <Quinoa/InputDeck/Keywords.h>

namespace tk {
namespace ctr {

//! RNGSSE's sequence length options
enum class RNGSSESeqLenType : uint8_t { SHORT,
                                        MEDIUM,
                                        LONG };

//! Class with base templated on the above enum class with associations
class RNGSSESeqLen : public tk::Toggle< RNGSSESeqLenType > {

  public:
    //! Constructor: pass associations references to base, which will handle
    //! class-user interactions
    explicit RNGSSESeqLen() :
      Toggle< RNGSSESeqLenType >("sequence length", names, values) {}

  private:
    //! Don't permit copy constructor
    RNGSSESeqLen(const RNGSSESeqLen&) = delete;
    //! Don't permit copy assigment
    RNGSSESeqLen& operator=(const RNGSSESeqLen&) = delete;
    //! Don't permit move constructor
    RNGSSESeqLen(RNGSSESeqLen&&) = delete;
    //! Don't permit move assigment
    RNGSSESeqLen& operator=(RNGSSESeqLen&&) = delete;

    //! Get access to RNGSSE sequence length keywords
    const tk::kw::seq_short seq_short {};
    const tk::kw::seq_med seq_med {};
    const tk::kw::seq_long seq_long {};

    //! Enums -> names
    const std::map< RNGSSESeqLenType, std::string > names {
      { RNGSSESeqLenType::SHORT, seq_short.name() },
      { RNGSSESeqLenType::MEDIUM, seq_med.name() },
      { RNGSSESeqLenType::LONG, seq_long.name() }
    };

    //! keywords -> Enums
    const std::map< std::string, RNGSSESeqLenType > values {
      { seq_short.string(), RNGSSESeqLenType::SHORT },
      { seq_med.string(), RNGSSESeqLenType::MEDIUM },
      { seq_long.string(), RNGSSESeqLenType::LONG }
    };
};

} // ctr::
} // tk::

#endif // RNGSSESeqLenOptions_h

//******************************************************************************
/*!
  \file      src/Parser/ParserException.h
  \author    J. Bakosi
  \date      Mon 21 Jan 2013 08:39:58 PM MST
  \copyright Copyright 2005-2012, Jozsef Bakosi, All rights reserved.
  \brief     ParserException
  \details   ParserException
*/
//******************************************************************************
#ifndef ParserException_h
#define ParserException_h

#include <string>

using namespace std;

#include <Exception.h>

namespace Quinoa {

//! Parser exception types
enum ParserExceptType { CMDLINE_EXCEPT=0,           //!< Command line exception
                        NUM_PARSER_EXCEPT
};

//! Parser exception error messages
const string ParserMsg[NUM_PARSER_EXCEPT] = {
  "Exactly one command line argument required: filename.q"
};

//! ParserException : Exception
class ParserException : public Exception {

  public:
    //! Constructor
    ParserException(ExceptType except,
                    ParserExceptType parserExcept,
                    const string& file,
                    const string& func,
                    const unsigned int& line) :
      Exception(except, file, func, line), m_except(parserExcept) {}

    //! Move constructor, necessary for throws, default compiler generated
    ParserException(ParserException&&) = default;

    //! Don't permit copy constructor
    // ICC: should be deleted and private
    ParserException(const ParserException&);

    //! Destructor
    virtual ~ParserException() {}

    //! Handle ParserException
    virtual ErrCode handleException(Driver* driver);

  private:
    //! Don't permit copy assignment
    ParserException& operator=(const ParserException&) = delete;
    //! Don't permit move assignment
    ParserException& operator=(ParserException&&) = delete;

    //! Parser exception type (MIXMODEL, etc.)
    ParserExceptType m_except;
};

} // namespace Quinoa

#endif // ParserException_h

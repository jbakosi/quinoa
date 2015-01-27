//******************************************************************************
/*!
  \file      src/Control/Quinoa/Options/Mix.h
  \author    J. Bakosi
  \date      Wed 21 Jan 2015 08:00:19 AM MST
  \copyright 2012-2014, Jozsef Bakosi.
  \brief     Mix model options
  \details   Mix model options
*/
//******************************************************************************
#ifndef QuinoaMixOptions_h
#define QuinoaMixOptions_h

#include <boost/mpl/vector.hpp>

#include <Toggle.h>
#include <Keywords.h>
#include <PUPUtil.h>

namespace quinoa {
namespace ctr {

//! Mix model types
//! \author J. Bakosi
enum class MixType : uint8_t { NO_MIX=0,
                               IEM,
                               IECM,
                               DIRICHLET,
                               GENDIR };

//! \brief Pack/Unpack MixType: forward overload to generic enum class packer
//! \author J. Bakosi
inline void operator|( PUP::er& p, MixType& e ) { PUP::pup( p, e ); }

//! \brief Mix model options: outsource searches to base templated on enum type
//! \author J. Bakosi
class Mix : public tk::Toggle< MixType > {

  public:
    //! Valid expected choices to make them also available at compile-time
    //! \author J. Bakosi
    using keywords = boost::mpl::vector< kw::mix_iem
                                       , kw::mix_iecm
                                       , kw::mix_dir
                                       , kw::mix_gendir
                                       >;

    //! \brief Options constructor
    //! \details Simply initialize in-line and pass associations to base, which
    //!    will handle client interactions
    //! \author J. Bakosi
    explicit Mix() :
      Toggle< MixType >(
        //! Group, i.e., options, name
        "Material mix",
        //! Enums -> names
        { { MixType::NO_MIX, "n/a" },
          { MixType::IEM, kw::mix_iem::name() },
          { MixType::IECM, kw::mix_iecm::name() },
          { MixType::DIRICHLET, kw::mix_dir::name() },
          { MixType::GENDIR, kw::mix_gendir::name() } },
       //! keywords -> Enums
       { { "no_mix", MixType::NO_MIX },
         { kw::mix_iem::string(), MixType::IEM },
         { kw::mix_iecm::string(), MixType::IECM },
         { kw::mix_dir::string(), MixType::DIRICHLET },
         { kw::mix_gendir::string(), MixType::GENDIR } } ) {}
};

} // ctr::
} // quinoa::

#endif // QuinoaMixOptions_h

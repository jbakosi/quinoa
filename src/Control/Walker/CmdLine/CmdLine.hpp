// *****************************************************************************
/*!
  \file      src/Control/Walker/CmdLine/CmdLine.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Walker's command line
  \details   Walker's command line
*/
// *****************************************************************************
#ifndef WalkerCmdLine_h
#define WalkerCmdLine_h

#include <string>

#include <brigand/algorithms/for_each.hpp>

#include "Control.hpp"
#include "HelpFactory.hpp"
#include "Keywords.hpp"
#include "Walker/Types.hpp"

namespace walker {
//! Walker control facilitating user input to internal data transfer
namespace ctr {

//! CmdLine : Control< specialized to Walker >, see Types.h
class CmdLine : public tk::Control<
                  // tag               type
                  tag::io,             ios,
                  tag::virtualization, kw::virtualization::info::expect::type,
                  tag::verbose,        bool,
                  tag::chare,          bool,
                  tag::help,           bool,
                  tag::helpctr,        bool,
                  tag::quiescence,     bool,
                  tag::trace,          bool,
                  tag::version,        bool,
                  tag::license,        bool,
                  tag::cmdinfo,        tk::ctr::HelpFactory,
                  tag::ctrinfo,        tk::ctr::HelpFactory,
                  tag::helpkw,         tk::ctr::HelpKw,
                  tag::error,          std::vector< std::string > > {

  public:
    //! Walker command-line keywords
    using keywords = tk::cmd_keywords< kw::verbose
                                     , kw::charestate
                                     , kw::virtualization
                                     , kw::help
                                     , kw::helpctr
                                     , kw::helpkw
                                     , kw::control
                                     , kw::pdf
                                     , kw::stat
                                     , kw::quiescence
                                     , kw::trace
                                     , kw::version
                                     , kw::license
                                     >;

    //! \brief Constructor: set all defaults.
    //! \param[in] ctrinfo std::map of control file keywords and their info
    //!  \details Anything not set here is initialized by the compiler using the
    //!   default constructor for the corresponding type. The ctrinfo map
    //!   argument is optional. If not given, it is an empty std::map
    //!   constructed in-place and affects nothing. If given, it contains the
    //!   control file keywords, all of which are moved into the relevant slot
    //!   (tag::ctrinfo). This allows constructing, e.g., a CmdLine object both
    //!   with and without this information in place, which are both used at
    //!   different stages of the execution. For example, because the
    //!   command-line is parsed very early on during runtime while the input
    //!   deck is only parsed much later, the control-file keywords and their
    //!   information (owned by and generated by the input deck and its
    //!   constructor) is not yet available when the CmdLine object is
    //!   constructed. However, during command-line parsing it is still possible
    //!   to request information on a control file keyword, so it must be
    //!   available. The input deck is where all parsed information goes during
    //!   control file parsing and is stored at global scope, see, e.g.,
    //!   walker::g_inputdeck. This global-scope (still namespace-scope), input
    //!   deck object is thus created before command-line parsing. The input
    //!   deck object's constructor (working only on type information, available
    //!   at compile-time, of all the control file keywords) creates a run-time
    //!   map. This is a run-time map, but available before main() starts,
    //!   because it is const and it is initialized as a global-scope map. This
    //!   map is then passed in here as ctrinfo, and its contents inserted into
    //!   the CmdLine object, making the control-file keywords and their info
    //!   available during command-line parsing. Since the input deck stack
    //!   contains a copy of the command-line stack, the command-line stack must
    //!   be possible to be instantiated without passing the ctrinfo map,
    //!   otherwise it would be a mutual dependency.
    // cppcheck-suppress noExplicitConstructor
    CmdLine( tk::ctr::HelpFactory ctrinfo = tk::ctr::HelpFactory() ) {
      set< tag::io, tag::output >( "out" );
      set< tag::io, tag::pdf >( "pdf" );
      set< tag::io, tag::stat >( "stat.txt" );
      set< tag::virtualization >( 0.0 );
      set< tag::verbose >( false ); // Quiet output by default
      set< tag::chare >( false ); // No chare state output by default
      set< tag::trace >( true ); // Output call and stack trace by default
      set< tag::version >( false ); // Do not display version info by default
      set< tag::license >( false ); // Do not display license info by default
      // Initialize help: fill from own keywords + add map passed in
      brigand::for_each< keywords::set >( tk::ctr::Info(get<tag::cmdinfo>()) );
      get< tag::ctrinfo >() = std::move( ctrinfo );
    }

    //! Pack/Unpack
    void pup( PUP::er& p ) {
      tk::Control< tag::io,             ios,
                   tag::virtualization, kw::virtualization::info::expect::type,
                   tag::verbose,        bool,
                   tag::chare,          bool,
                   tag::help,           bool,
                   tag::helpctr,        bool,
                   tag::quiescence,     bool,
                   tag::trace,          bool,
                   tag::version,        bool,
                   tag::license,        bool,
                   tag::cmdinfo,        tk::ctr::HelpFactory,
                   tag::ctrinfo,        tk::ctr::HelpFactory,
                   tag::helpkw,         tk::ctr::HelpKw,
                   tag::error,          std::vector< std::string > >::pup(p);
    }
    friend void operator|( PUP::er& p, CmdLine& c ) { c.pup(p); }
};

} // ctr::
} // walker::

#endif // WalkerCmdLine_h
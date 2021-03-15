// *****************************************************************************
/*!
  \file      src/Control/Keywords.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Definition of all keywords
  \details   This file contains the definition of all keywords, including those
    of command-line argument parsers as well as input, i.e., control, file
    parsers. All keywords are shared among all parsers of all executables.

    All keywords are case-sensitive.

    The information contained in this file is used to build data structures for
    on-screen help on the command-line arguments and control file keywords,
    available via the --help, --helpctr, and --helpkw command-line arguments.

    A note on design: Defining structs that have static member functions
    returning a std::string is a way of storing C++-style strings at
    compile-time (which is not possible in a straightforward manner at this
    time). This could also be done with C-style const char* as well. The
    '*_info' structs store these strings, which then is used to specialize the
    _kw::keyword_ template, defined in Control/Keyword.hpp. Specializing the
    _keyword_ template also requires a specification of the precise string of
    characters that make up a keyword eventually matched by the parsers. Since
    these are all template arguments, the construction of keywords, their help,
    as well as all grammars, are entirely assembled at compile-time. Since the
    '*_info' struct member functions are static, they can be called without
    instantiating an object and thus available at compile-time.

    The definition of an '*_info' struct _requires_ at least the name, short,
    and long descriptions, defined by member functions _name()_,
    _shortDescription()_, and _longDescription()_, respectively. Everything else
    is optional. However, when adding a new keyword it is highly recommended to
    define all of the _optional_ members if they make sense for the given
    keyword. If an expect value type is also given, that can be hooked up into
    where it is used.

    The most general definition of a keyword is as follows:

    \code{.cpp}
      // Keyword info definition
      struct keyword_info {

        // Required very short name, usually a single word or (for e.g.
        // policies) a single character. This can be the keyword itself, but
        // does not have to be. This field is used as an id of the option or
        // setting.
        static std::string name() { return "Name"; }

        // Required short keyword description
        static std::string shortDescription() { return "Short description"; }

        // Required detailed keyword description. This returns a string literal,
        // since this is usually multi-line and it is less work to maintain this
        // way.
        static std::string longDescription() { return
          R"(Longer, possibly multi-line description of the keyword. Example
          usage of the keyword is welcome here. Don't worry about formatting:
          when this field is printed, extra spaces will be removed and line
          breaks will be inserted.)";
        }

        // Optional keyword alias. See also kw::Alias and
        // <tpl_install_dir>/include/pegtl/pegtl/constants.hh for examples
        // of what can be passed as template arguments (basically single
        // characters). The one below defines the character 'c' as the alias.
        // Aliases are single character long. The idea of an alias is to have a
        // long as well as a short keyword for the same functionality.
        // Currently, this is only hooked up for command-line arguments and not
        // for control-file keywords, which is intentional. Command-line
        // arguments are a lot less than control file keywords and are more
        // frequently typed by the user. Thus command-line argument aliases are
        // user-friendly. There are many control file keywords and aliases would
        // only cause confusion. Defining an alias for a command-line argument
        // enables the command-line parser to match on '--longer_keyword' as
        // well as on '-c'. Depending on whether the alias typedef is defined
        // for a keyword or not, the correct grammar is automatically generated
        // at compile-time, matching on both the longer keyword as well as on
        // the alias. Defining an alias for a control file keyword can be done
        // but has no effect in a control file parser.
        using alias = Alias< c >;

        // Optional single-character (policy) code. See also kw::Code and
        // <tpl_install_dir/include/pegtl/pegtl/constants.hh for examples
        // of what can be passed as template arguments (basically single
        // characters). The one below defines the character 'C' as the (policy)
        // code. This code is used for abbreviating policies used to configure
        // various orthogonal behaviors of classes using policy-based design.
        using code = Code< C >;

        // Optional expected data for the keyword - bundled to struct expect.
        // This struct is entirely optional within a keyword definition.
        // However, if it is defined, it must at least define the static member
        // function description() which returns the description of the type the
        // keyword expects. It may also optionally define the following fields:
        //
        //    - type - defining the expected type
        //    - lower - lower bound of the expected value
        //    - upper - upper bound of the expected value
        //    - choices - valid choices for the expected value
        //
        struct expect {

          // If this struct is defined, required expected type description, max
          // 10 characters long
          static std::string description() { return "int"; }

          // Optional expected type
          using type = std::streamsize;

          // Optional expected value lower bound
          static const type lower = 1;

          // Optional expected value upper bound
          static const type upper =
            std::numeric_limits< tk::real >::digits10 + 1;

          // Optional expected valid choices description, here giving
          // information on the expected type and the valid bounds. Note that
          // this can be any string, but if they exist, it is a good idea give
          // at least some information on the bounds, as below, since the bounds
          // are NOT displayed in the help for a keyword. This decision keeps
          // the bounds specifications generic since they can be any type. As a
          // result, the help structures, defined in HelpFactory.hpp, are simpler
          // as they do not have to be parameterized by the type of the bounds,
          // which greatly simplifies that code.
          static std::string choices() {
            return "integer [" + std::to_string(lower) + "..." +
                   std::to_string(upper) + ']';
          }

        };

      };

      // Keyword definition, passing the above info struct as the first
      // template argument, and the rest of the template arguments are the
      // characters of the keyword to be matched by the parser. Remember: all
      // keywords are case-sensitive. This one contrived example below defines
      // keyword 'kw', matching the keyword 'KeYwOrD'.
      using kw = keyword< keyword_info, K,e,Y,w,O,r,D >;
    \endcode
  \see Control/Keyword.hpp
  \see Control/HelpFactory.hpp
*/
// *****************************************************************************
#ifndef Keywords_h
#define Keywords_h

#include <limits>

#undef I
#include <pegtl/contrib/alphabet.hpp>

#include "Types.hpp"
#include "Keyword.hpp"
#include "QuinoaBuildConfig.hpp"

//! Keywords used by all input deck and command line parsers
namespace kw {

using namespace tao::pegtl::alphabet;

struct title_info {
  static std::string name() { return "title"; }
  static std::string shortDescription() { return "Set analysis title"; }
  static std::string longDescription() { return
    R"(The analysis title may be specified in the input file using the 'title'
    keyword. The 'title' keyword must be followed by a double-quoted string
    specifying the analysis title. Example: title "Example problem".
    Specifying a title is optional.)";
  }
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using title = keyword< title_info, TAOCPP_PEGTL_STRING("title") >;

struct end_info {
  static std::string name() { return "end"; }
  static std::string shortDescription() { return "End of an input block"; }
  static std::string longDescription() { return
    R"(The end of a block is given by the 'end' keyword in the input file.
    Example: "rngs ... end".)";
  }
};
using end = keyword< end_info, TAOCPP_PEGTL_STRING("end") >;

struct help_info {
  static std::string name() { return "help"; }
  static std::string shortDescription() { return
    R"(Display one-liner help on all command-line arguments)"; }
  static std::string longDescription() { return
    R"(Get a short one-liner help on all command-line arguments from an
    executable. It also triggers the help from the Charm++ runtime system and in
    addition to that of the executable, it also lists command-line arguments
    from Converse Machine, Tracing, Load Balancer, Record/Replay, and Charm++
    command-line parameters.)";
  }
  using alias = Alias< h >;
};
using help = keyword< help_info, TAOCPP_PEGTL_STRING("help") >;

struct helpctr_info {
  static std::string name() { return "helpctr"; }
  static std::string shortDescription() { return
    "Display one-liner help on all control file keywords"; }
  static std::string longDescription() { return
    R"(This keyword can be used to get a short one-liner help on all control
    file keywords from an executable.)";
  }
  using alias = Alias< C >;
};
using helpctr = keyword< helpctr_info, TAOCPP_PEGTL_STRING("helpctr") >;

struct helpkw_info {
  static std::string name() { return "helpkw"; }
  static std::string shortDescription() { return
    "Display verbose help on a single keyword"; }
  static std::string longDescription() { return
    R"(This keyword can be used to get a verbose help on a single command-line
    argument or control-file keyword (i.e., help on keyword) from an
    executable.)";
  }
  using alias = Alias< H >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using helpkw = keyword< helpkw_info, TAOCPP_PEGTL_STRING("helpkw") >;

struct seed_info {
  static std::string name() { return "seed"; }
  static std::string shortDescription() { return
    "Set random number generator seed"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a seed for a random number generator
    Example: rngmkl_mcg31 seed 1234 end)";
  }
  struct expect {
    using type = unsigned int;
    static std::string description() { return "uint"; }
  };
};
using seed = keyword< seed_info, TAOCPP_PEGTL_STRING("seed") >;

struct mkl_mcg31_info {
  static std::string name() { return "MKL MCG311"; }
  static std::string shortDescription() { return
    "Select Intel MKL MCG31 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_MCG31', a 31-bit multiplicative
    congruential random number generator, provided by Intel's Math Kernel
    Library (MKL).)";
  }
};
using mkl_mcg31 = keyword< mkl_mcg31_info, TAOCPP_PEGTL_STRING("mkl_mcg31") >;

struct mkl_r250_info {
  static std::string name() { return "MKL R250"; }
  static std::string shortDescription() { return
    "Select Intel MKL R250 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_R250', a generalized feedback
    shift register random number generator, provided by Intel's Math Kernel
    Library (MKL).)";
  }
};
using mkl_r250 = keyword< mkl_r250_info, TAOCPP_PEGTL_STRING("mkl_r250") >;

struct mkl_mrg32k3a_info {
  static std::string name() { return "MKL MRG32K3A"; }
  static std::string shortDescription() { return
   "Select Intel MKL MRG32K3A RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_MRG32K3A', a combined multiple
    recursive random number generator with two components of order 3,
    provided by Intel's Math Kernel Library (MKL).)";
  }
};
using mkl_mrg32k3a =
  keyword< mkl_mrg32k3a_info, TAOCPP_PEGTL_STRING("mkl_mrg32k3a") >;

struct mkl_mcg59_info {
  static std::string name() { return "MKL MCG59"; }
  static std::string shortDescription() { return
    "Select Intel MKL MCG59 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_MCG59', a 59-bit multiplicative
    congruential random number generator, provided by Intel's Math Kernel
    Library (MKL).)";
  }
};

using mkl_mcg59 = keyword< mkl_mcg59_info, TAOCPP_PEGTL_STRING("mkl_mcg59") >;

struct mkl_wh_info {
  static std::string name() { return "MKL WH"; }
  static std::string shortDescription() { return
    "Select Intel MKL WH RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_WH', a set of 273 Wichmann-Hill
    combined multiplicative congruential random number generators, provided
    by Intel's Math Kernel Library (MKL).)";
  }
};
using mkl_wh = keyword< mkl_wh_info, TAOCPP_PEGTL_STRING("mkl_wh") >;

struct mkl_mt19937_info {
  static std::string name() { return "MKL MT19937"; }
  static std::string shortDescription() { return
    "Select Intel MKL MT19937 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_MT19937', a Mersenne Twister
    pseudorandom number generator, provided by Intel's Math Kernel Library
    (MKL).)";
  }
};
using mkl_mt19937 =
  keyword< mkl_mt19937_info, TAOCPP_PEGTL_STRING("mkl_mt19937") >;

struct mkl_mt2203_info {
  static std::string name() { return "MKL MT2203"; }
  static std::string shortDescription() { return
    "Select Intel MKL MT2203 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_MT2203', a set of 6024 Mersenne
    Twister pseudorandom number generators, available in Intel's Math Kernel
    Library (MKL).)";
  }
};
using mkl_mt2203 = keyword< mkl_mt2203_info, TAOCPP_PEGTL_STRING("mkl_mt2203") >;

struct mkl_sfmt19937_info {
  static std::string name() { return "MKL SFMT19937"; }
  static std::string shortDescription() { return
    "Select Intel MKL SFMT19937 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_SFMT19937', a SIMD-oriented Fast
    Mersenne Twister pseudorandom number generator, provided by Intel's Math
    Kernel Library (MKL).)";
  }
};
using mkl_sfmt19937 =
  keyword< mkl_sfmt19937_info, TAOCPP_PEGTL_STRING("mkl_sfmt19937") >;

struct mkl_sobol_info {
  static std::string name() { return "MKL SOBOL"; }
  static std::string shortDescription() { return
    "Select Intel MKL SOBOL RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_SOBOL', a 32-bit Gray code-based
    random number generator, producing low-discrepancy sequences for
    dimensions 1 .le. s .le. 40 with available user-defined dimensions, provided
    by Intel's Math Kernel Library (MKL).)";
  }
};
using mkl_sobol = keyword< mkl_sobol_info, TAOCPP_PEGTL_STRING("mkl_sobol") >;

struct mkl_niederr_info {
  static std::string name() { return "MKL NIEDERR"; }
  static std::string shortDescription() { return
   "Select Intel MKL NIEDERR RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_NIEDERR', a 32-bit Gray
    code-based random number generator, producing low-discrepancy sequences
    for dimensions 1 .le. s .le. 318 with available user-defined dimensions,
    provided by Intel's Math Kernel Library (MKL).)";
  }
};
using mkl_niederr = keyword< mkl_niederr_info, TAOCPP_PEGTL_STRING("mkl_niederr") >;

struct mkl_iabstract_info {
  static std::string name() { return "MKL IABSTRACT"; }
  static std::string shortDescription() { return
    "Select Intel MKL IABSTRACT RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_IABSTRACT', an abstract random
    number generator for integer arrays, provided by Intel's Math Kernel
    Library (MKL).)";
  }
};
using mkl_iabstract =
  keyword< mkl_iabstract_info, TAOCPP_PEGTL_STRING("mkl_iabstract") >;

struct mkl_dabstract_info {
  static std::string name() { return "MKL DABSTRACT"; }
  static std::string shortDescription() { return
    "Select Intel MKL DABSTRACT RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_DABSTRACT', an abstract random
    number generator for double-precision floating-point arrays, provided by
    Intel's Math Kernel Library (MKL).)";
  }
};
using mkl_dabstract =
  keyword< mkl_dabstract_info, TAOCPP_PEGTL_STRING("mkl_dabstract") >;

struct mkl_sabstract_info {
  static std::string name() { return "MKL SABSTRACT"; }
  static std::string shortDescription() { return
    "Select Intel MKL SABSTRACT RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_SABSTRACT', an abstract random
    number generator for single-precision floating-point arrays, provided by
    Intel's Math Kernel Library (MKL).)";
  }
};
using mkl_sabstract =
  keyword< mkl_sabstract_info, TAOCPP_PEGTL_STRING("mkl_sabstract") >;

struct mkl_nondeterm_info {
  static std::string name() { return "MKL NONDETERM"; }
  static std::string shortDescription() { return
    "Select Intel MKL NONDETERM RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select 'VSL_BRNG_NONDETERM', a non-deterministic
    random number generator, provided by Intel's Math Kernel Library (MKL).)";
  }
};
using mkl_nondeterm =
  keyword< mkl_nondeterm_info, TAOCPP_PEGTL_STRING("mkl_nondeterm") >;

struct standard_info {
  static std::string name() { return "standard"; }
  static std::string shortDescription() { return
    "Select the standard algorithm for uniform RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the standard method used to generate
    uniform random numbers using the Intel Math Kernel Library (MKL) random
    number generators. Valid options are 'standard' and 'accurate'.)";
  }
};
using standard = keyword< standard_info, TAOCPP_PEGTL_STRING("standard") >;

struct accurate_info {
  static std::string name() { return "accurate"; }
  static std::string shortDescription() { return
    "Select the accurate algorithm for uniform RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the accurate method used to generate
    uniform random numbers using the Intel Math Kernel Library (MKL) random
    number generators. Valid options are 'standard' and 'accurate'.)";
  }
};
using accurate = keyword< accurate_info, TAOCPP_PEGTL_STRING("accurate") >;

struct uniform_method_info {
  static std::string name() { return "uniform method"; }
  static std::string shortDescription() { return
    "Select an Intel MKL uniform RNG method"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the method used to generate uniform
    random numbers using the Intel Math Kernel Library (MKL) random number
    generators. Valid options are 'standard' and 'accurate'.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + standard::string() + "\' | \'"
                  + accurate::string() + '\'';
    }
  };
};
using uniform_method =
  keyword< uniform_method_info, TAOCPP_PEGTL_STRING("uniform_method") >;

struct boxmuller_info {
  static std::string name() { return "Box-Muller"; }
  static std::string shortDescription() { return
   "Select the Box-Muller algorithm for sampling a Gaussian"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Box-Muller method used to generate
    Gaussian random numbers using the Intel Math Kernel Library (MKL) random
    random number generators. Valid options are 'boxmuller', 'boxmuller2',
    and 'icdf'.)";
  }
};
using boxmuller = keyword< boxmuller_info, TAOCPP_PEGTL_STRING("boxmuller") >;

struct boxmuller2_info {
  static std::string name() { return "Box-Muller 2"; }
  static std::string shortDescription() { return
   "Select the Box-Muller 2 algorithm for sampling a Gaussian"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the Box-Muller 2 method used to generate
    Gaussian random numbers using the Intel Math Kernel Library (MKL) random
    number generators.)";
  }
};
using boxmuller2 = keyword< boxmuller2_info, TAOCPP_PEGTL_STRING("boxmuller2") >;

struct icdf_info {
  static std::string name() { return "ICDF"; }
  static std::string shortDescription() { return
    R"(Use inverse cumulative distribution function for sampling a Gaussian)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the inverse cumulative distribution
    function (ICDF) method used to generate Gaussian random numbers using the
    Intel Math Kernel Library (MKL) random number generators.)";
  }
};
using icdf = keyword< icdf_info, TAOCPP_PEGTL_STRING("icdf") >;

struct gaussian_method_info {
  static std::string name() { return "Gaussian method"; }
  static std::string shortDescription() { return
    "Select an Intel MKL Gaussian RNG method"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the method used to generate Gaussian
    random numbers using the Intel Math Kernel Library (MKL) random number
    generators. Valid options are 'boxmuller', 'boxmuller2', and 'icdf'.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + boxmuller::string() + "\' | \'"
                  + boxmuller2::string() + "\' | \'"
                  + icdf::string() + '\'';
    }
  };
};
using gaussian_method =
  keyword< gaussian_method_info, TAOCPP_PEGTL_STRING("gaussian_method") >;

struct gaussianmv_method_info {
  static std::string name() { return "multi-variate Gaussian method"; }
  static std::string shortDescription() { return
    "Select an Intel MKL multi-variate Gaussian RNG method"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the method used to generate multi-variate
    Gaussian random numbers using the Intel Math Kernel Library (MKL) random
    number generators. Valid options are 'boxmuller', 'boxmuller2', and
    'icdf'.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + boxmuller::string() + "\' | \'"
                  + boxmuller2::string() + "\' | \'"
                  + icdf::string() + '\'';
    }
  };
};
using gaussianmv_method =
  keyword< gaussianmv_method_info, TAOCPP_PEGTL_STRING("gaussianmv_method") >;

struct cja_info {
  static std::string name() { return "CJA"; }
  static std::string shortDescription() { return
   "Select the Cheng, Johnk, Atkinson algorithm for sampling a beta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Cheng-Johnk-Atkinson method used to
    generate beta random numbers using the Intel Math Kernel Library (MKL)
    random number generators.)";
  }
};
using cja = keyword< cja_info, TAOCPP_PEGTL_STRING("cja") >;

struct cja_accurate_info {
  static std::string name() { return "CJA accurate"; }
  static std::string shortDescription() { return
   "Select the accurate Cheng, Johnk, Atkinson algorithm for sampling a beta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the accurate version of the
    Cheng-Johnk-Atkinson method used to generate beta random numbers using the
    Intel Math Kernel Library (MKL) random number generators.)";
  }
};
using cja_accurate =
  keyword< cja_accurate_info, TAOCPP_PEGTL_STRING("cja_accurate") >;

struct beta_method_info {
  static std::string name() { return "Beta method"; }
  static std::string shortDescription() { return
    "Select an Intel MKL beta RNG method"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the method used to generate beta
    random numbers using the Intel Math Kernel Library (MKL) random number
    generators. Valid options are 'cja' and 'cja_accurate'.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + cja::string() + "\' | \'"
                  + cja_accurate::string() + '\'';
    }
  };
};
using beta_method =
  keyword< beta_method_info, TAOCPP_PEGTL_STRING("beta_method") >;

struct gnorm_info {
  static std::string name() { return "GNORM"; }
  static std::string shortDescription() { return
   "Select the GNORM (see MKL doc) algorithm for sampling a gamma"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GNORM method used to
    generate gamma random numbers using the Intel Math Kernel Library (MKL)
    random number generators.)";
  }
};
using gnorm = keyword< gnorm_info, TAOCPP_PEGTL_STRING("gnorm") >;

struct gnorm_accurate_info {
  static std::string name() { return "GNORM accurate"; }
  static std::string shortDescription() { return
   "Select the accurate GNORM (see MKL doc) algorithm for sampling a gamma"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the accurate version of the
    GNORM method used to generate gamma random numbers using the
    Intel Math Kernel Library (MKL) random number generator.)";
  }
};
using gnorm_accurate =
  keyword< gnorm_accurate_info, TAOCPP_PEGTL_STRING("gnorm_accurate") >;

struct gamma_method_info {
  static std::string name() { return "Gamma method"; }
  static std::string shortDescription() { return
    "Select an Intel MKL gamma RNG method"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the method used to generate gamma
    random numbers using the Intel Math Kernel Library (MKL) random number
    generators. Valid options are 'gnorm' and 'gnorm_accurate'.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + gnorm::string() + "\' | \'"
                  + gnorm_accurate::string() + '\'';
    }
  };
};
using gamma_method =
  keyword< gamma_method_info, TAOCPP_PEGTL_STRING("gamma_method") >;

struct rngsse_gm19_info {
  static std::string name() { return "RNGSSE GM19"; }
  static std::string shortDescription() { return
    "Select RNGSSE GM19 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GM19 random number generator, using a
    method based on parallel evolution of an ensemble of transformations of
    a two-dimensional torus, provided by the RNGSSE2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gm19 =
  keyword< rngsse_gm19_info, TAOCPP_PEGTL_STRING("rngsse_gm19") >;

struct rngsse_gm29_info {
  static std::string name() { return "RNGSSE GM29"; }
  static std::string shortDescription() { return
    "Select RNGSSE GM29 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GM29 generator, using a method based
    on parallel evolution of an ensemble of transformations of a
    two-dimensional torus, provided by the RNGSSE2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gm29 =
  keyword< rngsse_gm29_info, TAOCPP_PEGTL_STRING("rngsse_gm29") >;

struct rngsse_gm31_info {
  static std::string name() { return "RNGSSE GM31"; }
  static std::string shortDescription() { return
    "Select RNGSSE GM31 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GM31 generator, using a method based
    on parallel evolution of an ensemble of transformations of a
    two-dimensional torus, provided by the RNGSSE2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gm31 =
  keyword< rngsse_gm31_info, TAOCPP_PEGTL_STRING("rngsse_gm31") >;

struct rngsse_gm55_info {
  static std::string name() { return "RNGSSE GM55"; }
  static std::string shortDescription() { return
    "Select RNGSSE GM55 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GM55 generator, using a method based
    on parallel evolution of an ensemble of transformations of a
    two-dimensional torus, provided by the RNGSSE2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gm55 =
  keyword< rngsse_gm55_info, TAOCPP_PEGTL_STRING("rngsse_gm55") >;

struct rngsse_gm61_info {
  static std::string name() { return "RNGSSE GM61"; }
  static std::string shortDescription() { return
    "Select RNGSSE GM61 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GM61 generator, using a method based
    on parallel evolution of an ensemble of transformations of a
    two-dimensional torus, provided by the RNGSSE2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gm61 =
  keyword< rngsse_gm61_info, TAOCPP_PEGTL_STRING("rngsse_gm61") >;

struct rngsse_gq581_info {
  static std::string name() { return "RNGSSE GQ58.1"; }
  static std::string shortDescription() { return
    "Select RNGSSE GQ58.1 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GQ58.1 generator, using a method based
    on parallel evolution of an ensemble of transformations of a
    two-dimensional torus, provided by the RNGSSE2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gq581 =
  keyword< rngsse_gq581_info, TAOCPP_PEGTL_STRING("rngsse_gq58.1") >;

struct rngsse_gq583_info {
  static std::string name() { return "RNGSSE GQ58.3"; }
  static std::string shortDescription() { return
    "Select RNGSSE GQ58.3 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GQ58.3 generator, using a method based
    on parallel evolution of an ensemble of transformations of a
    two-dimensional torus, provided by the RNGSS2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gq583 =
  keyword< rngsse_gq583_info, TAOCPP_PEGTL_STRING("rngsse_gq58.3") >;

struct rngsse_gq584_info {
  static std::string name() { return "RNGSSE GQ58.4"; }
  static std::string shortDescription() { return
    "Select RNGSSE GQ58.4 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the GQ58.4 generator, using a method based
    on parallel evolution of an ensemble of transformations of a
    two-dimensional torus, provided by the RNGSSE2 random number generator
    library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_gq584 =
  keyword< rngsse_gq584_info, TAOCPP_PEGTL_STRING("rngsse_gq58.4") >;

struct rngsse_mt19937_info {
  static std::string name() { return "RNGSSE MT19937"; }
  static std::string shortDescription() { return
    "Select RNGSSE MT19937 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the MT19937 generator, a Mersenne Twister
    generator, provided by the RNGSSE2 random number generator library. For
    more info on RNGSSE see https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_mt19937 =
  keyword< rngsse_mt19937_info, TAOCPP_PEGTL_STRING("rngsse_mt19937") >;

struct rngsse_lfsr113_info {
  static std::string name() { return "RNGSSE LSFR113"; }
  static std::string shortDescription() { return
    "Select RNGSSE LFSR113 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the LFSR113 generator, provided by the
    RNGSSE2 random number generator library. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_lfsr113 =
  keyword< rngsse_lfsr113_info, TAOCPP_PEGTL_STRING("rngsse_lfsr113") >;

struct rngsse_mrg32k3a_info {
  static std::string name() { return "RNGSSE MRG32K3A"; }
  static std::string shortDescription() { return
    "Select RNGSSE MRG32K3A RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the MRG32K3A generator, a combined
    multiple recursive random number generator with two components of order
    3, provided by the RNGSS2 random number generator library. For more info
    on RNGSSE see https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using rngsse_mrg32k3a =
  keyword< rngsse_mrg32k3a_info, TAOCPP_PEGTL_STRING("rngsse_mrg32k3a") >;

struct seq_short_info {
  static std::string name() { return "short"; }
  static std::string shortDescription() { return
    "Select the short sequence length for an RNGSSE2 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the short sequence length used by the
    RNGSSE2 random number generator library. Valid options are 'short',
    'medium', and 'long'. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using seq_short = keyword< seq_short_info, TAOCPP_PEGTL_STRING("short") >;

struct seq_med_info {
  static std::string name() { return "medium"; }
  static std::string shortDescription() { return
    "Select the medium sequence length for an RNGSSE2 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the medium sequence length used by the
    RNGSSE2 random number generator library. Valid options are 'short',
    'medium', and 'long'. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using seq_med = keyword< seq_med_info, TAOCPP_PEGTL_STRING("medium") >;

struct seq_long_info {
  static std::string name() { return "long"; }
  static std::string shortDescription() { return
    "Select the long sequence length for an RNGSSE2 RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the medium sequence length used by the
    RNGSSE2 random number generator library. Valid options are 'short',
    'medium', and 'long'. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
};
using seq_long = keyword< seq_long_info, TAOCPP_PEGTL_STRING("long") >;

struct seqlen_info {
  static std::string name() { return "RNGSSE2 sequence length"; }
  static std::string shortDescription() { return
    "Specify the RNGSSE RNG sequence length"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a random number generator sequence length,
    used by the RNGSSE2 random number generator library. Valid options are
    'short', 'medium', and 'long'. For more info on RNGSSE see
    https://doi.org/10.1016/j.cpc.2011.03.022.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + seq_short::string() + "\' | \'"
                  + seq_med::string() + "\' | \'"
                  + seq_long::string() + '\'';
    }
  };
};
using seqlen = keyword< seqlen_info, TAOCPP_PEGTL_STRING("seqlen") >;

struct r123_threefry_info {
  static std::string name() { return "Random123 ThreeFry"; }
  static std::string shortDescription() { return
    "Select Random123 ThreeFry RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the ThreeFry generator, related to the
    Threefish block cipher from Skein Hash Function, provided by the Random123
    random number generator library. For more info on Random123 see
    http://dl.acm.org/citation.cfm?doid=2063405.)";
  }
};
using r123_threefry =
  keyword< r123_threefry_info, TAOCPP_PEGTL_STRING("r123_threefry") >;

struct r123_philox_info {
  static std::string name() { return "Random123 Philox"; }
  static std::string shortDescription() { return
    "Select Random123 Philox RNG"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Philox generator, based on Feistel
    network and integer multiplication, provided by the Random123
    random number generator library. For more info on Random123 see
    http://dl.acm.org/citation.cfm?doid=2063405.)";
  }
};
using r123_philox =
  keyword< r123_philox_info, TAOCPP_PEGTL_STRING("r123_philox") >;

struct pdfs_info {
  static std::string name() { return "PDFs block"; }
  static std::string shortDescription() { return
    "Start of probability density function (PDF) input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to start a block in the input file containing the
    descriptions and settings of requested output for probability density
    functions (PDFs). Example: "pdfs mypdf( y1 : 1.0e-2 ) end", which
    requests a single-variate PDF to be output to file, whose sample space
    variable is y1, using automatic determination of the bounds of the sample
    space, using 1.0e-2 as the sample space bin size, and call the PDF
    "mypdf". For more info on the structure of the pdfs ... end block, see
    doc/pages/statistics_output.dox.)";
  }
};
using pdfs = keyword< pdfs_info, TAOCPP_PEGTL_STRING("pdfs") >;

struct txt_info {
  static std::string name() { return "txt"; }
  static std::string shortDescription() { return
    "Select ASCII output for outputing PDFs"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the text
    output file type of a requested probability density function (PDF) within
    a pdfs ... end block. Example: "filetype txt", which selects text-file
    output. Valid options are 'txt', 'gmshtxt', 'gmshbin', and 'exodusii'.
    For more info on the structure of the pdfs ... end block, see
    doc/pages/statistics_output.dox.)"; }
};
using txt = keyword< txt_info, TAOCPP_PEGTL_STRING("txt") >;

struct gmshtxt_info {
  static std::string name() { return "gmshtxt"; }
  static std::string shortDescription() { return
    "Select Gmsh ASCII output for outputing PDFs"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the ASCII
    (text) output file type readable by Gmsh of a requested probability
    density function (PDF) within a pdfs ... end block. Example: "filetype
    gmshtxt", which selects Gmsh ASCII file output. Valid options are 'txt',
    'gmshtxt', 'gmshbin', and 'exodusii'. For more info on the structure of
    the pdfs ... end block, see doc/pages/statistics_output.dox. For more
    info on Gmsh, see http://www.geuz.org/gmsh.)"; }
};
using gmshtxt = keyword< gmshtxt_info, TAOCPP_PEGTL_STRING("gmshtxt") >;

struct gmshbin_info {
  static std::string name() { return "gmshbin"; }
  static std::string shortDescription() { return
    "Select Gmsh binary output for outputing PDFs"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the
    binary output file type readable by Gmsh of a requested probability
    density function (PDF) within a pdfs ... end block. Example: "filetype
    gmshbin", which selects Gmsh binary file output. Valid options are 'txt',
    'gmshtxt', 'gmshbin', and 'exodusii'. For more info on the structure of
    the pdfs ... end block, see doc/pages/statistics_output.dox. For more
    info on Gmsh, see http://www.geuz.org/gmsh.)"; }
};
using gmshbin = keyword< gmshbin_info, TAOCPP_PEGTL_STRING("gmshbin") >;

struct exodusii_info {
  static std::string name() { return "exo"; }
  static std::string shortDescription() { return
    "Select ExodusII output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the
    ExodusII output file type readable by, e.g., ParaView of either a requested
    probability density function (PDF) within a pdfs ... end block or for
    mesh-based field output in a field_output ... end block. Example:
    "filetype exodusii", which selects ExodusII file output. For more info on
    ExodusII, see http://sourceforge.net/projects/exodusii.)";
  }
};
using exodusii = keyword< exodusii_info, TAOCPP_PEGTL_STRING("exodusii") >;

struct root_info {
  static std::string name() { return "root"; }
  static std::string shortDescription() { return
    "Select Root output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Root output file type readable by the
    Root framework from CERN for mesh-based field output in a field_output ...
    end block. Example: "filetype root", which selects the root file output
    format. For more info on Root, see https://root.cern.ch.)";
  }
};
using root = keyword< root_info, TAOCPP_PEGTL_STRING("root") >;

struct filetype_info {
  static std::string name() { return "filetype"; }
  static std::string shortDescription() { return
    "Select output file type"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the output file type of a requested
    probability density function (PDF) within a pdfs ... end block or for
    mesh-based field output in a field_output ... end block. Example:
    "filetype exodusii", which selects ExodusII output. Valid options depend on
    which block the keyword is used: in a pdfs ... end the valid choices are
    'txt', 'gmshtxt', 'gmshbin', and 'exodusii', in a field_output ... end
    block the valid choices are 'exodusii' and 'root'.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + txt::string() + "\' | \'"
                  + gmshtxt::string() + "\' | \'"
                  + gmshbin::string() + "\' | \'"
                  + root::string() + "\' | \'"
                  + exodusii::string() + '\'';
    }
  };

};
using filetype = keyword< filetype_info, TAOCPP_PEGTL_STRING("filetype") >;

struct overwrite_info {
  static std::string name() { return "overwrite"; }
  static std::string shortDescription() { return
    "Select PDF output policy overwrite"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the
    the 'overwrite' output file policy for requested probability density
    functions (PDFs) within a pdfs ... end block. Example: "policy
    overwrite", which selects the overwrite output file policy. The
    overwrite policy overwrites the same output file containing a single time
    step. Valid PDF policy options are 'overwrite', 'multiple', and
    'evolution'. For more info on the structure of the pdfs ... end block,
    see doc/pages/statistics_output.dox.)"; }
};
using overwrite = keyword< overwrite_info, TAOCPP_PEGTL_STRING("overwrite") >;

struct multiple_info {
  static std::string name() { return "multiple"; }
  static std::string shortDescription() { return
    "Select PDF output policy multiple"; }
  static std::string longDescription() { return
    R"(This keyword is used to select
    the 'multiple' output file policy for requested probability density
    functions (PDFs) within a pdfs ... end block. Example: "policy
    multiple", which selects the multiple output file policy. The
    multiple policy output creates a new file for each time step. Valid PDF
    policy options are 'overwrite', 'multiple', and 'evolution'. For more
    info on the structure of the pdfs ... end block, see
    doc/pages/statistics_output.dox.)"; }
};
using multiple = keyword< multiple_info, TAOCPP_PEGTL_STRING("multiple") >;

struct evolution_info {
  static std::string name() { return "evolution"; }
  static std::string shortDescription() { return
    "Select PDF output policy evolution"; }
  static std::string longDescription() { return
    R"(This keyword is used to select
    the 'evolution' output file policy for requested probability density
    functions (PDFs) within a pdfs ... end block. Example: "policy
    evolution", which selects the evolution output file policy. The
    evolution policy output appends new time step to the same output file for
    each time instant, yielding a time evolution of data in a single file.
    Valid PDF policy options are 'overwrite', 'multiple', and 'evolution'. For
    more info on the structure of the pdfs ... end block, see
    doc/pages/statistics_output.dox.)";
  }
};
using evolution = keyword< evolution_info, TAOCPP_PEGTL_STRING("evolution") >;

struct policy_info {
  static std::string name() { return "policy"; }
  static std::string shortDescription() { return
    "Select PDF output file policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select
    the output file policy for requested probability density functions
    (PDFs) within a pdfs ... end block. Example: "policy overwrite", which
    selects the overwrite output file policy. Valid options are 'overwrite',
    'multiple', and 'evolution'. For more info on the structure of the
    pdfs ... end block, see doc/pages/statistics_output.dox.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + overwrite::string() + "\' | \'"
                  + multiple::string() + "\' | \'"
                  + evolution::string() + '\'';
    }
  };
};
using pdf_policy = keyword< policy_info, TAOCPP_PEGTL_STRING("policy") >;

struct txt_float_default_info {
  static std::string name() { return "default"; }
  static std::string shortDescription() { return
   "Select the default ASCII floating-point output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the
    'default' floating-point output format for ASCII floating-point real
    number output. Example: "format default", which selects the default
    floating-point output. Valid options are 'default', 'fixed', and
    'scientific'. For more info on these various formats, see
    http://en.cppreference.com/w/cpp/io/manip/fixed.)";
  }
};
using txt_float_default =
  keyword< txt_float_default_info, TAOCPP_PEGTL_STRING("default") >;

struct txt_float_scientific_info {
  static std::string name() { return "scientific"; }
  static std::string shortDescription() { return
   "Select the scientific ASCII floating-point output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the
    'scientific' floating-point output format for ASCII floating-point real
    number output. Example: "format scientific", which selects the
    scientific floating-point output. Valid options are 'default', 'fixed',
    and 'scientific'. For more info on these various formats, see
    http://en.cppreference.com/w/cpp/io/manip/fixed.)";
  }
};
using txt_float_scientific =
  keyword< txt_float_scientific_info, TAOCPP_PEGTL_STRING("scientific") >;

struct txt_float_fixed_info {
  static std::string name() { return "fixed"; }
  static std::string shortDescription() { return
   "Select the fixed ASCII floating-point output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the
    'fixed' floating-point output format for ASCII floating-point real
    number output. Example: \"format fixed\", which selects the fixed
    floating-point output. Valid options are 'default', 'fixed', and
    'scientific'. For more info on these various formats, see
    http://en.cppreference.com/w/cpp/io/manip/fixed.)";
  }
};
using txt_float_fixed = keyword< txt_float_fixed_info, TAOCPP_PEGTL_STRING("fixed") >;

struct txt_float_format_info {
  static std::string name() { return "float format"; }
  static std::string shortDescription() { return
    "Specify the ASCII floating-point output format"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the
    floating-point output format for ASCII floating-point number output.
    Example: "format scientific", which selects the scientific
    floating-point output. Valid options are 'default', 'fixed', and
    'scientific'. For more info on these various formats, see
    http://en.cppreference.com/w/cpp/io/manip/fixed.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + txt_float_default::string() + "\' | \'"
                  + txt_float_scientific::string() + "\' | \'"
                  + txt_float_fixed::string() + '\'';
    }
  };
};
using txt_float_format =
  keyword< txt_float_format_info, TAOCPP_PEGTL_STRING("format") >;

struct precision_info {
  static std::string name() { return "precision"; }
  static std::string shortDescription() { return
    R"(Precision in digits for ASCII floating-point output)"; }
  static std::string longDescription() { return
    R"(This keyword is used to select
    the precision in digits for ASCII floating-point real number output.
    Example: "precision 10", which selects ten digits for floating-point
    output, e.g., 3.141592654. The number of digits must be larger than zero
    and lower than the maximum representable digits for the given floating-point
    type, defined by std::numeric_limits< FLOAT_TYPE >::digits10+2.
    For more info on setting the precision in C++, see
    http://en.cppreference.com/w/cpp/io/manip/setprecision, and
    http://en.cppreference.com/w/cpp/types/numeric_limits/digits10)";
  }
  struct expect {
    using type = std::streamsize;
    static constexpr type lower = 1;
    static constexpr type upper = std::numeric_limits< tk::real >::digits10 + 1;
    static std::string description() { return "int"; }
    static std::string choices() {
      return "integer between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using precision = keyword< precision_info, TAOCPP_PEGTL_STRING("precision") >;

struct elem_info {
  static std::string name() { return "elem"; }
  static std::string shortDescription() { return
    "Specify elem-centering for output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select elem-centering for variable output. In
    walker for example, this is used to configure probability values on the
    sample space grid for file output of probability density functions (PDFs).
    Example: "centering elem", which selects element-centered values. Valid
    options are 'elem' and 'node', denoting cell-centered and point-centered
    output, respectively. In inciter this keyword is used in output variable
    specification blocks, prefixing variable names by either 'node' or 'elem',
    to specify their centering for output to file.)"; }
};
using elem = keyword< elem_info, TAOCPP_PEGTL_STRING("elem") >;

struct node_info {
  static std::string name() { return "node"; }
  static std::string shortDescription() { return
    "Specify node-centering for output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select node-centering for variable output. In
    walker for example, this is used to configure probability values on the
    sample space grid for file output of probability density functions (PDFs).
    Example: "centering elem", which selects element-centered values. Valid
    options are 'elem' and 'node', denoting cell-centered and point-centered
    output, respectively. In inciter this keyword is used in output variable
    specification blocks, prefixing variable names by either 'node' or 'elem',
    to specify their centering for output to file.)"; }
};
using node = keyword< node_info, TAOCPP_PEGTL_STRING("node") >;

struct centering_info {
  static std::string name() { return "centering"; }
  static std::string shortDescription() { return
    "Specify data-centering for PDF output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the data
    centering of the probability value output on the sample space grid for
    file output of probability density functions (PDFs). Example: "centering
    elem", which selects element-centered values. Valid options are 'elem'
    and 'node', denoting cell-centered and point-centered output,
    respectively.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + elem::string() + "\' | \'"
                  + node::string() + '\'';
    }
  };
};
using pdf_centering = keyword< centering_info, TAOCPP_PEGTL_STRING("centering") >;

struct raw_info {
  using code = Code< R >;
  static std::string name() { return "raw"; }
  static std::string shortDescription() { return
    "Select the raw initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the raw
    initialization policy. The initialization policy is used to specify how
    the initial conditions are set at t = 0 before time-integration.
    Example: "init raw", which selects raw initialization policy, which
    leaves the memory uninitialized. Note that this option may behave
    differently depending on the particular equation or physical model. See the
    the init policies in DiffEq/InitPolicy.hpp for valid options.)"; }
};
using raw = keyword< raw_info, TAOCPP_PEGTL_STRING("raw") >;

struct zero_info {
  using code = Code< Z >;
  static std::string name() { return "zero"; }
  static std::string shortDescription() { return
    "Select the zero initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the zero
    initialization policy. The initialization policy is used to specify how
    the initial conditions are set at t = 0 before time-integration.
    Example: "init zero", which selects zero initialization policy, which
    puts zeros in memory. Note that this option may behave differently
    depending on the particular equation or physical model. See the init
    policies in DiffEq/InitPolicy.hpp for valid options.)"; }
};
using zero = keyword< zero_info, TAOCPP_PEGTL_STRING("zero") >;

struct jointdelta_info {
  using code = Code< D >;
  static std::string name() { return "delta"; }
  static std::string shortDescription() { return
    "Select the joint delta initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the joint delta initialization policy.
    The initialization policy is used to specify how the initial conditions are
    set at t = 0 before time-integration. Example: "init zero", which selects
    zero initialization policy, which puts zeros in memory. Note that this
    option may behave differently depending on the particular equation or
    physical model. For an example, see tk::InitPolicies in
    DiffEq/InitPolicy.hpp for valid options.) The joint delta initialization
    policy can be used to prescribe delta-spikes on the sample space with given
    heights, i.e., probabilities. Example: "init jointdelta" - select delta
    init-policy, "delta spike 0.1 0.3 0.8 0.7 end end" - prescribe two
    delta-spikes at sample space positions 0.1 and 0.8 with spike heights 0.3
    and 0.7, respectively. Note that the sum of the heights must add up to
    unity. See also the help on keyword spike.)"; }
};
using jointdelta = keyword< jointdelta_info, TAOCPP_PEGTL_STRING("jointdelta") >;

struct jointgaussian_info {
  using code = Code< G >;
  static std::string name() { return "Gaussian"; }
  static std::string shortDescription() { return
    "Select the joint Gaussian initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the joint Gaussian initialization policy.
    The initialization policy is used to specify how the initial conditions are
    set at t = 0 before time-integration. Example: "init zero", which selects
    zero initialization policy, which puts zeros in memory. Note that this
    option may behave differently depending on the particular equation or
    physical model. For an example, see tk::InitPolicies in
    DiffEq/InitPolicy.hpp for valid options.) The joint Gaussian initialization
    policy can be used to prescribe a joint Gaussian (joint Gaussian) on the
    sample space with given variances. Example: "init jointgaussian" - select
    (joint) Gaussian init-policy, "gaussian 0.1 0.3 0.8 0.7 end" - prescribe two
    Gaussians with mean 0.1 and variance 0.3, and with mean 0.8 and 0.7,
    respectively. Note that the means can be any real number while the
    variances must be positive. No correlations between the Gaussians (as the
    initial conditions) are supported.)"; }
};
using jointgaussian =
  keyword< jointgaussian_info, TAOCPP_PEGTL_STRING("jointgaussian") >;

struct jointcorrgaussian_info {
  using code = Code< C >;
  static std::string name() { return "correlated Gaussian"; }
  static std::string shortDescription() { return
    "Select the joint correlated Gaussian initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the joint correlated Gaussian
    initialization policy. The initialization policy is used to specify how the
    initial conditions are
    set at t = 0 before time-integration. Example: "init zero", which selects
    zero initialization policy, which puts zeros in memory. Note that this
    option may behave differently depending on the particular equation or
    physical model. For an example, see tk::InitPolicies in
    DiffEq/InitPolicy.hpp for valid options.) The joint correlated Gaussian
    initialization policy can be used to prescribe a joint correlated Gaussian
    on the sample space with a given covariance matrix. Example:
     "init jointcorrgaussian
      icjointgaussian
        mean 0.0 0.5 1.0 end
        cov
          4.0  2.5   1.1
              32.0   5.6
                    23.0
        end
      end")"; }
};
using jointcorrgaussian =
  keyword< jointcorrgaussian_info, TAOCPP_PEGTL_STRING("jointcorrgaussian") >;

struct jointbeta_info {
  using code = Code< B >;
  static std::string name() { return "beta"; }
  static std::string shortDescription() { return
    "Select the joint beta initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the joint beta initialization policy.
    The initialization policy is used to specify how the initial conditions are
    set at t = 0 before time-integration. Example: "init zero", which selects
    zero initialization policy, which puts zeros in memory. Note that this
    option may behave differently depending on the particular equation or
    physical model. For an example, see tk::InitPolicies in
    DiffEq/InitPolicy.hpp for valid options.) The joint beta initialization
    policy can be used to prescribe a multi-dimensional sample space where the
    samples are generated from a joint beta distribution with independent
    marginal univariate beta distributions.)";
  }
};
using jointbeta = keyword< jointbeta_info, TAOCPP_PEGTL_STRING("jointbeta") >;

struct jointgamma_info {
  using code = Code< A >;
  static std::string name() { return "gamma"; }
  static std::string shortDescription() { return
    "Select the joint gamma initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the joint gamma initialization policy.
    The initialization policy is used to specify how the initial conditions are
    set at t = 0 before time-integration. Example: "init zero", which selects
    zero initialization policy, which puts zeros in memory. Note that this
    option may behave differently depending on the particular equation or
    physical model. For an example, see tk::InitPolicies in
    DiffEq/InitPolicy.hpp for valid options.) The joint gamma initialization
    policy can be used to prescribe a joint gamma distribution on the
    sample space with given shape and scale parameters. Example: "init
    jointgamma" - select the (joint) gamma init-policy, "gammapdf 0.1 0.3
    end" - prescribe a gamma distribution with shape 0.1 and scale 0.3
    parameters, respectively. Note that both shape and scale
    must be positive. Multiple independent gamma PDFs can be specified and the
    they will be used for the different scalar components configured for the
    equation. No correlations between the gamma distributions (as the
    initial conditions) are supported.)"; }
};
using jointgamma =
  keyword< jointgamma_info, TAOCPP_PEGTL_STRING("jointgamma") >;

struct jointdirichlet_info {
  using code = Code< I >;
  static std::string name() { return "Dirichlet"; }
  static std::string shortDescription() { return
    "Select the Dirichlet initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Dirichlet initialization policy.
    The initialization policy is used to specify how the initial conditions are
    set at t = 0 before time-integration. Example: "init zero", which selects
    zero initialization policy, which puts zeros in memory. Note that this
    option may behave differently depending on the particular equation or
    physical model. For an example, see tk::InitPolicies in
    DiffEq/InitPolicy.hpp for valid options.) The Dirichlet initialization
    policy can be used to prescribe a Dirichlet distribution on the
    sample space with given shape parameters. Example: "init
    jointdirichlet" - select the Dirichlet init-policy, "dirichletpdf 0.1 0.3
    0.2 end" - prescribe a Dirichlet distribution with shape parameters 0.1,
    0.3, and 0.2. All shape parameters must be positive.)"; }
};
using jointdirichlet =
  keyword< jointdirichlet_info, TAOCPP_PEGTL_STRING("jointdirichlet") >;

struct init_info {
  using code = Code< i >;
  static std::string name() { return "initialization policy"; }
  static std::string shortDescription() { return
    "Select initialization policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select an
    initialization policy. This is used to specify how the initial conditions
    are set at t = 0 before time-integration. Example: "init raw", which
    selects raw initialization policy, which leaves the memory uninitialized.
    Note that this option may behave differently depending on the particular
    equation or physical model. See the init policies in
    DiffEq/InitPolicy.hpp for valid options.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + raw::string() + "\' | \'"
                  + zero::string() + "\' | \'"
                  + jointdelta::string() + "\' | \'"
                  + jointbeta::string() + "\' | \'"
                  + jointgaussian::string() + "\' | \'"
                  + jointcorrgaussian::string() + "\' | \'"
                  + jointgamma::string() + '\'';
    }
  };
};
using init = keyword< init_info, TAOCPP_PEGTL_STRING("init") >;

struct constcoeff_info {
  using code = Code< C >;
  static std::string name() { return "constant coefficients"; }
  static std::string shortDescription() { return
    "Select constant coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the 'constant coefficients' coefficients
    policy. A coefficients policy is used to specify how the coefficients are
    set at each time step during time-integration. Example: "coeff
    const_coeff", which selects 'constant coefficients' coefficients policy,
    which sets constant coefficients before t = 0 and leaves the coefficients
    unchanged during time integration. Note that this option may behave
    differently depending on the particular equation or physical model.)"; }
};
using constcoeff =
  keyword< constcoeff_info, TAOCPP_PEGTL_STRING("const_coeff") >;

struct decay_info {
  using code = Code< D >;
  static std::string name() { return "decay"; }
  static std::string shortDescription() { return
    "Select decay coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the decay coefficients policy. This policy
    (or model) is used to constrain a beta stochastic differential equation so
    that its variance, <y^2>, always decays. A coefficients policy, in general,
    is used to specify how the coefficients are set at each time step during
    time-integration. Example: "coeff const", which selects constant
    coefficients policy, which sets constant coefficients before t = 0 and
    leaves the coefficients unchanged during time integration. Note that this
    option may behave differently depending on the particular equation or
    physical model.)"; }
};
using decay = keyword< decay_info, TAOCPP_PEGTL_STRING("decay") >;

struct homogeneous_info {
  using code = Code< H >;
  static std::string name() { return "homogeneous"; }
  static std::string shortDescription() { return
    "Select homogeneous coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the homogeneous coefficients policy.
    This policy (or model) is used to constrain a Dirichlet stochastic
    differential equation so that its mean density stays constant.
    A coefficients policy, in general, is used to specify how the
    coefficients are set at each time step during time-integration. Example:
    "coeff const", which selects constant coefficients policy, which sets
    constant coefficients before t = 0 and leaves the coefficients unchanged
    during time integration. Note that this option may behave differently
    depending on the particular equation or physical model.)"; }
};
using homogeneous =
  keyword< homogeneous_info, TAOCPP_PEGTL_STRING("homogeneous") >;

struct homdecay_info {
  using code = Code< Y >;
  static std::string name() { return "homogeneous decay"; }
  static std::string shortDescription() { return
    "Select homogeneous decay coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the homogeneous decay coefficients policy.
    This policy (or model) is used to constrain a beta stochastic differential
    equation so that its variance, <y^2>, always decays and its mean, <R> =
    rho2/(1+r<RY>/<R>), where Y = <Y> + y, does not change in time. Note that
    R = rho2/(1+rY). This policy is similar to 'montecarlo_homdecay', but
    computes the SDE coefficient S in a different but statistically equivalent
    way. While 'homdecay' only requires the estimation of statistics, <R>,
    <r^2>, and <r^3>, 'montecarlo_homdecay' requires <R^2>, <YR^2>, and
    <Y(1-Y)R^3>. A coefficients policy, in general, is used to specify how the
    coefficients are set at each time step during time-integration. Example:
    "coeff const", which selects constant coefficients policy, which sets
    constant coefficients before t = 0 and leaves the coefficients unchanged
    during time integration. Note that this option may behave differently
    depending on the particular equation or physical model.)"; }
};
using homdecay = keyword< homdecay_info, TAOCPP_PEGTL_STRING("homdecay") >;

struct montecarlo_homdecay_info {
  using code = Code< M >;
  static std::string name() { return "Monte Carlo homogeneous decay"; }
  static std::string shortDescription() { return
    "Select Monte Carlo homogeneous decay coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Monte Carlo homogeneous decay
    coefficients policy. This policy (or model) is used to constrain a beta
    stochastic differential equation (SDE) so that its variance, <y^2>, always
    decays and its mean, <R> = rho2/(1+r<RY>/<R>), where Y = <Y> + y, does not
    change in time. Note that R = rho2/(1+rY). This policy is similar to
    'homdecay', but computes the the SDE coefficient S in a different but
    statistically equivalent way. While 'homdecay' only requires the estimation
    of statistics, <R>, <r^2>, and <r^3>, 'montecarlo_homdecay' requires <R^2>,
    <YR^2>, and <Y(1-Y)R^3>. A coefficients policy, in general, is used to
    specify how the coefficients are set at each time step during
    time-integration. Example: "coeff const", which selects constant
    coefficients policy, which sets constant coefficients before t = 0 and
    leaves the coefficients unchanged during time integration. Note that this
    option may behave differently depending on the particular equation or
    physical model.)"; }
};
using montecarlo_homdecay =
  keyword< montecarlo_homdecay_info, TAOCPP_PEGTL_STRING("montecarlo_homdecay") >;

struct hydrotimescale_info {
  using code = Code< T >;
  static std::string name() { return "hydro-timescale"; }
  static std::string shortDescription() { return
    "Select hydro-timescale coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the hydrodynamics-timescale
    coefficients policy. This policy (or model) is used to constrain a
    beta stochastic differential equation (SDE) so that its variance, <y^2>,
    always decays and its mean, <R> = rho2/(1+r<RY>/<R>), where Y = <Y> + y,
    does not change in time. Note that R = rho2/(1+rY). This policy is similar
    to 'homdecay' as well as 'montecarlo_homdecay', but instead of simply
    constraining b' and kappa' to ensure decay in the evolution of <y^2>, b' and
    kappa' are specified as functions of an externally-specified hydrodynamics
    time scale, as a function of time. This policy is more similar to 'homdecay'
    than to 'montecarlo_homdecay' in that only requires the estimation
    of statistics, <R>, <r^2>, and <r^3>. A coefficients policy, in general, is
    used to specify how the coefficients are set at each time step during
    time-integration. Example: "coeff const", which selects constant
    coefficients policy, which sets constant coefficients before t = 0 and
    leaves the coefficients unchanged during time integration. Note that this
    option may behave differently depending on the particular equation or
    physical model.)"; }
};
using hydrotimescale =
  keyword< hydrotimescale_info, TAOCPP_PEGTL_STRING("hydrotimescale") >;

struct const_shear_info {
  using code = Code< S >;
  static std::string name() { return "prescribed constant shear"; }
  static std::string shortDescription() { return
    "Select constant shear coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the prescribed constant shear
    coefficients policy, used to compute a homogeneous free shear flow using the
    Langevin model. This policy (or model) prescribes a constant mean shear in
    the y direction and computes the dissipation of turbulent kinetic energy
    specifically for this flow. The flow is a fully developed homogeneous
    turbulent shear flow with a uniform mean velocity gradient in one direction
    (y) and the mean flow is in predominantly in the x direction. The flow is
    considered to be far from solid boundaries. See Pope, S.B. (2000). Turbulent
    flows (Cambridge: Cambridge University Press).)"; }
};
using const_shear =
  keyword< const_shear_info, TAOCPP_PEGTL_STRING("const_shear") >;

struct stationary_info {
  using code = Code< B >;
  static std::string name() { return "stationary"; }
  static std::string shortDescription() { return
     "Select the stationary coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the stationary coefficients
    policy. This policy will keep a stochastic differential equation at a
    constant statistically stationary state.)";
  }
};
using stationary =
  keyword< stationary_info, TAOCPP_PEGTL_STRING("stationary") >;

struct inst_velocity_info {
  using code = Code< V >;
  static std::string name() { return "instantaneous velocity"; }
  static std::string shortDescription() { return
    "Select the instantaneous velocity coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the instantaneous velocity coefficients
    policy. This is used to prescribe a coupling for instantaneous velocity to
    some other differential equation, e.g., to update Lagrangian particle
    position or to couple a mix model to velocity.)"; }
};
using inst_velocity =
  keyword< inst_velocity_info, TAOCPP_PEGTL_STRING("inst_velocity") >;

struct coeff_info {
  using code = Code< c >;
  static std::string name() { return "coefficients policy"; }
  static std::string shortDescription() { return
    "Select the coefficients policy"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a
    coefficients policy. This is used to specify how the coefficients are set
    at each time step during time-integration. Example: "coeff const",
    which selects constant coefficients policy, which sets constant
    coefficients before t = 0 and leaves the coefficients unchanged during
    time integration. Note that this option may behave differently depending
    on the particular equation or physical model.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' +
        kw::constcoeff::string() + "\' | \'" +
        kw::decay::string() + "\' | \'" +
        kw::homogeneous::string() + "\' | \'" +
        kw::homdecay::string() + "\' | \'" +
        kw::montecarlo_homdecay::string() + "\' | \'" +
        kw::hydrotimescale::string() + "\' | \'" +
        kw::const_shear::string() + "\' | \'" +
        kw::stationary::string() + "\' | \'" +
        kw::inst_velocity::string() + '\'';
    }
  };
};
using coeff = keyword< coeff_info,  TAOCPP_PEGTL_STRING("coeff") >;

struct walker_info {
  static std::string name() { return "walker"; }
  static std::string shortDescription() { return
    "Start configuration block of the random walker"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the walker. Walker, is a random walker,
    that allows temporal integration of a system of ordinary or stochastic
    differential equations (SDEs) of various types and the collection of
    arbitrary coupled statistics and probability density functions. Walker is
    intended as a general mathematical tool to analyze the behavior of SDEs
    and its statistics.)";
  }
};
using walker = keyword< walker_info, TAOCPP_PEGTL_STRING("walker") >;

struct npar_info {
  static std::string name() { return "npar"; }
  static std::string shortDescription() { return
    "Set total number of particles"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the total number of particles in a
    simulation.)";
  }
  struct expect {
    using type = uint64_t;
    static constexpr type lower = 1;
    static std::string description() { return "uint"; }
  };
};
using npar = keyword< npar_info, TAOCPP_PEGTL_STRING("npar") >;

struct nstep_info {
  static std::string name() { return "nstep"; }
  static std::string shortDescription() { return
    "Set number of time steps to take"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the number of time steps to take in a
    simulation. The number of time steps are used in conjunction with the
    maximmum time specified by keyword 'term': the simulation stops whichever is
    reached first. Both 'nstep' and 'term' can be left unspecified, in which
    case their default values are used. See also 'term'.)";
  }
  struct expect {
    using type = uint64_t;
    static constexpr type lower = 1;
    static std::string description() { return "uint"; }
  };
};
using nstep = keyword< nstep_info, TAOCPP_PEGTL_STRING("nstep") >;

struct term_info {
  static std::string name() { return "term"; }
  static std::string shortDescription() { return
    "Set maximum non-dimensional time to simulate"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the termination time in a simulation. The
    termination time and number of time steps, specified by 'nstep', are used in
    conjunction to determine when to stop a simulation: whichever is
    reached first. Both 'nstep' and 'term' can be left unspecified, in which
    case their default values are used. See also 'nstep'.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using term = keyword< term_info, TAOCPP_PEGTL_STRING("term") >;

struct t0_info {
  static std::string name() { return "t0"; }
  static std::string shortDescription() { return
    "Set starting non-dimensional time"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the starting time in a simulation.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using t0 = keyword< t0_info, TAOCPP_PEGTL_STRING("t0") >;

struct dt_info {
  static std::string name() { return "dt"; }
  static std::string shortDescription() { return
    "Select constant time step size"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the time step size that used as a
    constant during simulation. Setting 'cfl' and 'dt' are mutually
    exclusive. If both 'cfl' and 'dt' are set, 'dt' wins.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using dt = keyword< dt_info, TAOCPP_PEGTL_STRING("dt") >;

struct cfl_info {
  static std::string name() { return "CFL"; }
  static std::string shortDescription() { return
    "Set the Courant-Friedrichs-Lewy (CFL) coefficient"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the CFL coefficient for
    variable-time-step-size simulations. Setting 'cfl' and 'dt' are mutually
    exclusive. If both 'cfl' and 'dt' are set, 'dt' wins.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using cfl = keyword< cfl_info, TAOCPP_PEGTL_STRING("cfl") >;

struct ncomp_info {
  static std::string name() { return "ncomp"; }
  static std::string shortDescription() { return
    "Set number of scalar components for a system of differential equations"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the number of scalar
    components of a vector. 'ncomp' means "number of components". It is also
    used for specifying the number of scalar components of a transporter scalar
    (see also the keywords 'transport').)";
  }
  struct expect {
    using type = std::size_t;
    static constexpr type lower = 1;
    static std::string description() { return "uint"; }
  };
};
using ncomp = keyword< ncomp_info,  TAOCPP_PEGTL_STRING("ncomp") >;

struct nmat_info {
  static std::string name() { return "nmat"; }
  static std::string shortDescription() { return
    "Set number of materials for a system of differential equations"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the number of materials, e.g., for
    multi-material flow, see also the keyword 'multimat' and 'veleq'.)";
  }
  struct expect {
    using type = std::size_t;
    static constexpr type lower = 1;
    static std::string description() { return "uint"; }
  };
};
using nmat = keyword< nmat_info,  TAOCPP_PEGTL_STRING("nmat") >;

struct ttyi_info {
  static std::string name() { return "ttyi"; }
  static std::string shortDescription() { return
    "Set screen output interval"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the interval in time steps for screen
    output during a simulation.)";
  }
  struct expect {
    using type = uint32_t;
    static constexpr type lower = 0;
    static std::string description() { return "uint"; }
  };
};
using ttyi = keyword< ttyi_info, TAOCPP_PEGTL_STRING("ttyi") >;

struct pari_info {
  static std::string name() { return "pari"; }
  static std::string shortDescription() { return
    "Set particles output  interval"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the interval in time steps for particles
    output during a simulation.)";
  }
  struct expect {
    using type = uint32_t;
    static constexpr type lower = 1;
    static std::string description() { return "uint"; }
  };
};
using pari = keyword< pari_info, TAOCPP_PEGTL_STRING("pari") >;

struct interval_info {
  static std::string name() { return "interval"; }
  static std::string shortDescription() { return
    "Set interval (within a relevant block)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify an interval in time steps. This must be
    used within a relevant block.)";
  }
  struct expect {
    using type = uint32_t;
    static constexpr type lower = 0;
    static std::string description() { return "uint"; }
  };
};
using interval = keyword< interval_info, TAOCPP_PEGTL_STRING("interval") >;

struct statistics_info {
  static std::string name() { return "statistics"; }
  static std::string shortDescription() { return
    "Start of statistics input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to start a block in the input file containing the
    descriptions and settings of requested output for statistical moments.
    Example: "statistics <Y> <yy> end", which requests the first two moments of
    the flutcutating variable 'Y'. For more info on the structure of the
    statistics ... end block, see doc/pages/statistics_output.dox.)";
  }
};
using statistics = keyword< statistics_info, TAOCPP_PEGTL_STRING("statistics") >;

struct history_output_info {
  static std::string name() { return "history_output"; }
  static std::string shortDescription() { return
    "Start of history_output input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to start a block in the input file containing the
    descriptions and settings of requested history output.)";
  }
};
using history_output =
  keyword< history_output_info, TAOCPP_PEGTL_STRING("history_output") >;

struct field_output_info {
  static std::string name() { return "field_output"; }
  static std::string shortDescription() { return
    "Start of field_output input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to start a block in the input file containing the
    list and settings of requested field output.)";
  }
};
using field_output =
  keyword< field_output_info, TAOCPP_PEGTL_STRING("field_output") >;

struct outvar_density_info {
  static std::string name() { return "density"; }
  static std::string shortDescription() { return "Request density"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid density as an output
       variable.)";
  }
};
using outvar_density =
  keyword< outvar_density_info, TAOCPP_PEGTL_STRING("density") >;

struct outvar_xmomentum_info {
  static std::string name() { return "x-momentum"; }
  static std::string shortDescription() { return "Request x-momentum"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid x-momentum as an output
       variable.)";
  }
};
using outvar_xmomentum =
  keyword< outvar_xmomentum_info, TAOCPP_PEGTL_STRING("x-momentum") >;

struct outvar_ymomentum_info {
  static std::string name() { return "y-momentum"; }
  static std::string shortDescription() { return "Request y-momentum"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid y-momentum as an output
       variable.)";
  }
};
using outvar_ymomentum =
  keyword< outvar_ymomentum_info, TAOCPP_PEGTL_STRING("y-momentum") >;

struct outvar_zmomentum_info {
  static std::string name() { return "z-momentum"; }
  static std::string shortDescription() { return "Request z-momentum"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid z-momentum as an output
       variable.)";
  }
};
using outvar_zmomentum =
  keyword< outvar_zmomentum_info, TAOCPP_PEGTL_STRING("z-momentum") >;

struct outvar_specific_total_energy_info {
  static std::string name() { return "specific_total_energy"; }
  static std::string shortDescription() {
    return "Request total specific energy"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the specific total energy as an output
       variable.)";
  }
};
using outvar_specific_total_energy =
  keyword< outvar_specific_total_energy_info,
           TAOCPP_PEGTL_STRING("specific_total_energy") >;

struct outvar_volumetric_total_energy_info {
  static std::string name() { return "volumetric_total_energy"; }
  static std::string shortDescription() {
    return "Request total volumetric energy"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the volumetric total energy as an output
       variable.)";
  }
};
using outvar_volumetric_total_energy =
  keyword< outvar_volumetric_total_energy_info,
           TAOCPP_PEGTL_STRING("volumetric_total_energy") >;

struct outvar_xvelocity_info {
  static std::string name() { return "x-velocity"; }
  static std::string shortDescription() { return "Request x-velocity"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid x-velocity as an output
       variable.)";
  }
};
using outvar_xvelocity =
  keyword< outvar_xvelocity_info, TAOCPP_PEGTL_STRING("x-velocity") >;

struct outvar_yvelocity_info {
  static std::string name() { return "y-velocity"; }
  static std::string shortDescription() { return "Request y-velocity"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid y-velocity as an output
       variable.)";
  }
};
using outvar_yvelocity =
  keyword< outvar_yvelocity_info, TAOCPP_PEGTL_STRING("y-velocity") >;

struct outvar_zvelocity_info {
  static std::string name() { return "z-velocity"; }
  static std::string shortDescription() { return "Request z-velocity"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid z-velocity as an output
       variable.)";
  }
};
using outvar_zvelocity =
  keyword< outvar_zvelocity_info, TAOCPP_PEGTL_STRING("z-velocity") >;

struct outvar_pressure_info {
  static std::string name() { return "pressure"; }
  static std::string shortDescription() { return "Request pressure"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the fluid pressure as an output
       variable.)";
  }
};
using outvar_pressure =
  keyword< outvar_pressure_info, TAOCPP_PEGTL_STRING("pressure") >;

struct outvar_material_indicator_info {
  static std::string name() { return "material_indicator"; }
  static std::string shortDescription() { return "Request material_indicator"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the material indicator function as an
       output variable.)";
  }
};
using outvar_material_indicator =
  keyword< outvar_material_indicator_info,
    TAOCPP_PEGTL_STRING("material_indicator") >;

struct outvar_analytic_info {
  static std::string name() { return "analytic"; }
  static std::string shortDescription() { return "Request analytic solution"; }
  static std::string longDescription() { return
    R"(This keyword is used to request the analytic solution (if exist) as an
    output variable.)";
  }
};
using outvar_analytic =
  keyword< outvar_analytic_info, TAOCPP_PEGTL_STRING("analytic") >;

struct outvar_info {
  static std::string name() { return "outvar"; }
  static std::string shortDescription() { return
    "Start of var ... end input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to start a block in the input file containing a
       list of physics variables for output. The following keywords are allowed
       in an var ... end block:)"
    + std::string("\'")
    + outvar_density::string()+ "\', \'"
    + outvar_xmomentum::string()+ "\', \'"
    + outvar_ymomentum::string()+ "\', \'"
    + outvar_zmomentum::string()+ "\', \'"
    + outvar_specific_total_energy::string() + "\', \'"
    + outvar_volumetric_total_energy::string() + "\', \'"
    + outvar_xvelocity::string() + "\', \'"
    + outvar_yvelocity::string() + "\', \'"
    + outvar_zvelocity::string() + "\', \'"
    + outvar_pressure::string() + "\', \'"
    + outvar_material_indicator::string() + "\', \'"
    + outvar_analytic::string() + "\'.";
  }
};
using outvar = keyword< outvar_info, TAOCPP_PEGTL_STRING("var") >;

struct rngs_info {
  static std::string name() { return "rngs"; }
  static std::string shortDescription() { return
    "Start of a random number generators description input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to start a block in the input file containing the
    descriptions and settings of requested random number generators.
    Example: "rngs mkl_mcg59 seed 2134 uniform_method accurate end end" which
    enables the MCG59 generator from MKL using the seed 2134. For more info on
    the structure of the rngs ... end block, see
    doc/pages/rngs_input.dox.)";
  }
};
using rngs = keyword< rngs_info, TAOCPP_PEGTL_STRING("rngs") >;

struct rng_info {
  static std::string name() { return "rng"; }
  static std::string shortDescription() { return
    "Select random number generator (RNG) from pool of enabled RNGs"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a particular random number generator (RNG)
    from a pre-selected set of (enabled and configured) pool of RNGs. The pool
    is specified by the 'rngs ... end' block and it must precede the selection
    of an RNG.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + r123_threefry::string()   + "\' | \'"
                  + r123_philox::string()
                  #ifdef HAS_RNGSSE2
                                              + "\' | \'"
                  + rngsse_gm19::string()     + "\' | \'"
                  + rngsse_gm29::string()     + "\' | \'"
                  + rngsse_gm31::string()     + "\' | \'"
                  + rngsse_gm55::string()     + "\' | \'"
                  + rngsse_gm61::string()     + "\' | \'"
                  + rngsse_gq581::string()    + "\' | \'"
                  + rngsse_gq583::string()    + "\' | \'"
                  + rngsse_gq584::string()    + "\' | \'"
                  + rngsse_mt19937::string()  + "\' | \'"
                  + rngsse_lfsr113::string()  + "\' | \'"
                  + rngsse_mrg32k3a::string()
                  #endif
                  #ifdef HAS_MKL
                                              + "\' | \'"
                  + mkl_mcg31::string()       + "\' | \'"
                  + mkl_r250::string()        + "\' | \'"
                  + mkl_mrg32k3a::string()    + "\' | \'"
                  + mkl_mcg59::string()       + "\' | \'"
                  + mkl_wh::string()          + "\' | \'"
                  + mkl_mt19937::string()     + "\' | \'"
                  + mkl_mt2203::string()      + "\' | \'"
                  + mkl_sfmt19937::string()   + "\' | \'"
                  + mkl_sobol::string()       + "\' | \'"
                  + mkl_niederr::string()     + "\' | \'"
                  + mkl_iabstract::string()   + "\' | \'"
                  + mkl_dabstract::string()   + "\' | \'"
                  + mkl_sabstract::string()   + "\' | \'"
                  + mkl_nondeterm::string()   + "\' "
                  #else
                                              + "\' "
                  #endif
             + "Remember: the RNG must be listed in the pool before it can be "
               "selected via this keyword!";
    }
  };
};
using rng = keyword< rng_info, TAOCPP_PEGTL_STRING("rng") >;

struct sde_omega_info {
  static std::string name() { return "omega"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) omega)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "omega 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_omega = keyword< sde_omega_info, TAOCPP_PEGTL_STRING("omega") >;

struct sde_c0_info {
  static std::string name() { return "C0"; }
  static std::string shortDescription() { return
    R"(Set Langevin SDE parameter C0)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to parameterize the
    Langevin model for the fluctuating velocity in homogeneous variable-density
    turbulence. Example: "C0 2.1".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using sde_c0 = keyword< sde_c0_info,  TAOCPP_PEGTL_STRING("C0") >;

struct gravity_info {
  static std::string name() { return "gravity"; }
  static std::string shortDescription() { return
    R"(Set Langevin SDE parameter gravity)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of 3 real numbers used to
    parameterize the Langevin model for the fluctuating velocity in homogeneous
    variable-density turbulence, prescribing a gravy body force in the three
    coordinate directions, x, y, z. Example: "gravity 0.0 0.2 1.0 end".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "3 reals"; }
  };
};
using gravity = keyword< gravity_info, TAOCPP_PEGTL_STRING("gravity") >;

struct sde_c3_info {
  static std::string name() { return "C3"; }
  static std::string shortDescription() { return
    R"(Set gamma (dissipation) SDE parameter C3)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to parameterize the
    gamma distribution dissipation (turbulence frequency) model for particles
    Example: "C3 1.0".)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using sde_c3 = keyword< sde_c3_info,  TAOCPP_PEGTL_STRING("C3") >;

struct sde_c4_info {
  static std::string name() { return "C4"; }
  static std::string shortDescription() { return
    R"(Set gamma (dissipation) SDE parameter C4)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to parameterize the
    gamma distribution dissipation (turbulence frequency) model for particles
    Example: "C4 0.25".)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using sde_c4 = keyword< sde_c4_info,  TAOCPP_PEGTL_STRING("C4") >;

struct sde_com1_info {
  static std::string name() { return "COM1"; }
  static std::string shortDescription() { return
    R"(Set gamma (dissipation) SDE parameter COM1)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to parameterize the
    gamma distribution dissipation (turbulence frequency) model for particles
    Example: "COM1 0.44".)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using sde_com1 = keyword< sde_com1_info,  TAOCPP_PEGTL_STRING("COM1") >;

struct sde_com2_info {
  static std::string name() { return "COM2"; }
  static std::string shortDescription() { return
    R"(Set gamma (dissipation) SDE parameter COM2)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to parameterize the
    gamma distribution dissipation (turbulence frequency) model for particles
    Example: "COM2 0.9".)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using sde_com2 = keyword< sde_com2_info,  TAOCPP_PEGTL_STRING("COM2") >;

struct sde_b_info {
  static std::string name() { return "b"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) b)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "b 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_b = keyword< sde_b_info,  TAOCPP_PEGTL_STRING("b") >;

struct sde_S_info {
  static std::string name() { return "S"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) S)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "S 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_S = keyword< sde_S_info,  TAOCPP_PEGTL_STRING("S") >;

struct sde_kappa_info {
  static std::string name() { return "kappa"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) kappa)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "kappa 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_kappa = keyword< sde_kappa_info,  TAOCPP_PEGTL_STRING("kappa") >;

struct sde_bprime_info {
  static std::string name() { return "bprime"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) bprime)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "bprime 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = sde_b_info::expect::type;
    static std::string description() { return "real(s)"; }
  };
};
using sde_bprime = keyword< sde_bprime_info,  TAOCPP_PEGTL_STRING("bprime") >;

struct sde_kappaprime_info {
  static std::string name() { return "kappaprime"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) kappaprime)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "kappaprime 5.0 2.0 3.0 end". The length of the vector depends on the
    particular type of SDE system and is controlled by the preceding keyword
    'ncomp'.)"; }
  struct expect {
    using type = sde_kappa_info::expect::type;
    static std::string description() { return "real(s)"; }
  };
};
using sde_kappaprime = keyword< sde_kappaprime_info,  TAOCPP_PEGTL_STRING("kappaprime") >;

struct sde_c_info {
  static std::string name() { return "c"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) c)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "c 5.0 2.0 3.0 end". The length of the vector depends on the particular type
    of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_c = keyword< sde_c_info,  TAOCPP_PEGTL_STRING("c") >;

struct sde_sigmasq_info {
  static std::string name() { return "sigmasq"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) sigmasq)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
       "sigmasq
          4.0  2.5   1.1
              32.0   5.6
                    23.0
        end"
    The length of the vector depends on the
    particular type of SDE system and is controlled by the preceding keyword
    'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_sigmasq = keyword< sde_sigmasq_info, TAOCPP_PEGTL_STRING("sigmasq") >;

struct sde_cov_info {
  static std::string name() { return "cov"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) cov)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
       "cov
          4.0  2.5   1.1
              32.0   5.6
                    23.0
        end"
    The length of the vector depends on the
    particular type of SDE system and is controlled by the preceding keyword
    'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_cov = keyword< sde_cov_info, TAOCPP_PEGTL_STRING("cov") >;

struct sde_theta_info {
  static std::string name() { return "theta"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) theta)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "theta 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_theta = keyword< sde_theta_info, TAOCPP_PEGTL_STRING("theta") >;

struct sde_mu_info {
  static std::string name() { return "mu"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) mu)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "mu 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_mu = keyword< sde_mu_info, TAOCPP_PEGTL_STRING("mu") >;

struct sde_mean_info {
  static std::string name() { return "mean"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) mean)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "mean 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_mean = keyword< sde_mean_info, TAOCPP_PEGTL_STRING("mean") >;

struct sde_T_info {
  static std::string name() { return "T"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) T)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "T 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_T = keyword< sde_T_info, TAOCPP_PEGTL_STRING("T") >;

struct sde_lambda_info {
  static std::string name() { return "lambda"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) lambda)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "lambda 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_lambda = keyword< sde_lambda_info, TAOCPP_PEGTL_STRING("lambda") >;

struct spike_info {
  static std::string name() { return "spike"; }
  static std::string shortDescription() { return
    R"(Configure a delta spike)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the configuration of delta spikes for,
    the delta initialization policy. The configuration is given by an even set
    of real numbers inside a spike...end block. Example: "spike 0.1 1.0 end",
    which specifies a delta spike at sample space position 0.1 with relative
    height 1.0. The height must be between [0.0...1.0] inclusive and specifies a
    relative probability. See also the help on keyword icdelta.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "even reals"; }
  };
};
using spike = keyword< spike_info, TAOCPP_PEGTL_STRING("spike") >;

struct icdelta_info {
  static std::string name() { return "icdelta"; }
  static std::string shortDescription() { return
    R"(Introduce a icdelta...end block used to configure delta spikes)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a icdelta...end block in which delta
    spikes are configured for the delta initialization policy. Example:
    "init jointdelta" - select joint delta init-policy,"icdelta spike 0.1 0.3
    0.9 0.7 end end" - prescribe a univariate distribution that consists of two
    delta-spikes at sample space positions 0.1 and 0.9 with spike heights 0.3
    and 0.7, respectively. Note that the sum of the heights must add up to
    unity. See also the help on keyword jointdelta and spike.)"; }
};
using icdelta = keyword< icdelta_info, TAOCPP_PEGTL_STRING("icdelta") >;

struct betapdf_info {
  static std::string name() { return "betapdf"; }
  static std::string shortDescription() { return
    R"(Configure a beta distribution)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the configuration of beta distributions
    for the beta initialization policy. The configuration is given by four
    real numbers inside a betapdf...end block. Example: "betapdf 0.2 0.3 0.0 1.0
    end", which specifies a univariate beta distribution with shape parameters
    0.2 and 0.3, displacement 0.0, and scale 1.0. See also the help on keyword
    icbeta.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "4 reals"; }
  };
};
using betapdf = keyword< betapdf_info, TAOCPP_PEGTL_STRING("betapdf") >;

struct gaussian_info {
  static std::string name() { return "Gaussian"; }
  static std::string shortDescription() { return
    R"(Configure a Gaussian distribution)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the configuration of Gaussian
    distributions for the jointgaussian initialization policy. The configuration
    is given by two real numbers inside a gaussian...end block. Example:
    "gaussian 0.2 0.3 end", which specifies a Gaussian distribution with 0.2
    mean and 0.3 variance. See also the help on keyword icgaussian.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "2 reals"; }
  };
};
using gaussian = keyword< gaussian_info, TAOCPP_PEGTL_STRING("gaussian") >;

struct icbeta_info {
  static std::string name() { return "icbeta"; }
  static std::string shortDescription() { return
    R"(Introduce an icbeta...end block used to configure beta distributions)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an icbeta...end block in which beta
    distributions are configured for the beta initialization policy. Example:
    "init jointbeta" - select beta init-policy,"icbeta betapdf 0.2 0.3 0.0 1.0
    end end" - prescribe a univariate beta distribution with shape parameters
    0.2 and 0.3, displacement 0.0, and scale 1.0. See also the help on keyword
    jointbeta and betapdf.)"; }
};
using icbeta = keyword< icbeta_info, TAOCPP_PEGTL_STRING("icbeta") >;

struct icgaussian_info {
  static std::string name() { return "icgaussian"; }
  static std::string shortDescription() {
    return R"(Configure a joint uncorrelated Gaussian as initial condition)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an icgaussian...end block in which
    Gaussian distributions are configured for the jointgaussian initialization
    policy.  Example: "init jointgaussian" - select jointgaussian
    init-policy,"icgaussian gaussian 0.2 0.3 end end" - prescribes a univariate
    Gaussian distribution with 0.2 mean and 0.3 variance. See also the help on
    keyword jointgaussian and gaussian.)"; }
};
using icgaussian = keyword< icgaussian_info, TAOCPP_PEGTL_STRING("icgaussian") >;

struct icjointgaussian_info {
  static std::string name() { return "icjointgaussian"; }
  static std::string shortDescription() {
    return R"(Configure an joint correlated Gaussian as initial condition)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an icjointgaussian...end block in which
    a multi-variate joint Gaussian distribution is configured for the
    jointgaussian initialization policy. Example: "init jointgaussian" - select
    jointgaussian init-policy, "
      icjointgaussian
        mean 0.0 0.5 1.0 end
        cov
          4.0  2.5   1.1
              32.0   5.6
                    23.0
        end
      end" - prescribes a tri-variate joint Gaussian distribution with means
      0.0, 0.5 and 1.0, and a covariance matrix which must be symmetric positive
    definite. See also the help on keyword jointgaussian and gaussian.)"; }
};
using icjointgaussian =
  keyword< icjointgaussian_info, TAOCPP_PEGTL_STRING("icjointgaussian") >;

struct gammapdf_info {
  static std::string name() { return "gammapdf"; }
  static std::string shortDescription() { return
    R"(Configure a gamma distribution)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the configuration of gamma distributions
    for the gamma initialization policy. The configuration is given by two
    real numbers inside a gammapdf...end block. Example: "gammapdf 0.2 0.3
    end", which specifies a univariate gamma distribution with shape and scale
    parameters 0.2 and 0.3, respectively. See also the help on keyword
    icgamma.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "2 reals"; }
  };
};
using gammapdf = keyword< gammapdf_info, TAOCPP_PEGTL_STRING("gammapdf") >;

struct icgamma_info {
  static std::string name() { return "icgamma"; }
  static std::string shortDescription() { return
    R"(Configure a gamma distribution as initial condition)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an icgamma...end block in which gamma
    distributions are configured for the gamma initialization policy. Example:
    "init jointgamma" - select gamma init-policy,"icgamma gammapdf 0.2 0.3
    end end" - prescribe a univariate gamma distribution with shape and scale
    parameters 0.2 and 0.3, respectively. See also the help on keyword
    jointgamma and gammapdf.)"; }
};
using icgamma = keyword< icgamma_info, TAOCPP_PEGTL_STRING("icgamma") >;

struct dirichletpdf_info {
  static std::string name() { return "dirichletpdf"; }
  static std::string shortDescription() { return
    R"(Configure a Dirichlet distribution)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the configuration of a Dirichlet
    distribution for the Dirichlet initialization policy. The configuration is
    given by a vector of positive real numbers inside a dirichletpdf...end
    block. Example: "dirichletpdf 0.1 0.3 0.2 end" - prescribe a Dirichlet
    distribution with shape parameters 0.1, 0.3, and 0.2. See also the help on
    keyword icdirichlet.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "reals"; }
  };
};
using dirichletpdf =
  keyword< dirichletpdf_info, TAOCPP_PEGTL_STRING("dirichletpdf") >;

struct icdirichlet_info {
  static std::string name() { return "icdirichlet"; }
  static std::string shortDescription() { return
    R"(Configure a Dirichlet PDF as initial condition)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an icdirichlet...end block in which a
    Dirichlet distribution is configured for the Dirichlet initialization
    policy)"; }
};
using icdirichlet =
  keyword< icdirichlet_info, TAOCPP_PEGTL_STRING("icdirichlet") >;

struct velocity_info {
  static std::string name() { return "velocity"; }
  static std::string shortDescription() { return "Specify velocity"; }
  static std::string longDescription() { return
    R"(This keyword is used to configure a velocity vector, used for, e.g.,
    boundary or initial conditions.)";
  }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using velocity = keyword< velocity_info, TAOCPP_PEGTL_STRING("velocity") >;

struct mass_info {
  static std::string name() { return "mass"; }
  static std::string shortDescription() { return "Specify mass"; }
  static std::string longDescription() { return
    R"(This keyword is used to configure the mass
       and associated volume within a box.)";
  }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using mass = keyword< mass_info, TAOCPP_PEGTL_STRING("mass") >;

struct density_info {
  static std::string name() { return "density"; }
  static std::string shortDescription() { return "Specify density"; }
  static std::string longDescription() { return
    R"(This keyword is used to configure a density, used for, e.g., boundary or
    initial conditions.)";
  }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using density = keyword< density_info, TAOCPP_PEGTL_STRING("density") >;

struct pressure_info {
  static std::string name() { return "pressure"; }
  static std::string shortDescription() { return "Specify pressure"; }
  static std::string longDescription() { return
    R"(This keyword is used to configure a pressure, used for, e.g., boundary or
    initial conditions.)";
  }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pressure = keyword< pressure_info, TAOCPP_PEGTL_STRING("pressure") >;

struct energy_info {
  static std::string name() { return "energy"; }
  static std::string shortDescription() { return
    "Specify energy per unit mass"; }
  static std::string longDescription() { return
    R"(This keyword is used to configure energy per unit mass, used for, e.g.,
    boundary or initial conditions.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using energy = keyword< energy_info, TAOCPP_PEGTL_STRING("energy") >;

struct energy_content_info {
  static std::string name() { return "energy_content"; }
  static std::string shortDescription() { return
    "Specify energy per unit volume";
  }
  static std::string longDescription() { return
    R"(This keyword is used to configure energy per unit volume, used for, e.g.,
    boundary or initial conditions.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using energy_content =
  keyword< energy_content_info, TAOCPP_PEGTL_STRING("energy_content") >;

struct temperature_info {
  static std::string name() { return "temperature"; }
  static std::string shortDescription() { return "Specify temperature"; }
  static std::string longDescription() { return
    R"(This keyword is used to configure temperature, used for, e.g.,
    boundary or initial conditions.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using temperature =
  keyword< temperature_info, TAOCPP_PEGTL_STRING("temperature") >;

struct lua_info {
  static std::string name() { return "lua"; }
  static std::string shortDescription() { return
    R"(Introduce a lua ... end block to inject lua code in control files)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a lua ... end block which can be used
    to inject arbitrary Lua code into control files. For more info on the lua
    language, see https://www.lua.org.)"; }
};
using lua = keyword< lua_info, TAOCPP_PEGTL_STRING("lua") >;

struct xmin_info {
  static std::string name() { return "xmin"; }
  static std::string shortDescription() { return "Minimum x coordinate"; }
  static std::string longDescription() { return
    R"(This keyword used to configure a minimum x coordinate, e.g., to specify
    a box.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using xmin = keyword< xmin_info, TAOCPP_PEGTL_STRING("xmin") >;

struct xmax_info {
  static std::string name() { return "xmax"; }
  static std::string shortDescription() { return "Maximum x coordinate"; }
  static std::string longDescription() { return
    R"(This keyword used to configure a maximum x coordinate, e.g., to specify
    a box.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using xmax = keyword< xmax_info, TAOCPP_PEGTL_STRING("xmax") >;

struct ymin_info {
  static std::string name() { return "ymin"; }
  static std::string shortDescription() { return "Minimum y coordinate"; }
  static std::string longDescription() { return
    R"(This keyword used to configure a minimum y coordinate, e.g., to specify
    a box.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using ymin = keyword< ymin_info, TAOCPP_PEGTL_STRING("ymin") >;

struct ymax_info {
  static std::string name() { return "ymax"; }
  static std::string shortDescription() { return "Maximum y coordinate"; }
  static std::string longDescription() { return
    R"(This keyword used to configure a maximum y coordinate, e.g., to specify
    a box.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using ymax = keyword< ymax_info, TAOCPP_PEGTL_STRING("ymax") >;

struct zmin_info {
  static std::string name() { return "zmin"; }
  static std::string shortDescription() { return "Minimum z coordinate"; }
  static std::string longDescription() { return
    R"(This keyword used to configure a minimum z coordinate, e.g., to specify
    a box.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using zmin = keyword< zmin_info, TAOCPP_PEGTL_STRING("zmin") >;

struct zmax_info {
  static std::string name() { return "zmax"; }
  static std::string shortDescription() { return "Maximum z coordinate"; }
  static std::string longDescription() { return
    R"(This keyword used to configure a maximum z coordinate, e.g., to specify
    a box.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using zmax = keyword< zmax_info, TAOCPP_PEGTL_STRING("zmax") >;

struct impulse_info {
  static std::string name() { return "impulse"; }
  static std::string shortDescription() { return
    "Select the impulse initiation type, e.g., for a box IC"; }
  static std::string longDescription() { return
    R"(This keyword can be used to select the 'impulse' initiation/assignment
    type for box initial conditions. It simply assigns the prescribed values to
    mesh points within a configured box at t=0.)"; }
};
using impulse = keyword< impulse_info, TAOCPP_PEGTL_STRING("impulse") >;

struct linear_info {
  static std::string name() { return "linear"; }
  static std::string shortDescription() { return
    "Select the linear initiation type, e.g., for a box IC"; }
  static std::string longDescription() { return
    R"(This keyword can be used to select the 'linear' initiation/assignment
    type for box initial conditions. Linear initiation uses a linear function
    in time and space, configured with an initiation point in space, an initial
    radius around the point, and a constant velocity that grows a sphere in time
    (and space) linearly and assigns values to mesh points falling within a
    growing sphere within a configured box.)"; }
};
using linear = keyword< linear_info, TAOCPP_PEGTL_STRING("linear") >;

struct initiate_info {
  static std::string name() { return "IC box initiate type"; }
  static std::string shortDescription() { return "Initiation/assignemt type"; }
  static std::string longDescription() { return
    R"(This keyword is used to select an initiation type to configure how
    values are assigned, e.g., for a box initial condition. This can be used to
    specify, how the values are assigned to mesh nodes within a box. Examples:
    (1) impulse: assign the full values at t=0 for all points in a box,
    (2) linear: use a linear function in time and space, configured with an
    initiation point in space, an initial radius around the point, and a
    velocity that grows a sphere in time (and space) linearly and assigns values
    to mesh points falling within a growing sphere within a configured box.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + impulse::string() + "\' | \'"
                  + linear::string() + '\'';
    }
  };
};
using initiate = keyword< initiate_info, TAOCPP_PEGTL_STRING("initiate") >;

struct box_info {
  static std::string name() { return "box"; }
  static std::string shortDescription() { return
    R"(Introduce a box ... end block used to assign initial conditions)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a box ... end block used to assign
    initial conditions within a box given by spatial coordinates. Example:
    box x- 0.5 x+ 1.5 y- -0.5 y+ 0.5 z- -0.5 z+ 0.5 density 1.2 end pressure
    1.4 end end", which specifies a box with extends within which the density
    will be set to 1.2 and the pressure to be 1.4. Besides the box dimensions,
    the following physics keywords are allowed in a box ... end block:)"
    + std::string("\'")
    + mass::string()+ "\', \'"
    + density::string()+ "\', \'"
    + velocity::string() + "\', \'"
    + energy::string() + "\', \'"
    + energy_content::string() + "\', \'"
    + temperature::string() + "\', \'"
    + pressure::string() + "\'."; }
};
using box = keyword< box_info, TAOCPP_PEGTL_STRING("box") >;

struct ic_info {
  static std::string name() { return "ic"; }
  static std::string shortDescription() { return
    R"(Introduce an ic...end block used to configure initial conditions)"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an ic...end block used to set initial
    conditions. Keywords allowed in a ic ... end block: )" + std::string("\'")
    + mass::string()+ "\', \'"
    + density::string()+ "\', \'"
    + velocity::string() + "\', \'"
    + pressure::string() + "\', \'"
    + energy::string() + "\', \'"
    + temperature::string() + "\', \'"
    + box::string() + "\'.";
  }
};
using ic = keyword< ic_info, TAOCPP_PEGTL_STRING("ic") >;

struct depvar_info {
  static std::string name() { return "depvar"; }
  static std::string shortDescription() { return
    "Select dependent variable (in a relevant block)"; }
  static std::string longDescription() { return
    R"(Dependent variable, e.g, in differential equations.)"; }
  struct expect {
    using type = char;
    static std::string description() { return "character"; }
  };
};
using depvar = keyword< depvar_info, TAOCPP_PEGTL_STRING("depvar") >;

struct sde_rho2_info {
  static std::string name() { return "rho2"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) rho2)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "rho2 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_rho2 = keyword< sde_rho2_info,  TAOCPP_PEGTL_STRING("rho2") >;

struct sde_rho_info {
  static std::string name() { return "rho"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) rho)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "rho 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_rho = keyword< sde_rho_info,  TAOCPP_PEGTL_STRING("rho") >;

struct mean_gradient_info {
  static std::string name() { return "Prescribed mean gradient"; }
  static std::string shortDescription() { return
    R"(Set prescribed mean gradient)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "mean_gradient 1.0 1.0 0.0 end". One use of a mean gradient vector is to
    specify a prescribed mean scalar gradient in 3 spatial directions for a
    scalar transprot equation.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using mean_gradient = keyword< mean_gradient_info,
  TAOCPP_PEGTL_STRING("mean_gradient") >;

struct sde_rcomma_info {
  static std::string name() { return "rcomma"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) rcomma)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "rcomma 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_rcomma = keyword< sde_rcomma_info, TAOCPP_PEGTL_STRING("rcomma") >;

struct sde_r_info {
  static std::string name() { return "r"; }
  static std::string shortDescription() { return
    R"(Set SDE parameter(s) r)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of stochastic differential equations. Example:
    "r 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of SDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using sde_r = keyword< sde_r_info, TAOCPP_PEGTL_STRING("r") >;

struct dirichlet_info {
  static std::string name() { return "Dirichlet"; }
  static std::string shortDescription() { return
    "Start configuration block for the Dirichlet SDE"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a dirichlet ... end block, used to
    specify the configuration of a system of stochastic differential
    equations (SDEs), whose invariant is the Dirichlet distribution. For more
    details on the Dirichlet SDE, see https://doi.org/10.1155/2013/842981.
    Keywords allowed in a dirichlet ... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_b::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_kappa::string() + "\'. "
    + R"(For an example dirichlet ... end block, see
      doc/html/walker_example_dirichlet.html.)";
  }
};
using dirichlet = keyword< dirichlet_info,  TAOCPP_PEGTL_STRING("dirichlet") >;

struct mixdirichlet_info {
  static std::string name() { return "MixDirichlet"; }
  static std::string shortDescription() { return
    "Start configuration block for the Mixture Dirichlet SDE"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a mixdirichlet ... end block, used to
    specify the configuration of a system of stochastic differential
    equations (SDEs), whose invariant is the Dirichlet distribution constrained
    to model multi-material mixing in turbulent flows. For more
    details on the Dirichlet SDE, see https://doi.org/10.1155/2013/842981.
    Keywords allowed in a mixdirichlet ... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_b::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_kappa::string() + "\', \'"
    + sde_rho2::string() + "\', \'"
    + sde_r::string() + "\'. "
    + R"(For an example mixdirichlet ... end block, see
      doc/html/walker_example_mixdirichlet.html.)";
  }
};
using mixdirichlet =
  keyword< mixdirichlet_info, TAOCPP_PEGTL_STRING("mixdirichlet") >;

struct gendir_info {
  static std::string name() { return "Generalized Dirichlet"; }
  static std::string shortDescription() { return
    "Start configuration block for the generalized Dirichlet SDE"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a gendir ... end
    block, used to specify the configuration of a system of stochastic
    differential equations (SDEs), whose invariant is Lochner's generalized
    Dirichlet distribution. For more details on the generalized Dirichlet
    SDE, see https://doi.org/10.1063/1.4822416. Keywords allowed in a gendir
    ... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_b::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_c::string() + "\', \'"
    + sde_kappa::string() + "\'. "
    + R"(For an example gendir... end block, see
      doc/html/walker_example_gendir.html.)";
  }
};
using gendir = keyword< gendir_info, TAOCPP_PEGTL_STRING("gendir") >;

struct wrightfisher_info {
  static std::string name() { return "Wright-Fisher"; }
  static std::string shortDescription() { return
    "Start configuration block for the Wright-Fisher SDE"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a wright_fisher ... end block, used
    to specify the configuration of a system of stochastic differential
    equations (SDEs), whose invariant is the Dirichlet distribution. For more
    details on the Wright-Fisher SDE, see
    http://www.sciencedirect.com/science/article/pii/S0040580912001013.
    Keywords allowed in a wright-fisher... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_omega::string() + "\'. "
    + R"(For an example wright-fisher ... end block, see
      doc/html/walker_example_wf.html.)";
  }
};
using wrightfisher =
  keyword< wrightfisher_info, TAOCPP_PEGTL_STRING("wright-fisher") >;

struct skewnormal_info {
  static std::string name() { return "Skew-Normal"; }
  static std::string shortDescription() { return
    "Start configuration block for the Skew-normal SDE"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a skew-normal ... end block, used
    to specify the configuration of a system of stochastic differential
    equations (SDEs), whose invariant is the joint skew-normal distribution.
    For more details on the skew-normal distribution, see
    http://www.jstor.org/stable/2337278. Keywords allowed in an skew-normal ...
    end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_sigmasq::string() + "\', \'"
    + sde_T::string() + "\', \'"
    + sde_lambda::string() + "\'. "
    + R"(For an example skew-normal... end block, see
      doc/html/walker_example_skewnormal.html.)";
  }
};
using skewnormal = keyword< skewnormal_info,  TAOCPP_PEGTL_STRING("skew-normal") >;

struct beta_info {
  static std::string name() { return "Beta"; }
  static std::string shortDescription() { return
    "Introduce the beta SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a beta ... end block, used to specify
    the configuration of a system of stochastic differential equations (SDEs),
    with linear drift and quadratic diagonal diffusion, whose invariant is the
    joint beta distribution. For more details on the beta SDE, see
    https://doi.org/10.1080/14685248.2010.510843 and src/DiffEq/Beta/Beta.hpp.
    Keywords allowed in a beta ... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_b::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_kappa::string() + "\'. "
    + R"(For an example beta ... end block, see
      doc/html/walker_example_beta.html.)";
  }
};
using beta = keyword< beta_info, TAOCPP_PEGTL_STRING("beta") >;

struct numfracbeta_info {
  static std::string name() { return "Number-fraction beta"; }
  static std::string shortDescription() { return
    "Introduce the numfracbeta SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a numfracbeta ... end block, used to
    specify the configuration of a system of number-fraction beta SDEs, a system
    of stochastic differential equations (SDEs), in which, in addition to the
    dependent variable, computed with linear drift and quadratic diagonal
    diffusion (whose invariant is joint beta), two additional variables are
    computed. In other words, this is a beta SDE but there are two additional
    stochastic variables computed based on the beta SDE. If X is governed by the
    beta SDE, then the number-fraction beta SDE additionally governs rho(X) and
    V(X), where both rho and V are random variables, computed by rho(X) = rho2
    ( 1 - r' X ), and V(X) = 1 / [ rho2 ( 1 - r'X ) ]. For more details on the
    beta SDE, see https://doi.org/10.1080/14685248.2010.510843 and
    src/DiffEq/Beta/Beta.hpp. Keywords allowed in a numfracbeta ... end block: )"
    + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_b::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_kappa::string() + "\', \'"
    + sde_rho2::string() + "\', \'"
    + sde_rcomma::string() + "\'. "
    + R"(For an example numfracbeta ... end block, see
      doc/html/walker_example_numfracbeta.html.)";
  }
};
using numfracbeta = keyword< numfracbeta_info, TAOCPP_PEGTL_STRING("numfracbeta") >;

struct massfracbeta_info {
  static std::string name() { return "Mass-fraction beta"; }
  static std::string shortDescription() { return
    "Introduce the massfracbeta SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a massfracbeta ... end block, used to
    specify the configuration of a system of number-fraction beta SDEs, a system
    of stochastic differential equations (SDEs), in which, in addition to the
    dependent variable, computed with linear drift and quadratic diagonal
    diffusion (whose invariant is joint beta), two additional variables are
    computed. In other words, this is a beta SDE but there are two additional
    stochastic variables computed based on the beta SDE. If Y is governed by the
    beta SDE, then the mass-fraction beta SDE additionally governs rho(Y) and
    V(Y), where both rho and V are random variables, computed by rho(Y) = rho2 /
    ( 1 + r Y ), and V(Y) = ( 1 + r Y ) / rho2. For more details on the beta
    SDE, see
    https://doi.org/10.1080/14685248.2010.510843 and src/DiffEq/Beta/Beta.hpp.
    Keywords allowed in a massfracbeta ... end block: )"
    + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_b::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_kappa::string() + "\', \'"
    + sde_rho2::string() + "\', \'"
    + sde_r::string() + "\'. "
    + R"(For an example massfracbeta ... end block, see
      doc/html/walker_example_massfracbeta.html.)";
  }
};
using massfracbeta = keyword< massfracbeta_info, TAOCPP_PEGTL_STRING("massfracbeta") >;

struct mixnumfracbeta_info {
  static std::string name() { return "Mix number-fraction beta"; }
  static std::string shortDescription() { return
    "Introduce the mixnumfracbeta SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a mixnumfracbeta ... end block, used
    to specify the configuration of a system of mix number-fraction beta SDEs, a
    system of stochastic differential equations (SDEs), whose solution is the
    joint beta distribution and in which the usual beta SDE parameters b and
    kappa are specified via functions that constrain the beta SDE to be
    consistent with the turbulent mixing process. The mix number-fraction beta
    SDE is similar to the number-fraction beta SDE, only the process is made
    consistent with the no-mix and fully mixed limits via the specification of
    the SDE coefficients b and kappa. As in the number-fraction beta SDE, X is
    governed by the beta SDE and two additional stochastic variables are
    computed. However, in the mix number-fraction beta SDE the parameters b and
    kappa are given by b = Theta * b' and kappa = kappa' * <x^2>, where Theta =
    1 - <x^2> / [ <X> ( 1 - <X> ], the fluctuation about the mean, <X>, is
    defined as usual: x = X - <X>, and b' and kappa' are user-specified
    constants. Similar to the number-fraction beta SDE, there two additional
    random variables computed besides, X, and they are rho(X) and V(X). For more
    detail on the number-fraction beta SDE, see the help on keyword
    'numfracbeta'. For more details on the beta SDE, see
    https://doi.org/10.1080/14685248.2010.510843 and src/DiffEq/Beta/Beta.h.
    Keywords allowed in a mixnumfracbeta ... end block: )"
    + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_bprime::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_kappaprime::string() + "\', \'"
    + sde_rho2::string() + "\', \'"
    + sde_rcomma::string() + "\'. "
    + R"(For an example mixnumfracbeta ... end block, see
      doc/html/walker_example_mixnumfracbeta.html.)";
  }
};
using mixnumfracbeta =
  keyword< mixnumfracbeta_info, TAOCPP_PEGTL_STRING("mixnumfracbeta") >;

struct eq_A005H_info {
  static std::string name() { return "eq_A005H"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.05, IC:light<<heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.05, IC: light << heavy.)"; }
};
using eq_A005H = keyword< eq_A005H_info, TAOCPP_PEGTL_STRING("eq_A005H") >;

struct eq_A005S_info {
  static std::string name() { return "eq_A005S"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.05, IC:light=heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.05, IC: light = heavy.)"; }
};
using eq_A005S = keyword< eq_A005S_info, TAOCPP_PEGTL_STRING("eq_A005S") >;

struct eq_A005L_info {
  static std::string name() { return "eq_A005L"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.05, IC:light>>heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.05, IC: light >> heavy.)"; }
};
using eq_A005L = keyword< eq_A005L_info, TAOCPP_PEGTL_STRING("eq_A005L") >;

struct eq_A05H_info {
  static std::string name() { return "eq_A05H"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.5, IC:light<<heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.5, IC: light << heavy.)"; }
};
using eq_A05H = keyword< eq_A05H_info, TAOCPP_PEGTL_STRING("eq_A05H") >;

struct eq_A05S_info {
  static std::string name() { return "eq_A05S"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.5, IC:light=heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.5, IC: light = heavy.)"; }
};
using eq_A05S = keyword< eq_A05S_info, TAOCPP_PEGTL_STRING("eq_A05S") >;

struct eq_A05L_info {
  static std::string name() { return "eq_A05L"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.5, IC:light>>heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.5, IC: light >> heavy.)"; }
};
using eq_A05L = keyword< eq_A05L_info, TAOCPP_PEGTL_STRING("eq_A05L") >;

struct eq_A075H_info {
  static std::string name() { return "eq_A075H"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.75, IC:light<<heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.75, IC: light << heavy.)"; }
};
using eq_A075H = keyword< eq_A075H_info, TAOCPP_PEGTL_STRING("eq_A075H") >;

struct eq_A075S_info {
  static std::string name() { return "eq_A075S"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.75, IC:light=heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.75, IC: light = heavy.)"; }
};
using eq_A075S = keyword< eq_A075S_info, TAOCPP_PEGTL_STRING("eq_A075S") >;

struct eq_A075L_info {
  static std::string name() { return "eq_A075L"; }
  static std::string shortDescription() { return "Select inverse equilibrium "
   "hydro time scale from DNS of HRT, A=0.75, IC:light>>heavy"; }
  static std::string longDescription() { return
    R"(Inverse equilibrium hydrodynamics time scale from DNS of homogeneous
       Rayleigh-Taylor instability, tau_eq, A = 0.75, IC: light >> heavy.)"; }
};
using eq_A075L = keyword< eq_A075L_info, TAOCPP_PEGTL_STRING("eq_A075L") >;

struct hydrotimescales_info {
  static std::string name() { return "hydrotimescales"; }
  static std::string shortDescription() { return
    R"(Set MixMassFractionBeta SDE parameter(s) hydrotimescales)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of strings used to parameterize
    the system of stochastic differential equations, configured in a
    mixmassfracbeta ... end block. Within the mixmassfracbeta ... end block the
    coefficients policy must be set to 'hydrotimescale' in order for the
    hydrotimescales ... end  block to be in effect. The 'hydrotimescales'
    keyword is then used to specify a list of strings, each specifying which
    inverse time scale should be used for the particular component integrated.
    Available time scales are defined in src/DiffEq/HydroTimescales.hpp.
    Example: "hydrotimescales eq_A05S eq_A05H eq_A05L eq_A05S eq_A05S end",
    which configures five inverse hydrodynamics time scales associated to 5
    components, i.e., 5 scalar stochastic differential equations, integrated,
    specified and configured within the given mixmassfracbeta ... end block. The
    length of the hydrotimescales vector depends on the number of scalar
    components and is controlled by the preceding keyword 'ncomp'. For
    mixmassfracbeta, ncomp is the actual number of scalar components * 4, since
    mixmassfractionbeta always computes 4 additional derived stochastic
    variables (in a diagnostic) fashion. See also MixMassFractionBeta::derived()
    in src/DiffEq/Beta/MixMassFractionBeta.hpp. Keywords allowed in a
    hydrotimescales ... end block: )" + std::string("\'")
    + eq_A005H::string() + "\', \'"
    + eq_A005S::string() + "\', \'"
    + eq_A005L::string() + "\', \'"
    + eq_A05H::string() + "\', \'"
    + eq_A05S::string() + "\', \'"
    + eq_A05L::string() + "\', \'"
    + eq_A075H::string() + "\', \'"
    + eq_A075S::string() + "\', \'"
    + eq_A075L::string() + "\'. "
    + R"(For an example hydrotimescales ... end block, see
      doc/html/walker_example_mixmassfracbeta.html.)"; }
  struct expect {
    using type = std::string;
    static std::string description() { return "string(s)"; }
  };
};
using hydrotimescales =
  keyword< hydrotimescales_info, TAOCPP_PEGTL_STRING("hydrotimescales") >;

struct prod_A005H_info {
  static std::string name() { return "prod_A005H"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.05, IC:light<<heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.05, IC: light << heavy.)"; }
};
using prod_A005H = keyword< prod_A005H_info, TAOCPP_PEGTL_STRING("prod_A005H") >;

struct prod_A005S_info {
  static std::string name() { return "prod_A005S"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.05, IC:light=heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.05, IC: light = heavy.)"; }
};
using prod_A005S = keyword< prod_A005S_info, TAOCPP_PEGTL_STRING("prod_A005S") >;

struct prod_A005L_info {
  static std::string name() { return "prod_A005L"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.05, IC:light>>heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.05, IC: light >> heavy.)"; }
};
using prod_A005L = keyword< prod_A005L_info, TAOCPP_PEGTL_STRING("prod_A005L") >;

struct prod_A05H_info {
  static std::string name() { return "prod_A05H"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.5, IC:light<<heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.5, IC: light << heavy.)"; }
};
using prod_A05H = keyword< prod_A05H_info, TAOCPP_PEGTL_STRING("prod_A05H") >;

struct prod_A05S_info {
  static std::string name() { return "prod_A05S"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.5, IC:light=heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.5, IC: light = heavy.)"; }
};
using prod_A05S = keyword< prod_A05S_info, TAOCPP_PEGTL_STRING("prod_A05S") >;

struct prod_A05L_info {
  static std::string name() { return "prod_A05L"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.5, IC:light>>heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.5, IC: light >> heavy.)"; }
};
using prod_A05L = keyword< prod_A05L_info, TAOCPP_PEGTL_STRING("prod_A05L") >;

struct prod_A075H_info {
  static std::string name() { return "prod_A075H"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.75, IC:light<<heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.75, IC: light << heavy.)"; }
};
using prod_A075H = keyword< prod_A075H_info, TAOCPP_PEGTL_STRING("prod_A075H") >;

struct prod_A075S_info {
  static std::string name() { return "prod_A075S"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.75, IC:light=heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.75, IC: light = heavy.)"; }
};
using prod_A075S = keyword< prod_A075S_info, TAOCPP_PEGTL_STRING("prod_A075S") >;

struct prod_A075L_info {
  static std::string name() { return "prod_A075L"; }
  static std::string shortDescription() { return "Select production divided by "
    "dissipation rate from DNS of HRT, A=0.75, IC:light>>heavy"; }
  static std::string longDescription() { return
    R"(Production divided by dissipation rate from DNS of homogeneous
       Rayleigh-Taylor instability, P/e, A = 0.75, IC: light >> heavy.)"; }
};
using prod_A075L = keyword< prod_A075L_info, TAOCPP_PEGTL_STRING("prod_A075L") >;

struct hydroproductions_info {
  static std::string name() { return "P/eps"; }
  static std::string shortDescription() { return
    R"(Set MixMassFractionBeta SDE parameter(s) productions)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of strings used to parameterize
    the system of stochastic differential equations, configured in a
    mixmassfracbeta ... end block. Within the mixmassfracbeta ... end block the
    coefficients policy must be set to 'hydrotimescale' in order for the
    hydroproductions ... end  block to be in effect. The 'hydroproductions'
    keyword is then used to specify a list of strings, each specifying which
    turbulent kinetic energy production dividied by the dissipation rate (P/eps)
    data (from direct numerical simulations) should be used for the particular
    component integrated. Available P/eps data are defined in
    src/DiffEq/HydroProductions.hpp. Example: "productions prod_A05S prod_A05H
    prod_A05L prod_A05S prod_A05S end", which
    configures five P/eps data sets associated to 5 components, i.e., 5 scalar
    stochastic differential equations, integrated, specified and configured
    within the given mixmassfracbeta ... end block. The length of the
    hydroproductions vector depends on the number of scalar components and is
    controlled by the preceding keyword 'ncomp'. For mixmassfracbeta, ncomp is
    the actual number of scalar components * 4, since mixmassfractionbeta always
    computes 4 additional derived stochastic variables (in a diagnostic)
    fashion. See also MixMassFractionBeta::derived() in
    src/DiffEq/MixMassFractionBeta.hpp. Keywords allowed in a hydroproductions
    ... end block: )" + std::string("\'")
    + prod_A005H::string() + "\', \'"
    + prod_A005S::string() + "\', \'"
    + prod_A005L::string() + "\', \'"
    + prod_A05H::string() + "\', \'"
    + prod_A05S::string() + "\', \'"
    + prod_A05L::string() + "\', \'"
    + prod_A075H::string() + "\', \'"
    + prod_A075S::string() + "\', \'"
    + prod_A075L::string() + "\'. "
    + R"(For an example hydroproductions ... end block, see
      doc/html/walker_example_mixmassfracbeta.html.)"; }
  struct expect {
    using type = std::string;
    static std::string description() { return "string(s)"; }
  };
};
using hydroproductions =
  keyword< hydroproductions_info, TAOCPP_PEGTL_STRING("hydroproductions") >;

struct mixmassfracbeta_info {
  static std::string name() { return "Mix mass-fraction beta"; }
  static std::string shortDescription() { return
    "Introduce the mixmassfracbeta SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used in multiple ways: (1) To introduce a mixmassfracbeta
    ... end block, used to specify the configuration of a system of mix
    mass-fraction beta SDEs, a system of stochastic differential equations
    (SDEs), whose solution is the joint beta distribution and in which the usual
    beta SDE parameters b and kappa are specified via functions that constrain
    the beta SDE to be consistent with the turbulent mixing process. The mix
    mass-fraction beta SDE is similar to the mass-fraction beta SDE, only the
    process is made consistent with the no-mix and fully mixed limits via the
    specification of the SDE coefficients b and kappa. As in the mass-fraction
    beta SDE, Y is governed by the beta SDE and two additional stochastic
    variables are computed. However, in the mix mass-fraction beta SDE the
    parameters b and kappa are given by b = Theta * b' and kappa = kappa' *
    <y^2>, where Theta = 1 - <y^2> / [ <Y> ( 1 - <Y> ], the fluctuation about
    the mean, <Y>, is defined as usual: y = Y - <Y>, and b' and kappa' are
    user-specified constants. Similar to the mass-fraction beta SDE, there two
    additional random variables computed besides, Y, and they are rho(Y) and
    V(Y). For more detail on the mass-fraction beta SDE, see the help on keyword
    'massfracbeta'. For more details on the beta SDE, see
    https://doi.org/10.1080/14685248.2010.510843 and src/DiffEq/Beta/Beta.hpp.
    Keywords allowed in a mixmassfracbeta ... end block: )"
    + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_bprime::string() + "\', \'"
    + sde_S::string() + "\', \'"
    + sde_kappaprime::string() + "\', \'"
    + sde_rho2::string() + "\', \'"
    + hydrotimescales::string() + "\', \'"
    + hydroproductions::string() + "\', \'"
    + "velocity" + "\', \'"
    + "dissipation" + "\', \'"
    + sde_r::string() + "\'. "
    + R"(For an example mixmassfracbeta ... end block, see
    doc/html/walker_example_mixmassfracbeta.html. (2) To specify a dependent
    variable (by a character) used to couple a differential equation system, in
    which the 'mixmassfracbeta' keyword appears) to another labeled by a
    'depvar'.)";
  }
};
using mixmassfracbeta =
  keyword< mixmassfracbeta_info, TAOCPP_PEGTL_STRING("mixmassfracbeta") >;

struct fullvar_info {
  static std::string name() { return "full variable"; }
  static std::string shortDescription() { return
    "Select full variable (as the dependent variable) to solve for"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the full random (instantaneous) variable
    as what quantity to solve for, i.e., use as the dependent variable, in,
    e.g., a position or velocity model for a stochastic particle. This
    configures how statistics must be interpreted.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using fullvar = keyword< fullvar_info, TAOCPP_PEGTL_STRING("fullvar") >;

struct fluctuation_info {
  static std::string name() { return "fluctuation"; }
  static std::string shortDescription() { return
    "Select fluctuation (as the dependent variable) to solve for"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the fluctuation of a random variable as
    what quantity to solve for, i.e., use as the dependent variable, e.g., in a
    position or velocity model for a stochastic particle. This configures how
    statistics must be interpreted.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using fluctuation =
  keyword< fluctuation_info, TAOCPP_PEGTL_STRING("fluctuation") >;

struct product_info {
  static std::string name() { return "product"; }
  static std::string shortDescription() { return
    "Select product (as the dependent variable) to solve for"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the product of multiple random variables
    as what quantity to solve for, i.e., use as the dependent variable, e.g., in
    a velocity model, solve for the product of the full density and the full
    velocity, i.e., the full momentum, for a stochastic particle. This
    configures how statistics must be interpreted.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using product =
  keyword< product_info, TAOCPP_PEGTL_STRING("product") >;

struct fluctuating_momentum_info {
  static std::string name() { return "fluctuating momentum"; }
  static std::string shortDescription() { return
    "Select fluctuating moment (as the dependent variable) to solve for"; }
  static std::string longDescription() { return
    R"(This keyword is used to select fluctuating moment as the dependent
    variable. This is a very specific quantity and used in conjunction with the
    Langevin equation for the velocity/momentum. The dependent variable is
    phi_i^* = rho^* u_i - <rho^* u_i^*>, where the star superscript means a full
    (i.e., not only a fluctuation about some mean) random variable, u_i is the
    fluctuating velocity, u_i = u^*_i - <u_i^*>, and angle brackets denote the
    ensemble average. This also configures how statistics must be
    interpreted.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using fluctuating_momentum = keyword< fluctuating_momentum_info,
                               TAOCPP_PEGTL_STRING("fluctuating_momentum") >;

struct solve_info {
  static std::string name() { return "solve for"; }
  static std::string shortDescription() { return
    "Select dependent variable to solve for"; }
  static std::string longDescription() { return
    R"(This keyword is used to select an the quantity (the dependent variable)
    to solve for in walker's position and/or velocity model. This configures how
    statistics must be interpreted.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + fullvar::string() + "\' | \'"
                  + fluctuation::string() + "\' | \'"
                  + product::string() + "\' | \'"
                  + fluctuating_momentum::string() + '\'';
    }
  };
};
using solve = keyword< solve_info, TAOCPP_PEGTL_STRING("solve") >;

struct slm_info {
  static std::string name() { return "slm"; }
  static std::string shortDescription() { return
    "Select the simplified Langevin model (SLM) for the velocity PDF model"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the simplified Langevin model (SLM) for
    the Lagrangian velocity in turbulent flows.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using slm = keyword< slm_info, TAOCPP_PEGTL_STRING("slm") >;

struct glm_info {
  static std::string name() { return "glm"; }
  static std::string shortDescription() { return
    "Select the generalized Langevin model for the velocity PDF model"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the generalized Langevin model for the
    Lagrangian velocity in turbulent flows.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using glm = keyword< glm_info, TAOCPP_PEGTL_STRING("glm") >;

struct variant_info {
  static std::string name() { return "variant"; }
  static std::string shortDescription() { return
    "Select velocity PDF model variant"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the velocity PDF model variant.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + slm::string() + "\' | \'"
                  + glm::string() + '\'';
    }
  };
};
using variant = keyword< variant_info, TAOCPP_PEGTL_STRING("variant") >;

struct light_info {
  static std::string name() { return "light"; }
  static std::string shortDescription() { return
    "Select the light-fluid normalization for the mixture Dirichlet SDE"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the light-fluid normalization for the
    mixture Dirichlet PDF/SDE model for multi-material mixing in turbulent
    flows.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using light = keyword< light_info, TAOCPP_PEGTL_STRING("light") >;

struct heavy_info {
  static std::string name() { return "heavy"; }
  static std::string shortDescription() { return
    "Select the heavy-fluid normalization for the mixture Dirichlet SDE"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the heavy-fluid normalization for the
    mixture Dirichlet PDF/SDE model for multi-material mixing in turbulent
    flows.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using heavy = keyword< heavy_info, TAOCPP_PEGTL_STRING("heavy") >;

struct normalization_info {
  static std::string name() { return "normalization"; }
  static std::string shortDescription() { return
    "Select mixture Dirichlet PDF model normalization type"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the mixture Dirichlet PDF model
    normalization type.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + light::string() + "\' | \'"
                  + heavy::string() + '\'';
    }
  };
};
using normalization =
  keyword< normalization_info, TAOCPP_PEGTL_STRING("normalization") >;

struct position_info {
  static std::string name() { return "position"; }
  static std::string shortDescription() { return
    "Introduce the (particle) position equation input block or coupling"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a position ...
    end block, used to specify the configuration of a system of deterministic or
    stochastic differential equations, governing particle positions usually in
    conjunction with velocity model, e.g, the Langevin, model. Note that the
    random number generator r123_philox is automatically put on the list as a
    selected RNG if no RNG is selected. Keywords allowed in a position ... end
    block: )" +
    std::string("\'")
    + depvar::string()+ "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + "velocity" + "\', \'"
    + R"(For an example position ... end block, see
    doc/html/walker_example_position.html. (2) To specify a dependent
    variable (by a character) used to couple a differential equation system, in
    which the 'position' keyword appears) to another labeled by a 'depvar'.)";
  }
};
using position = keyword< position_info, TAOCPP_PEGTL_STRING("position") >;

struct dissipation_info {
  static std::string name() { return "dissipation"; }
  static std::string shortDescription() { return
    "Introduce the (particle) dissipation equation input block or coupling"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a dissipation
    ... end block, used to specify the configuration of a system of
    deterministic or stochastic differential equations, governing a particle
    quantity that models the dissipation rate of turbulent kinetic energy, used
    to coupled to particle velocity model, e.g, the Langevin, model. Note that
    the random number generator r123_philox is automatically put on the list as
    a selected RNG if no RNG is selected. Keywords allowed in a dissipation ...
    end block: )" +
    std::string("\'")
    + depvar::string()+ "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + "velocity" + "\', \'"
    + R"(For an example dissipation ... end block, see
    doc/html/walker_example_dissipation.html. (2) To specify a dependent
    variable (by a character) used to couple a differential equation system, in
    which the 'dissipation' keyword appears) to another labeled by a 'depvar'.)";
  }
};
using dissipation =
  keyword< dissipation_info, TAOCPP_PEGTL_STRING("dissipation") >;

struct velocitysde_info {
  static std::string name() { return "velocity"; }
  static std::string shortDescription() { return
    "Introduce the velocity equation input block or coupling"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a velocity ...
    end block, used to specify the configuration of a system of stochastic
    differential equations (SDEs), governed by the Langevin model for the
    fluctuating velocity in homogeneous variable-density turbulence. For more
    details on this Langevin model, see
    https://doi.org/10.1080/14685248.2011.554419 and
    src/DiffEq/Velocity/Velocity.hpp. Keywords allowed in a velocity ... end
    block: )" +
    std::string("\'")
    + depvar::string()+ "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + hydrotimescales::string() + "\', \'"
    + hydroproductions::string() + "\', \'"
    + sde_c0::string() + "\'. "
    + position::string() + "\', \'"
    + dissipation::string() + "\', \'"
    + mixmassfracbeta::string() + "\', \'"
    + R"(For an example velocity ... end block, see
    doc/html/walker_example_velocity.html. (2) To specify a dependent
    variable (by a character) used to couple a differential equation system, in
    which the 'velocity' keyword appears) to another labeled by a 'depvar'.)";
  }
};
using velocitysde = keyword< velocity_info, TAOCPP_PEGTL_STRING("velocity") >;

struct gamma_info {
  static std::string name() { return "Gamma"; }
  static std::string shortDescription() { return
    "Introduce the gamma SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the gamma ... end block, used to
    specify the configuration of a system of stochastic differential equations
    (SDEs), with linear drift and linear diagonal diffusion, whose invariant
    is the joint gamma distribution.)";
  }
};
using gamma = keyword< gamma_info, TAOCPP_PEGTL_STRING("gamma") >;

struct ornstein_uhlenbeck_info {
  static std::string name() { return "Ornstein-Uhlenbeck"; }
  static std::string shortDescription() { return
    "Introduce the Ornstein-Uhlenbeck SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an ornstein-uhlenbeck ... end block,
    used to specify the configuration of a system of stochastic differential
    equations (SDEs), with linear drift and constant diffusion, whose
    invariant is the joint normal distribution. Keywords allowed in an
    ornstein-uhlenbeck ... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_sigmasq::string() + "\', \'"
    + sde_theta::string() + "\', \'"
    + sde_mu::string() + "\'. "
    + R"(For an example ornstein-uhlenbeck ... end block, see
      doc/html/walker_example_ou.html.)";
  }
};
using ornstein_uhlenbeck =
  keyword< ornstein_uhlenbeck_info, TAOCPP_PEGTL_STRING("ornstein-uhlenbeck") >;

struct diag_ou_info {
  static std::string name() { return "Diagonal Ornstein-Uhlenbeck"; }
  static std::string shortDescription() { return
    "Introduce the diagonal Ornstein-Uhlenbeck SDE input block"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a diag_ou ... end
    block, where 'diag_ou' stands for diagonal Ornstein-Uhlenbeck' and is used
    to specify the configuration of a system of stochastic differential
    equations (SDEs), with linear drift and constant diagonal diffusion, whose
    invariant is the joint normal distribution. Keywords
    allowed in a diagou ... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + ncomp::string() + "\', \'"
    + rng::string() + "\', \'"
    + init::string() + "\', \'"
    + coeff::string() + "\', \'"
    + sde_sigmasq::string() + "\', \'"
    + sde_theta::string() + "\', \'"
    + sde_mu::string() + "\'. "
    + R"(For an example diagou ... end block, see
      doc/html/walker_example_diagou.html.)";
  }
};
using diag_ou = keyword< diag_ou_info, TAOCPP_PEGTL_STRING("diag_ou") >;

struct control_info {
  static std::string name() { return "control"; }
  static std::string shortDescription()
  { return "Specify the control file name [REQUIRED]"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the name of the control file from which
    detailed user input is parsed.)";
  }
  using alias = Alias< c >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using control = keyword< control_info, TAOCPP_PEGTL_STRING("control") >;

struct smallcrush_info {
  static std::string name() { return "SmallCrush"; }
  static std::string shortDescription() {
    return "Select RNG battery SmallCrush"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the description of the random number
    generator test suite, i.e., battery, 'SmallCrush'. SmallCrush is a
    battery of relatively small number, O(10), of tests, defined in TestU01,
    a library for the empirical testing of random number generators. For more "
    info, see http://www.iro.umontreal.ca/~simardr/testu01/tu01.html.)";
  }
};
using smallcrush = keyword< smallcrush_info, TAOCPP_PEGTL_STRING("smallcrush") >;

struct crush_info {
  static std::string name() { return "Crush"; }
  static std::string shortDescription() { return
    "Select RNG battery Crush"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the description of the random number
    generator test suite, i.e., battery, 'Crush'. Crush is a suite of
    stringent statistical tests, O(100), defined in TestU01, a library for
    the empirical testing of random number generators. For more info, see
    http://www.iro.umontreal.ca/~simardr/testu01/tu01.html.)";
  }
};
using crush = keyword< crush_info, TAOCPP_PEGTL_STRING("crush") >;

struct bigcrush_info {
  static std::string name() { return "BigCrush"; }
  static std::string shortDescription() { return
    "Select RNG battery BigCrush"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the description of the random number
    generator test suite, i.e., battery, 'BigCrush'. BigCrush is a
    suite of very stringent statistical tests, O(100), defined in TestU01, a
    library for the empirical testing of random number generators. For more
    info, see http://www.iro.umontreal.ca/~simardr/testu01/tu01.html.)";
  }
};
using bigcrush = keyword< bigcrush_info, TAOCPP_PEGTL_STRING("bigcrush") >;

struct verbose_info {
  static std::string name() { return "verbose"; }
  static std::string shortDescription() { return
    "Select verbose screen output"; }
  static std::string longDescription() { return
    R"(This keyword is used to select verbose screen-output as opposed to the
    default quiet output. With quiet output only the most important messages
    are echoed to screen.)";
  }
  using alias = Alias< v >;
};
using verbose = keyword< verbose_info, TAOCPP_PEGTL_STRING("verbose") >;

struct charestate_info {
  static std::string name() { return "charestate"; }
  static std::string shortDescription() { return
    "Enable verbose chare state screen output"; }
  static std::string longDescription() { return
    R"(This keyword is used to enable verbose Charm++ chare state collection and
    screen output. The chare state is displayed after a run is finished and the
    data collected is grouped by chare id (thisIndex), and within groups data
    is ordered by the time-stamp when a given chare member function is
    called. See src/Base/ChareState.hpp for details on what is collected. Note
    that to collect chare state, the given chare must be instrumented. Note that
    if quescence detection is enabled,
    chare state collection is also automatically enabled, but the chare state is
    only output if quiescence is detected (which also triggers an error).)";
  }
  using alias = Alias< S >;
};
using charestate = keyword< charestate_info, TAOCPP_PEGTL_STRING("state") >;

struct benchmark_info {
  static std::string name() { return "benchmark"; }
  static std::string shortDescription() { return "Select benchmark mode"; }
  static std::string longDescription() { return
    R"(This keyword is used to select benchmark mode. In benchmark mode no large
       file output is performed, overriding the configuration in the control
       file.)";
  }
  using alias = Alias< b >;
};

using benchmark = keyword< benchmark_info, TAOCPP_PEGTL_STRING("benchmark") >;

struct nonblocking_info {
  static std::string name() { return "nonblocking"; }
  static std::string shortDescription()
  { return "Select non-blocking migration"; }
  static std::string longDescription() { return
    R"(This keyword is used to select non-blocking, instead of the default
       blocking, migration. WARNING: This feature is experimental, not well
       tested, and may not always work as expected.)";
  }
  using alias = Alias< n >;
};

using nonblocking =
  keyword< nonblocking_info, TAOCPP_PEGTL_STRING("nonblocking") >;

struct lbfreq_info {
  static std::string name() { return "Load balancing frequency"; }
  static std::string shortDescription()
  { return "Set load-balancing frequency during time stepping"; }
  static std::string longDescription() { return
    R"(This keyword is used to set the frequency of load-balancing during
       time stepping. The default is 1, which means that load balancing is
       initiated every time step. Note, however, that this does not necessarily
       mean that load balancing will be performed by the runtime system every
       time step, only that the Charm++ load-balancer is initiated. For more
       information, see the Charm++ manual.)";
  }
  using alias = Alias< l >;
  struct expect {
    using type = std::size_t;
    static constexpr type lower = 1;
    static constexpr type upper = std::numeric_limits< type >::max()-1;
    static std::string description() { return "int"; }
    static std::string choices() {
      return "integer between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using lbfreq = keyword< lbfreq_info, TAOCPP_PEGTL_STRING("lbfreq") >;

struct rsfreq_info {
  static std::string name() { return "Checkpoint/restart frequency"; }
  static std::string shortDescription()
  { return "Set checkpoint/restart frequency during time stepping"; }
  static std::string longDescription() { return
    R"(This keyword is used to set the frequency of dumping checkpoint/restart
       files during time stepping. The default is 1000, which means that
       checkpoint/restart files are dumped at every 1000th time step.)";
  }
  using alias = Alias< r >;
  struct expect {
    using type = std::size_t;
    static constexpr type lower = 1;
    static constexpr type upper = std::numeric_limits< type >::max()-1;
    static std::string description() { return "int"; }
    static std::string choices() {
      return "integer between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using rsfreq = keyword< rsfreq_info, TAOCPP_PEGTL_STRING("rsfreq") >;

struct feedback_info {
  static std::string name() { return "feedback"; }
  static std::string shortDescription() { return "Enable on-screen feedback"; }
  static std::string longDescription() { return
    R"(This keyword is used to enable more detailed on-screen feedback on
       particular tasks and sub-tasks as they happen. This is useful for large
       problems and debugging.)";
  }
  using alias = Alias< f >;
};
using feedback = keyword< feedback_info, TAOCPP_PEGTL_STRING("feedback") >;

struct version_info {
  static std::string name() { return "Show version"; }
  static std::string shortDescription() { return "Show version information"; }
  static std::string longDescription() { return
    R"(This keyword is used to display version information for the
       executable/tool on the standard output and exit successfully.)";
  }
  using alias = Alias< V >;
};
using version = keyword< version_info, TAOCPP_PEGTL_STRING("version") >;

struct license_info {
  static std::string name() { return "Show license"; }
  static std::string shortDescription() { return "Show license information"; }
  static std::string longDescription() { return
    R"(This keyword is used to display license information for the
       executable/tool on the standard output and exit successfully.)";
  }
  using alias = Alias< L >;
};
using license = keyword< license_info, TAOCPP_PEGTL_STRING("license") >;

struct trace_info {
  static std::string name() { return "trace"; }
  static std::string shortDescription()
  { return "Disable call and stack trace"; }
  static std::string longDescription() { return R"(This keyword can be used to
    disable the on-screen call trace and stack trace after an exception is
    thrown. Trace output is on by default and in some cases, the call and
    stack trace can be huge and not very helpful, hence this command line
    option.)"; }
  using alias = Alias< t >;
};
using trace = keyword< trace_info, TAOCPP_PEGTL_STRING("trace") >;

struct quiescence_info {
  static std::string name() { return "quiescence"; }
  static std::string shortDescription()
  { return "Enable quiescence detection"; }
  static std::string longDescription() { return
    R"(This keyword is used to enable the quiescence detection feature of
       Charm++, used to catch logic errors in the asynchronous control flow,
       resulting in deadlocks. This is useful for automated testing and
       debugging and does have some overhead, so it is off by default.)";
  }
  using alias = Alias< q >;
};
using quiescence =
  keyword< quiescence_info, TAOCPP_PEGTL_STRING("quiescence") >;

struct virtualization_info {
  static std::string name() { return "virtualization"; }
  static std::string shortDescription() { return
    R"(Set degree of virtualization)"; }
  static std::string longDescription() { return
    R"(This option is used to set the degree of virtualization
    (over-decomposition). The virtualization parameter is a real number
    between 0.0 and 1.0, inclusive, which controls the degree of
    virtualization or over-decomposition. Independent of the value of
    virtualization the work is approximately evenly distributed among the
    available processing elements. For zero virtualization (no
    over-decomposition), the work is simply decomposed into
    total_work/numPEs, which yields the smallest number of Charm++ chares and
    the largest chunks of work units. The other extreme is unity
    virtualization, which decomposes the total work into the smallest size
    work units possible, yielding the largest number of Charm++ chares.
    Obviously, the optimum will be between 0.0 and 1.0, depending on the
    problem.)";
  }
  using alias = Alias< u >;
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static constexpr type upper = 1.0;
    static std::string description() { return "real"; }
    static std::string choices() {
      return "real between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using virtualization =
  keyword< virtualization_info, TAOCPP_PEGTL_STRING("virtualization") >;

struct pdf_info {
  static std::string name() { return "pdf"; }
  static std::string shortDescription() { return
    "Specify the name of the PDF output file"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the name of the output file in which to
    store probability density functions (PDFs) during a simulation.)";
  }
  using alias = Alias< p >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using pdf = keyword< pdf_info, TAOCPP_PEGTL_STRING("pdf") >;

struct stat_info {
  static std::string name() { return "stat"; }
  static std::string shortDescription() { return
    "Specify the name of the statistical moments output file"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the name of the output file in which to
    store statistical moments during a simulation.)";
  }
  using alias = Alias< s >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using stat = keyword< stat_info, TAOCPP_PEGTL_STRING("stat") >;

struct particles_info {
  static std::string name() { return "particles"; }
  static std::string shortDescription() { return
    "Specify the name of the particles position output file"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the name of the output file in which to
    store particles positions during a simulation.)";
  }
  using alias = Alias< x >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using particles = keyword< particles_info, TAOCPP_PEGTL_STRING("particles") >;

struct input_info {
  static std::string name() { return "input"; }
  static std::string shortDescription() { return "Specify the input file"; }
  static std::string longDescription() { return
    R"(This option is used to define the name of input file.)";
  }
  using alias = Alias< i >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using input = keyword< input_info, TAOCPP_PEGTL_STRING("input") >;

struct output_info {
  static std::string name() { return "output"; }
  static std::string shortDescription() { return "Specify the output file"; }
  static std::string longDescription() { return
    R"(This option is used to define the output file name. In MeshConv, this is
    used to specify the output mesh file name. In Inciter this is used to
    specify the output base filename. The base filename is appended by
    ".e-s.<meshid>.<numchares>.<chareid>", where 'e-s' probably stands for
    ExodusII sequence (the output file format), <meshid> counts the number of
    new meshes (this is incremented whenever the mesh is new compared to the
    previous iteration, due to, e.g., mesh refinement), <numchares> is the total
    number of mesh partitions, and <chareid> is the work unit (or mesh
    partition) id.)";
  }
  using alias = Alias< o >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using output = keyword< output_info, TAOCPP_PEGTL_STRING("output") >;

struct refined_info {
  static std::string name() { return "Refined field output"; }
  static std::string shortDescription() { return
    "Turn refined field output on/off"; }
  static std::string longDescription() { return
    R"(This keyword can be used to turn on/off refined field output, which
    refines the mesh and evaluates the solution on the refined mesh for saving
    the solution.)"; }
  struct expect {
    using type = bool;
    static std::string description() { return "string"; }
    static std::string choices() { return "true | false"; }
  };
};
using refined =keyword< refined_info, TAOCPP_PEGTL_STRING("refined") >;

struct screen_info {
  static std::string name() { return "screen"; }
  static std::string shortDescription() {
    return "Specify the screen output file"; }
  static std::string longDescription() { return
    R"(This option is used to set the screen output file name. The default is
    "<executable>_screen.log".)";
  }
  using alias = Alias< O >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using screen = keyword< screen_info, TAOCPP_PEGTL_STRING("screen") >;

struct restart_info {
  static std::string name() { return "checkpoint/restart directory name"; }
  static std::string shortDescription()
    { return "Specify the directory for restart files"; }
  static std::string longDescription() { return
    R"(This option is used to specify the directory name in which to save
    checkpoint/restart files.)";
  }
  using alias = Alias< R >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using restart = keyword< restart_info, TAOCPP_PEGTL_STRING("restart") >;

struct l2_info {
  static std::string name() { return "L2"; }
  static std::string shortDescription() { return "Select the L2 norm"; }
  static std::string longDescription() { return
    R"(This keyword is used to enable computing the L2 norm. Example:
    "diagnostics error l2 end'.")"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using l2 = keyword< l2_info, TAOCPP_PEGTL_STRING("l2") >;

struct linf_info {
  static std::string name() { return "Linf"; }
  static std::string shortDescription() { return
    "Select the L_{infinity} norm"; }
  static std::string longDescription() { return
    R"(This keyword is used to enable computing the L-infinity norm. Example:
    "diagnostics error linf end'.")"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using linf = keyword< linf_info, TAOCPP_PEGTL_STRING("linf") >;

struct error_info {
  static std::string name() { return "error"; }
  static std::string shortDescription() { return "Select an error norm"; }
  static std::string longDescription() { return
    R"(This keyword is used to select, i.e., turn on, the estimation of an
    error norm. The keyword is used in a 'diagnostics ... end' block. Example:
    "diagnostics error l2 end", which configures computation of the L2 norm.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + l2::string() + "\' | \'"
                  + linf::string() + '\'';
    }
  };
};
using error = keyword< error_info, TAOCPP_PEGTL_STRING("error") >;

struct diagnostics_cmd_info {
  static std::string name() { return "diagnostics"; }
  static std::string shortDescription()
  { return "Specify the diagnostics file name"; }
  static std::string longDescription() { return
    R"(This option is used to define the diagnostics file name.)";
  }
  using alias = Alias< d >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using diagnostics_cmd =
  keyword< diagnostics_cmd_info, TAOCPP_PEGTL_STRING("diagnostics") >;

struct diagnostics_info {
  static std::string name() { return "diagnostics"; }
  static std::string shortDescription()
  { return "Specify the diagnostics file name"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the dagnostics ... end block, used to
    configure diagnostics output. Keywords allowed in this block: )"
    + std::string("\'")
    + interval::string() + "\' | \'"
    + txt_float_format::string() + "\' | \'"
    + error::string() + "\' | \'"
    + precision::string() + "\'.";
  }
};
using diagnostics =
  keyword< diagnostics_info, TAOCPP_PEGTL_STRING("diagnostics") >;

struct reorder_cmd_info {
  static std::string name() { return "reorder"; }
  static std::string shortDescription() { return "Reorder mesh nodes"; }
  static std::string longDescription() { return
    R"(This keyword is used as a command line argument to instruct the mesh
    converter to not only convert but also reorder the mesh nodes using the
    advancing front technique. Reordering is optional in meshconv and
    inciter.)";
  }
  using alias = Alias< r >;
  struct expect {
    using type = bool;
    static std::string description() { return "string"; }
  };
};
using reorder_cmd = keyword< reorder_cmd_info, TAOCPP_PEGTL_STRING("reorder") >;

struct pelocal_reorder_info {
  static std::string name() { return "PE-local reorder"; }
  static std::string shortDescription() { return "PE-local reorder"; }
  static std::string longDescription() { return
    R"(This keyword is used in inciter as a keyword in the inciter...end block
    as "pelocal_reorder true" (or false) to do (or not do) a global distributed
    mesh reordering across all PEs that yields an approximately continous mesh
    node ID order as mesh partitions are assigned to PEs after mesh
    partitioning. This reordering is optional.)";
  }
  struct expect {
    using type = bool;
    static std::string choices() { return "true | false"; }
    static std::string description() { return "string"; }
  };
};
using pelocal_reorder =
  keyword< pelocal_reorder_info, TAOCPP_PEGTL_STRING("pelocal_reorder") >;

struct operator_reorder_info {
  static std::string name() { return "operator_reorder"; }
  static std::string shortDescription() { return "Operator-access reorder"; }
  static std::string longDescription() { return
    R"(This keyword is used in inciter as a keyword in the inciter...end block
    as "operator_reorder on" (or off) to do (or not do) a local mesh node
    reordering based on the PDE operator access pattern. This reordering is
    optional.)";
  }
  struct expect {
    using type = bool;
    static std::string choices() { return "true | false"; }
    static std::string description() { return "string"; }
  };
};
using operator_reorder =
  keyword< operator_reorder_info, TAOCPP_PEGTL_STRING("operator_reorder") >;

struct steady_state_info {
  static std::string name() { return "steady_state"; }
  static std::string shortDescription() { return "March to steady state"; }
  static std::string longDescription() { return
    R"(This keyword is used indicate that local time stepping should be used
       towards a stationary solution.)"; }
  struct expect {
    using type = bool;
    static std::string choices() { return "true | false"; }
    static std::string description() { return "string"; }
  };
};
using steady_state =
  keyword< steady_state_info, TAOCPP_PEGTL_STRING("steady_state") >;

struct residual_info {
  static std::string name() { return "residual"; }
  static std::string shortDescription() { return
    "Set the convergence criterion for the residual to reach"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a convergence criterion for, e.g., local
    time stepping marching to steady state, below which the simulation is
    considered converged.)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 1.0e-14;
    static std::string description() { return "real"; }
  };
};
using residual = keyword< residual_info, TAOCPP_PEGTL_STRING("residual") >;

struct rescomp_info {
  static std::string name() { return "rescomp"; }
  static std::string shortDescription() { return
    "Equation system component index for convergence"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a single integer that is used to denote
    the equation component index in the complete system of equation systems
    configured in an input file to use for the convergence criterion for local
    time stepping marching towards steady state.)";
  }
  struct expect {
    using type = uint32_t;
    static constexpr type lower = 1;
    static std::string description() { return "uint"; }
  };
};
using rescomp = keyword< rescomp_info, TAOCPP_PEGTL_STRING("rescomp") >;

struct group_info {
  static std::string name() { return "group"; }
  static std::string shortDescription() { return
    "Select test group(s) to run"; }
  static std::string longDescription() { return
    R"(This option can be used to select one or more test groups to run by
    specifying the full or a partial name of a test group. All tests of a
    selected group will be executed. If this option is not given, all test
    groups are executed by default. Examples: '--group make_list' - run only
    the 'make_list' test group, '--group Parser' - run the test groups that have
    the string 'Parser' in their name, e.g., groups 'Control/FileParser' and
    'Control/StringParser'.)";
  }
  using alias = Alias< g >;
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using group = keyword< group_info, TAOCPP_PEGTL_STRING("group") >;

struct inciter_info {
  static std::string name() { return "inciter"; }
  static std::string shortDescription() { return
    "Start configuration block for inciter"; }
  static std::string longDescription() { return
    R"(This keyword is used to select inciter. Inciter, is a continuum-realm
    shock hydrodynamics tool, solving a PDE.)";
  }
};
using inciter = keyword< inciter_info, TAOCPP_PEGTL_STRING("inciter") >;

struct user_defined_info {
  using code = Code< U >;
  static std::string name() { return "User-defined"; }
  static std::string shortDescription() { return
    "Select user-defined specification for a problem"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the user-define specification for a
    problem to be solved by a partial differential equation. The initial and
    boundary conditions are expected to be specified elsewhere in the input file
    to set up the problem. Example: "problem user_defined". This the default
    problem type.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};

using user_defined = keyword< user_defined_info, TAOCPP_PEGTL_STRING("user_defined") >;

struct shear_diff_info {
  using code = Code< S >;
  static std::string name() { return "Shear-diffusion"; }
  static std::string shortDescription() { return
    "Select the shear + diffusion test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the shear diffusion test problem. The
    initial and boundary conditions are specified to set up the test problem
    suitable to exercise and test the advection and diffusion terms of the
    scalar transport equation. Example: "problem shear_diff".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using shear_diff = keyword< shear_diff_info, TAOCPP_PEGTL_STRING("shear_diff") >;

struct slot_cyl_info {
  using code = Code< Z >;
  static std::string name() { return "Zalesak's slotted cylinder"; }
  static std::string shortDescription() { return
    "Select Zalesak's slotted cylinder test problem"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Zalesak's slotted cylinder test
    problem. The initial and boundary conditions are specified to set up the
    test problem suitable to exercise and test the advection and diffusion
    terms of the scalar transport equation. Example: "problem slot_cyl".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using slot_cyl = keyword< slot_cyl_info, TAOCPP_PEGTL_STRING("slot_cyl") >;

struct gauss_hump_info {
  using code = Code< G >;
  static std::string name() { return "Advection of 2D Gaussian hump"; }
  static std::string shortDescription() { return
    "Select advection of 2D Gaussian hump test problem"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the advection of 2D Gaussian hump test
    problem. The initial and boundary conditions are specified to set up the
    test problem suitable to exercise and test the advection
    terms of the scalar transport equation. Example: "problem gauss_hump".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using gauss_hump = keyword< gauss_hump_info, TAOCPP_PEGTL_STRING("gauss_hump") >;

struct cyl_advect_info {
  using code = Code< C >;
  static std::string name() { return "Advection of cylinder"; }
  static std::string shortDescription() { return
    "Select advection of cylinder test problem"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the advection of cylinder test
    problem. The initial and boundary conditions are specified to set up the
    test problem suitable to exercise and test the advection
    terms of the scalar transport equation. Example: "problem cyl_advect".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using cyl_advect = keyword< cyl_advect_info, TAOCPP_PEGTL_STRING("cyl_advect") >;

struct cyl_vortex_info {
  using code = Code< X >;
  static std::string name() { return "Deformation of cylinder in a vortex"; }
  static std::string shortDescription() { return
    "Select deformation of cylinder in a vortex test problem"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the test problem which deforms a cylinder
    in a vortical velocity field. The initial and boundary conditions are
    specified to set up the test problem suitable to exercise and test the
    advection terms of the scalar transport equation.
    Example: "problem cyl_vortex".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using cyl_vortex = keyword< cyl_vortex_info, TAOCPP_PEGTL_STRING("cyl_vortex") >;

struct vortical_flow_info {
  using code = Code< V >;
  static std::string name() { return "Vortical flow"; }
  static std::string shortDescription() { return
    "Select the vortical flow test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the vortical flow test problem. The
    purpose of this test problem is to test velocity errors generated by spatial
    operators in the presence of 3D vorticity and in particluar the
    superposition of planar and vortical flows, analogous to voritcity
    stretching. Example: "problem vortical_flow. For more details, see Waltz,
    et. al, "Manufactured solutions for the three-dimensional Euler equations
    with relevance to Inertial Confinement Fusion", Journal of Computational
    Physics 267 (2014) 196-209.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using vortical_flow =
  keyword< vortical_flow_info, TAOCPP_PEGTL_STRING("vortical_flow") >;

struct nl_energy_growth_info {
  using code = Code< N >;
  static std::string name() { return "Nonlinear energy growth"; }
  static std::string shortDescription() { return
    "Select the nonlinear energy growth test problem ";}
  static std::string longDescription() { return
    R"(This keyword is used to select the nonlinear energy growth test problem.
    The purpose of this test problem is to test nonlinear, time dependent energy
    growth and the subsequent development of pressure gradients due to coupling
    between the internal energy and the equation of state. Example: "problem
    nl_energy_growth". For more details, see Waltz, et. al, "Manufactured
    solutions for the three-dimensional Euler equations with relevance to
    Inertial Confinement Fusion", Journal of Computational Physics 267 (2014)
    196-209.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using nl_energy_growth =
  keyword< nl_energy_growth_info, TAOCPP_PEGTL_STRING("nl_energy_growth") >;

struct rayleigh_taylor_info {
  using code = Code< R >;
  static std::string name() { return "Rayleigh-Taylor"; }
  static std::string shortDescription() { return
    "Select the Rayleigh-Taylor test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Rayleigh-Taylor unstable configuration
    test problem. The purpose of this test problem is to assess time dependent
    fluid motion in the presence of Rayleigh-Taylor unstable conditions, i.e.
    opposing density and pressure gradients. Example: "problem rayleigh_taylor".
    For more details, see Waltz, et. al, "Manufactured solutions for the
    three-dimensional Euler equations with relevance to Inertial Confinement
    Fusion", Journal of Computational Physics 267 (2014) 196-209.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using rayleigh_taylor =
  keyword< rayleigh_taylor_info, TAOCPP_PEGTL_STRING("rayleigh_taylor") >;

struct taylor_green_info {
  using code = Code< T >;
  static std::string name() { return "Taylor-Green"; }
  static std::string shortDescription() { return
    "Select the Taylor-Green test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Taylor-Green vortex test problem. The
    purpose of this problem is to test time accuracy and the correctness of the
    discretization of the viscous term in the Navier-Stokes equation. Example:
    "problem taylor_green". For more details on the flow, see G.I. Taylor, A.E.
    Green, "Mechanism of the Production of Small Eddies from Large Ones", Proc.
    R. Soc. Lond. A 1937 158 499-521; DOI: 10.1098/rspa.1937.0036. Published 3
    February 1937.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using taylor_green =
  keyword< taylor_green_info, TAOCPP_PEGTL_STRING("taylor_green") >;

struct shedding_flow_info {
  using code = Code< F >;
  static std::string name() { return "Shedding flow over triangular wedge"; }
  static std::string shortDescription() { return
    "Select the Shedding flow test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Shedding flow test problem. It
    describe a quasi-2D inviscid flow over a triangular wedge in tetrahedron
    grid. The purpose of this test problem is to test the capability of DG
    scheme for retaining the shape of vortices and also different error
    indicator behavior for this external flow problem when p-adaptive DG scheme
    is applied. Example: "problem shedding_flow".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using shedding_flow =
  keyword< shedding_flow_info, TAOCPP_PEGTL_STRING("shedding_flow") >;

struct sod_shocktube_info {
  using code = Code< H >;
  static std::string name() { return "Sod shock-tube"; }
  static std::string shortDescription() { return
    "Select the Sod shock-tube test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Sod shock-tube test problem. The
    purpose of this test problem is to test the correctness of the
    approximate Riemann solver and its shock and interface capturing
    capabilities. Example: "problem sod_shocktube". For more details, see
    G. A. Sod, "A Survey of Several Finite Difference Methods for Systems of
    Nonlinear Hyperbolic Conservation Laws", J. Comput. Phys., 27 (1978)
    1–31.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using sod_shocktube =
  keyword< sod_shocktube_info, TAOCPP_PEGTL_STRING("sod_shocktube") >;

struct sod_rotated_shocktube_info {
  using code = Code< O >;
  static std::string name() { return "Rotated Sod shock-tube"; }
  static std::string shortDescription() { return
    "Select the rotated Sod shock-tube test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the rotated Sod shock-tube test problem.
    This the same as Sod shocktube but the geometry is rotated about X, Y, Z
    each by 45 degrees (in that order) so that none of the domain boundary align
    with any of the coordinate directions. The purpose of this test problem is
    to test the correctness of the approximate Riemann solver and its shock and
    interface capturing capabilities in an arbitrarily oriented geometry.
    Example: "problem rotated_sod_shocktube". For more details on the Sod
    problem, see G. A. Sod, "A Survey of Several Finite Difference Methods for
    Systems of Nonlinear Hyperbolic Conservation Laws", J. Comput. Phys., 27
    (1978) 1–31.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using rotated_sod_shocktube =
  keyword< sod_rotated_shocktube_info,
           TAOCPP_PEGTL_STRING("rotated_sod_shocktube") >;

struct sedov_blastwave_info {
  using code = Code< B >;
  static std::string name() { return "Sedov blast-wave"; }
  static std::string shortDescription() { return
    "Select the Sedov blast-wave test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Sedov blast-wave test problem. The
    purpose of this test problem is to test the correctness of the
    approximate Riemann solver and its strong shock and interface capturing
    capabilities. Example: "problem sedov_blastwave".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using sedov_blastwave =
  keyword< sedov_blastwave_info, TAOCPP_PEGTL_STRING("sedov_blastwave") >;

struct interface_advection_info {
  using code = Code< I >;
  static std::string name() { return "Interface advection"; }
  static std::string shortDescription() { return
    "Select the interface advection test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the interface advection test problem. The
    purpose of this test problem is to test the well-balancedness of the
    multi-material discretization and its interface capturing
    capabilities. Example: "problem interface_advection".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using interface_advection =
  keyword< interface_advection_info,
           TAOCPP_PEGTL_STRING("interface_advection") >;

struct gauss_hump_compflow_info {
  using code = Code< A >;
  static std::string name()
  { return "Advection of 2D Gaussian hump for Euler equations"; }
  static std::string shortDescription()
  { return "Select advection of 2D Gaussian hump test problem"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the advection of 2D Gaussian hump test
    problem. The initial and boundary conditions are specified to set up the
    test problem suitable to exercise and test the advection terms of the
    Euler equations. The baseline of the density distribution in this testcase
    is 1 instead of 0 in gauss_hump_transport which enables it to be the
    regression testcase for p-adaptive DG scheme. Example: "problem
    gauss_hump_compflow".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using gauss_hump_compflow = keyword< gauss_hump_compflow_info,
                            TAOCPP_PEGTL_STRING("gauss_hump_compflow") >;

struct waterair_shocktube_info {
  using code = Code< W >;
  static std::string name() { return "Water-air shock-tube"; }
  static std::string shortDescription() { return
    "Select the water-air shock-tube test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Water-air shock-tube test problem. The
    purpose of this test problem is to test the correctness of the
    multi-material pressure relaxation procedure and its interface capturing
    capabilities. Example: "problem waterair_shocktube". For more details, see
    Chiapolino, A., Saurel, R., & Nkonga, B. (2017). Sharpening diffuse
    interfaces with compressible fluids on unstructured meshes. Journal of
    Computational Physics, 340, 389-417.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using waterair_shocktube =
  keyword< waterair_shocktube_info, TAOCPP_PEGTL_STRING("waterair_shocktube") >;

struct triple_point_info {
  using code = Code< K >;
  static std::string name() { return "Triple point problem"; }
  static std::string shortDescription() { return
    "Select the triple point test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the triple point test problem. The
    purpose of this test problem is to test the correctness of the
    multi-material algorithm and its interface capturing
    capabilities. Example: "problem triple_point". For more details, see
    Galera, S., Maire, P. H., & Breil, J. (2010). A two-dimensional unstructured
    cell-centered multi-material ALE scheme using VOF interface reconstruction.
    Journal of Computational Physics, 229(16), 5755-5787.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using triple_point =
  keyword< triple_point_info, TAOCPP_PEGTL_STRING("triple_point") >;

struct gas_impact_info {
  using code = Code< P >;
  static std::string name() { return "Gas impact problem"; }
  static std::string shortDescription() { return
    "Select the gas impact test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the gas impact test problem. The
    purpose of this test problem is to test the correctness of the
    multi-material algorithm and its interface capturing
    capabilities. Example: "problem gas_impact". For more details, see
    Barlow, A., Hill, R., & Shashkov, M. (2014). Constrained optimization
    framework for interface-aware sub-scale dynamics closure model for
    multimaterial cells in Lagrangian and arbitrary Lagrangian–Eulerian
    hydrodynamics. Journal of Computational Physics, 276, 92-135.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using gas_impact =
  keyword< gas_impact_info, TAOCPP_PEGTL_STRING("gas_impact") >;

struct gas_impact_4mat_info {
  using code = Code< L >;
  static std::string name() { return "Gas impacting with two slabs problem"; }
  static std::string shortDescription() { return
    "Select the gas impacting with two slabs test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the test involving a gas impacting two
    slabs. It involves four materials: the impactor, two slabs and the
    background. The purpose of this test problem is to test the correctness of
    multi-material algorithm for more than three materials and its interface
    capturing capabilities under high deformation.
    Example: "problem gas_impact_4mat".)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using gas_impact_4mat =
  keyword< gas_impact_4mat_info, TAOCPP_PEGTL_STRING("gas_impact_4mat") >;

struct shock_hebubble_info {
  using code = Code< E >;
  static std::string name() { return "Shock He-bubble problem"; }
  static std::string shortDescription() { return
    "Select the shock He-bubble test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the shock He-bubble test problem. The
    purpose of this test problem is to test the correctness of the
    multi-material algorithm and its shock-interface interaction
    capabilities. Example: "problem shock_hebubble". For more details, see
    Quirk, J. J., & Karni, S. (1996). On the dynamics of a shock–bubble
    interaction. Journal of Fluid Mechanics, 318, 129-163.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using shock_hebubble =
  keyword< shock_hebubble_info, TAOCPP_PEGTL_STRING("shock_hebubble") >;

struct underwater_ex_info {
  using code = Code< D >;
  static std::string name() { return "Underwater explosion problem"; }
  static std::string shortDescription() { return
    "Select the underwater explosion test problem "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the underwater explosion test problem. The
    purpose of this test problem is to test the correctness of the
    multi-material algorithm and its interface capturing capabilities in the
    presence of strong shocks and large deformations.
    Example: "problem underwater_ex". For more details, see
    Chiapolino, A., Saurel, R., & Nkonga, B. (2017). Sharpening diffuse
    interfaces with compressible fluids on unstructured meshes. Journal of
    Computational Physics, 340, 389-417.)"; }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using underwater_ex =
  keyword< underwater_ex_info, TAOCPP_PEGTL_STRING("underwater_ex") >;

struct problem_info {
  using code = Code< t >;
  static std::string name() { return "Test problem"; }
  static std::string shortDescription() { return
    "Specify problem configuration for a partial differential equation solver";
  }
  static std::string longDescription() { return
    R"(This keyword is used to specify the problem configuration for a partial
    differential equation solver in the input file.)";
  }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + user_defined::string() + "\' | \'"
                  + shear_diff::string() + "\' | \'"
                  + slot_cyl::string() + "\' | \'"
                  + gauss_hump::string() + "\' | \'"
                  + cyl_advect::string() + "\' | \'"
                  + cyl_vortex::string() + "\' | \'"
                  + vortical_flow::string() + "\' | \'"
                  + nl_energy_growth::string() + "\' | \'"
                  + rayleigh_taylor::string() + "\' | \'"
                  + taylor_green::string() + "\' | \'"
                  + sod_shocktube::string() + "\' | \'"
                  + rotated_sod_shocktube::string() + "\' | \'"
                  + interface_advection::string() + "\' | \'"
                  + gauss_hump_compflow::string() + '\'';
    }
  };
};
using problem = keyword< problem_info, TAOCPP_PEGTL_STRING("problem") >;

struct navierstokes_info {
  using code = Code< N >;
  static std::string name() { return "Navier-Stokes"; }
  static std::string shortDescription() { return "Specify the Navier-Stokes "
    "(viscous) compressible flow physics configuration"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Navier-Stokes (viscous) compressible
    flow physics configuration. Example: "compflow physics navierstokes end")";
    }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using navierstokes =
  keyword< navierstokes_info, TAOCPP_PEGTL_STRING("navierstokes") >;

struct euler_info {
  using code = Code< E >;
  static std::string name() { return "Euler"; }
  static std::string shortDescription() { return "Specify the Euler (inviscid) "
    "compressible flow physics configuration"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Euler (inviscid) compressible
    flow physics configuration. Example: "compflow physics euler end")";
    }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using euler = keyword< euler_info, TAOCPP_PEGTL_STRING("euler") >;

struct veleq_info {
  using code = Code< V >;
  static std::string name() { return "Velocity equilibrium"; }
  static std::string shortDescription() { return "Specify the multi-material "
    " compressible flow with velocity equilibrium as physics configuration"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a compressible flow algorithm as physics
    configuration designed for multiple materials assuming velocity equailibrium
    (single velocity). Example: "multimat physics veleq end")";
    }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using veleq = keyword< veleq_info, TAOCPP_PEGTL_STRING("veleq") >;

struct advection_info {
  using code = Code< A >;
  static std::string name() { return "Advection"; }
  static std::string shortDescription() { return
    "Specify the advection physics configuration for a PDE "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the advection physics configuration for a
    PDE. Example: "transport physics advection end")";
    }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using advection = keyword< advection_info, TAOCPP_PEGTL_STRING("advection") >;

struct advdiff_info {
  using code = Code< D >;
  static std::string name() { return "Advection + diffusion"; }
  static std::string shortDescription() { return
    "Specify the advection + diffusion physics configuration for a PDE "; }
  static std::string longDescription() { return
    R"(This keyword is used to select the advection +diffusion physics
    configuration for a PDE. Example: "transport physics advdiff end")";
    }
  struct expect {
    static std::string description() { return "string"; }
  };
};
using advdiff = keyword< advdiff_info, TAOCPP_PEGTL_STRING("advdiff") >;

struct physics_info {
  using code = Code< p >;
  static std::string name() { return "Physics configuration"; }
  static std::string shortDescription() { return
    "Specify the physics configuration for a system of PDEs"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the physics configuration for a particular
    PDE system. Example: "physics navierstokes", which selects the Navier-Stokes
    equations for solving viscous compressible flow, given within the
    compflow ... end block. Valid options depend on the given block the keyword
    is used.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + advection::string() + "\' | \'"
                  + advdiff::string() + "\' | \'"
                  + navierstokes::string() + "\' | \'"
                  + euler::string() + '\'';
    }
  };
};
using physics = keyword< physics_info, TAOCPP_PEGTL_STRING("physics") >;

struct pde_diffusivity_info {
  static std::string name() { return "diffusivity"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) diffusivity)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of partial differential equations. Example:
    "diffusivity 5.0 2.0 3.0 end". The length of the vector depends on the
    particular type of PDE system and is controlled by the preceding keyword
    'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using pde_diffusivity =
  keyword< pde_diffusivity_info, TAOCPP_PEGTL_STRING("diffusivity") >;

struct pde_lambda_info {
  static std::string name() { return "lambda"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) lambda)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of partial differential equations. Example:
    "lambda 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of PDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using pde_lambda = keyword< pde_lambda_info, TAOCPP_PEGTL_STRING("lambda") >;

struct pde_u0_info {
  static std::string name() { return "u0"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) u0)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a vector of real numbers used to
    parameterize a system of partial differential equations. Example:
    "u0 5.0 2.0 3.0 end". The length of the vector depends on the particular
    type of PDE system and is controlled by the preceding keyword 'ncomp'.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using pde_u0 = keyword< pde_u0_info, TAOCPP_PEGTL_STRING("u0") >;

struct pde_alpha_info {
  static std::string name() { return "alpha"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) alpha)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to
    parameterize a system of partial differential equations. Example:
    "alpha 5.0".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_alpha = keyword< pde_alpha_info, TAOCPP_PEGTL_STRING("alpha") >;

struct pde_beta_info {
  static std::string name() { return "beta"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) beta)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to
    parameterize a system of partial differential equations. Example:
    "beta 5.0".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_beta = keyword< pde_beta_info, TAOCPP_PEGTL_STRING("beta") >;

struct pde_p0_info {
  static std::string name() { return "p0"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) p0)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to
    parameterize a system of partial differential equations. Example:
    "p0 10.0".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_p0 = keyword< pde_p0_info, TAOCPP_PEGTL_STRING("p0") >;

// nonlinear energy parameters here
struct pde_betax_info {
  static std::string name() { return "betax"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) betax)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to
    parameterize a system of partial differential equations. Example:
    "betax 1.0".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_betax = keyword< pde_betax_info, TAOCPP_PEGTL_STRING("betax") >;

struct pde_betay_info {
  static std::string name() { return "betay"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) betay)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to
    parameterize a system of partial differential equations. Example:
    "betay 0.75".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_betay = keyword< pde_betay_info, TAOCPP_PEGTL_STRING("betay") >;

struct pde_betaz_info {
  static std::string name() { return "betaz"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) betaz)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to
    parameterize a system of partial differential equations. Example:
    "betaz 0.5".)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_betaz = keyword< pde_betaz_info, TAOCPP_PEGTL_STRING("betaz") >;

struct pde_ce_info {
  static std::string name() { return "ce"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) ce)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to parameterize the
    Euler equations solving the manufactured solution test case "non-linear
    energy growth". Example: "ce -1.0". For more information on the test case see
    Waltz, et. al, "Manufactured solutions for the three-dimensional Euler
    equations with relevance to Inertial Confinement Fusion", Journal of
    Computational Physics 267 (2014) 196-209.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_ce = keyword< pde_ce_info, TAOCPP_PEGTL_STRING("ce") >;

struct pde_kappa_info {
  static std::string name() { return "kappa"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) kappa)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to
    parameterize a system of partial differential equations. Example:
    "kappa 0.8")"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_kappa = keyword< pde_kappa_info, TAOCPP_PEGTL_STRING("kappa") >;

struct pde_r0_info {
  static std::string name() { return "r0"; }
  static std::string shortDescription() { return
    R"(Set PDE parameter(s) r0)"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a real number used to parameterize the
    Euler equations solving the manufactured solution test case "non-linear
    energy growth". Example: "r0 2.0". For more information on the test case see
    Waltz, et. al, "Manufactured solutions for the three-dimensional Euler
    equations with relevance to Inertial Confinement Fusion", Journal of
    Computational Physics 267 (2014) 196-209.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using pde_r0 = keyword< pde_r0_info, TAOCPP_PEGTL_STRING("r0") >;

struct ctau_info {
  static std::string name() { return "ctau"; }
  static std::string shortDescription() { return
    R"(Set FCT mass diffusion coefficient, ctau)"; }
  static std::string longDescription() { return
    R"(This keyword is used to set the mass diffusion coefficient used in
    flux-corrected transport, used for integrating transport equations. Example:
    "ctau 1.0".)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static constexpr type upper = 1.0;
    static std::string description() { return "real"; }
    static std::string choices() {
      return "real between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "]";
    }
  };
};
using ctau = keyword< ctau_info, TAOCPP_PEGTL_STRING("ctau") >;

struct fcteps_info {
  static std::string name() { return "Small number for FCT"; }
  static std::string shortDescription() { return
    R"(A number that is considered small enough for FCT)"; }
  static std::string longDescription() { return
    R"(This keyword is used to set the epsilon (a small number) below which FCT
    quantities are considered small enough to be treated as zero. Setting this
    number to be somewhat larger than the machine zero, e.g., 1.0e-15, helps
    ignoring some noise that otherwise could contaminate the solution.)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static constexpr type upper = 1.0;
    static std::string description() { return "real"; }
    static std::string choices() {
      return "real between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "]";
    }
  };
};
using fcteps = keyword< fcteps_info, TAOCPP_PEGTL_STRING("fcteps") >;

struct cweight_info {
  static std::string name() { return "cweight"; }
  static std::string shortDescription() { return
    R"(Set value for central linear weight used by WENO, cweight)"; }
  static std::string longDescription() { return
    R"(This keyword is used to set the central linear weight used for the
    central stencil in the Weighted Essentially Non-Oscillatory (WENO) limiter
    for discontinuous Galerkin (DG) methods. Example:
    "cweight 10.0".)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 1.0;
    static constexpr type upper = 1000.0;
    static std::string description() { return "real"; }
    static std::string choices() {
      return "real between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "]";
    }
  };
};
using cweight = keyword< cweight_info, TAOCPP_PEGTL_STRING("cweight") >;

struct sideset_info {
  static std::string name() { return "sideset"; }
  static std::string shortDescription() { return
    "Specify configuration for setting BC on a side set";
  }
  static std::string longDescription() { return
    R"(This keyword is used to specify boundary conditions on a side set for a
    solving partial differential equation.)";
  }
  struct expect {
    using type = std::string;
    static std::string description() { return "strings"; }
  };
};
using sideset = keyword< sideset_info, TAOCPP_PEGTL_STRING("sideset") >;

struct bc_dirichlet_info {
  static std::string name() { return "Dirichlet boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing Dirichlet boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an bc_dirichlet ... end block, used to
    specify the configuration for setting Dirichlet boundary conditions (BC) for
    a partial differential equation. This keyword is used to list multiple side
    sets on which a prescribed Dirichlet BC is then applied. Such prescribed BCs
    at each point in space and time are evaluated using a built-in function,
    e.g., using the method of manufactured solutions.
    Keywords allowed in a bc_dirichlet ... end block: )" + std::string("\'")
    + sideset::string() + "\'. "
    + R"(For an example bc_dirichlet ... end block, see
      doc/html/inicter_example_shear.html.)";
  }
};
using bc_dirichlet =
  keyword< bc_dirichlet_info, TAOCPP_PEGTL_STRING("bc_dirichlet") >;

struct bc_sym_info {
  static std::string name() { return "Symmetry boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing symmetry boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an bc_sym ... end block, used to
    specify the configuration for setting symmetry boundary conditions for a
    partial differential equation. Keywords allowed in a bc_sym ... end
    block: )" + std::string("\'")
    + sideset::string() + "\'. "
    + R"(For an example bc_sym ... end block, see
      doc/html/inicter_example_gausshump.html.)";
  }
};
using bc_sym =
  keyword< bc_sym_info, TAOCPP_PEGTL_STRING("bc_sym") >;

struct point_info {
  static std::string name() { return "point"; }
  static std::string shortDescription() { return "Specify a point"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a point, used, e.g., in specifying a
    point in 3D space for setting a stagnation (velocity vector = 0).  Example
    specification: 'point 0.0 0.1 0.2 end')";
  }
  struct expect {
    using type = tk::real;
    static std::string description() { return "3 reals"; }
  };
};
using point = keyword< point_info, TAOCPP_PEGTL_STRING("point") >;

struct radius_info {
  static std::string name() { return "radius"; }
  static std::string shortDescription() { return "Specify a radius"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a radius, used, e.g., in specifying a
    point in 3D space for setting a stagnation (velocity vector = 0).  Example
    specification: 'radius 1.0e-5')";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using radius = keyword< radius_info, TAOCPP_PEGTL_STRING("radius") >;

struct bc_stag_info {
  static std::string name() { return "Stagnation boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing stagnation boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an bc_stag ... end block, used to
    specify the configuration for setting stagnation boundary conditions for a
    partial differential equation. Keywords allowed in a bc_stag ... end
    block: )" + std::string("\'")
    + point::string() + "\', \'"
    + radius::string() + "\'. ";
  }
};
using bc_stag = keyword< bc_stag_info, TAOCPP_PEGTL_STRING("bc_stag") >;

struct bc_skip_info {
  static std::string name() { return "Skip boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing skip boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an bc_skip ... end block, used to
    specify the configuration for setting 'skip' boundary conditions for a
    partial differential equation. If a mesh point falls into a skip region,
    configured by a point and a radius, any application of boundary conditions
    on those points will be skipped. Keywords allowed in a bc_skip ... end
    block: )" + std::string("\'")
    + point::string() + "\', \'"
    + radius::string() + "\'. ";
  }
};
using bc_skip = keyword< bc_skip_info, TAOCPP_PEGTL_STRING("bc_skip") >;

struct bc_inlet_info {
  static std::string name() { return "Inlet boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing inlet boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an bc_inlet ... end block, used to
    specify the configuration for setting inlet boundary conditions for a
    partial differential equation. Keywords allowed in a bc_inlet ... end
    block: )" + std::string("\'")
    + sideset::string() + "\'. "
    + R"(For an example bc_inlet ... end block, see
      doc/html/inicter_example_gausshump.html.)";
  }
};
using bc_inlet =
  keyword< bc_inlet_info, TAOCPP_PEGTL_STRING("bc_inlet") >;

struct bc_outlet_info {
  static std::string name() { return "Inlet boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing outlet boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an bc_outlet ... end block, used to
    specify the configuration for setting outlet boundary conditions for a
    partial differential equation. Keywords allowed in a bc_outlet ... end
    block: )" + std::string("\'")
    + sideset::string() + "\'. "
    + R"(For an example bc_outlet ... end block, see
      doc/html/inicter_example_gausshump.html.)";
  }
};
using bc_outlet =
  keyword< bc_outlet_info, TAOCPP_PEGTL_STRING("bc_outlet") >;

struct bc_farfield_info {
  static std::string name() { return "Farfield boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing farfield boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a bc_farfield ... end block, used
    to specify the configuration for setting farfield boundary conditions
    for the compressible flow equations. Keywords allowed in a bc_farfield
    ... end block: )" + std::string("\'")
    + density::string() + "\', \'"
    + pressure::string() + "\', \'"
    + velocity::string() + "\', \'"
    + sideset::string() + "\'. ";
  }
};
using bc_farfield =
  keyword< bc_farfield_info, TAOCPP_PEGTL_STRING("bc_farfield") >;

struct bc_extrapolate_info {
  static std::string name() { return "Extrapolation boundary condition"; }
  static std::string shortDescription() { return
    "Start configuration block describing Extrapolation boundary conditions"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a bc_extrapolate ... end block, used to
    specify the configuration for setting extrapolation boundary conditions for a
    partial differential equation. Keywords allowed in a bc_extrapolate ... end
    block: )" + std::string("\'")
    + sideset::string() + "\'. "
    + R"(For an example bc_extrapolate ... end block, see
      doc/html/inciter_example_gausshump.html.)";
  }
};
using bc_extrapolate =
  keyword< bc_extrapolate_info, TAOCPP_PEGTL_STRING("bc_extrapolate") >;

struct id_info {
  static std::string name() { return "id"; }
  static std::string shortDescription() { return "ID"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify an ID, a positive integer.)";
  }
  struct expect {
    using type = uint64_t;
    static constexpr type lower = 1;
    static std::string description() { return "uint"; }
  };
};
using id = keyword< id_info, TAOCPP_PEGTL_STRING("id") >;

struct prelax_info {
  static std::string name() { return "Pressure relaxation"; }
  static std::string shortDescription() { return
    "Select multi-material pressure relaxation"; }
  static std::string longDescription() { return
    R"(This keyword is used to select pressure relaxation between multiple
       materials. A value of 1 is used to specify finite-rate pressure
       relaxation, and a value of 2 to specify instantaneous pressure
       relaxation. It is used only for the multi-material solver, and has no
       effect when used for the other PDE types.)";
  }
  struct expect {
    using type = int;
    static std::string description() { return "string"; }
    static std::string choices() { return "1 | 2 | 0"; }
  };
};
using prelax = keyword< prelax_info, TAOCPP_PEGTL_STRING("prelax") >;

struct prelax_timescale_info {
  static std::string name() { return "Pressure relaxation time-scale"; }
  static std::string shortDescription() { return
    "Time-scale for multi-material finite pressure relaxation"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the time-scale at which finite pressure
       relaxation between multiple materials occurs. The default value of 1.0
       corresponds to a relaxation time of the order of time required for a
       sound wave to pass through a computational element. It is used only for
       multimat when finite-rate pressure relaxation is selected, and has no
       effect for the other PDE types.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.001;
    static std::string description() { return "real"; }
  };
};
using prelax_timescale = keyword< prelax_timescale_info,
                                  TAOCPP_PEGTL_STRING("prelax_timescale") >;

struct intsharp_info {
  static std::string name() { return "Interface sharpening"; }
  static std::string shortDescription() { return
    "Turn multi-material interface sharpening on/off"; }
  static std::string longDescription() { return
    R"(This keyword is used to turn interface sharpening on/off. It uses the
       multi-material THINC interface reconstruction.
       Ref. Pandare A. K., Waltz J., & Bakosi J. (2021) Multi-Material
       Hydrodynamics with Algebraic Sharp Interface Capturing. Computers &
       Fluids, doi: https://doi.org/10.1016/j.compfluid.2020.104804. It is used
       for the multi-material and the transport solver, and has no effect when
       used for the other PDE types.)";
  }
  struct expect {
    using type = int;
    static std::string description() { return "string"; }
    static std::string choices() { return "1 | 0"; }
  };
};
using intsharp = keyword< intsharp_info, TAOCPP_PEGTL_STRING("intsharp") >;

struct intsharp_param_info {
  static std::string name() { return "Interface sharpening parameter"; }
  static std::string shortDescription() { return
    "Parameter for multi-material interface sharpening"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the parameter for the interface
       sharpening. This parameter affects how many cells the material interfaces
       span, after the use of sharpening. It is used for multimat and transport,
       and has no effect for the other PDE types.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.1;
    static std::string description() { return "real"; }
  };
};
using intsharp_param = keyword< intsharp_param_info,
                                  TAOCPP_PEGTL_STRING("intsharp_param") >;

struct mat_gamma_info {
  static std::string name() { return "gamma"; }
  static std::string shortDescription() { return "ratio of specific heats"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the material property, ratio of specific
       heats.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using mat_gamma = keyword< mat_gamma_info, TAOCPP_PEGTL_STRING("gamma") >;

struct mat_pstiff_info {
  static std::string name() { return "pstiff"; }
  static std::string shortDescription() { return "EoS stiffness parameter"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the material property, stiffness
       parameter in the stiffened gas equation of state.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using mat_pstiff = keyword< mat_pstiff_info, TAOCPP_PEGTL_STRING("pstiff") >;

struct mat_mu_info {
  static std::string name() { return "mu"; }
  static std::string shortDescription() { return "dynamic viscosity"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the material property, dynamic
       viscosity.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using mat_mu = keyword< mat_mu_info, TAOCPP_PEGTL_STRING("mu") >;

struct mat_cv_info {
  static std::string name() { return "cv"; }
  static std::string shortDescription() {
    return "specific heat at constant volume"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the material property, specific heat at
       constant volume.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using mat_cv = keyword< mat_cv_info, TAOCPP_PEGTL_STRING("cv") >;

struct mat_k_info {
  static std::string name() { return "k"; }
  static std::string shortDescription() { return "heat conductivity"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify the material property, heat
       conductivity.)";
  }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static std::string description() { return "real"; }
  };
};
using mat_k = keyword< mat_k_info, TAOCPP_PEGTL_STRING("k") >;

struct material_info {
  static std::string name() { return "Material properties block"; }
  static std::string shortDescription() { return
    "Start configuration block for material properties"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a material ... end block, used to
    specify material properties. Keywords allowed in a material ... end
    block: )" + std::string("\'")
    + id::string()+ "\', \'"
    + mat_gamma::string()+ "\', \'"
    + mat_pstiff::string()+ "\', \'"
    + mat_mu::string()+ "\', \'"
    + mat_cv::string()+ "\', \'"
    + mat_k::string() + "\'. "
    + R"(For an example material ... end block, see
      doc/html/inicter_example_compflow.html.)";
  }
};
using material = keyword< material_info, TAOCPP_PEGTL_STRING("material") >;

struct transport_info {
  static std::string name() { return "Transport"; }
  static std::string shortDescription() { return
    "Start configuration block for an transport equation"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce an transport ... end block, used to
    specify the configuration for a transport equation type. Keywords allowed
    in an transport ... end block: )" + std::string("\'")
    + depvar::string() + "\', \'"
    + ncomp::string() + "\', \'"
    + problem::string() + "\', \'"
    + physics::string() + "\', \'"
    + pde_diffusivity::string() + "\', \'"
    + pde_lambda::string() + "\', \'"
    + bc_dirichlet::string() + "\', \'"
    + bc_sym::string() + "\', \'"
    + bc_inlet::string() + "\', \'"
    + bc_outlet::string() + "\', \'"
    + pde_u0::string() + "\'. "
    + intsharp::string() + "\', \'"
    + intsharp_param::string() + "\', \'"
    + R"(For an example transport ... end block, see
      doc/html/inicter_example_transport.html.)";
  }
};
using transport = keyword< transport_info, TAOCPP_PEGTL_STRING("transport") >;

struct compflow_info {
  static std::string name() { return "Compressible single-material flow"; }
  static std::string shortDescription() { return
    "Start configuration block for the compressible flow equations"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the compflow ... end block, used to
    specify the configuration for a system of partial differential equations,
    governing compressible fluid flow. Keywords allowed in an compflow ... end
    block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + physics::string() + "\', \'"
    + problem::string() + "\', \'"
    + material::string() + "\', \'"
    + npar::string() + "\', \'"
    + pde_alpha::string() + "\', \'"
    + pde_p0::string() + "\', \'"
    + pde_betax::string() + "\', \'"
    + pde_betay::string() + "\', \'"
    + pde_betaz::string() + "\', \'"
    + pde_beta::string() + "\', \'"
    + pde_r0::string() + "\', \'"
    + pde_ce::string() + "\', \'"
    + pde_kappa::string() + "\', \'"
    + bc_dirichlet::string() + "\', \'"
    + bc_sym::string() + "\', \'"
    + bc_inlet::string() + "\', \'"
    + bc_outlet::string() + "\', \'"
    + bc_farfield::string() + "\', \'"
    + bc_extrapolate::string() + "\'."
    + R"(For an example compflow ... end block, see
      doc/html/inicter_example_compflow.html.)";
  }
};
using compflow = keyword< compflow_info, TAOCPP_PEGTL_STRING("compflow") >;

struct multimat_info {
  static std::string name() { return "Compressible multi-material flow"; }
  static std::string shortDescription() { return "Start configuration block "
    "for the multi-material compressible flow equations"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the multimat ... end block,
    used to specify the configuration for a system of partial differential
    equations, governing multi-material compressible fluid flow. Keywords
    allowed in a multimat ... end block: )" + std::string("\'")
    + depvar::string()+ "\', \'"
    + physics::string() + "\', \'"
    + problem::string() + "\', \'"
    + material::string() + "\', \'"
    + nmat::string() + "\', \'"
    + prelax::string() + "\', \'"
    + prelax_timescale::string() + "\', \'"
    + intsharp::string() + "\', \'"
    + intsharp_param::string() + "\', \'"
    + pde_alpha::string() + "\', \'"
    + pde_p0::string() + "\', \'"
    + pde_betax::string() + "\', \'"
    + pde_betay::string() + "\', \'"
    + pde_betaz::string() + "\', \'"
    + pde_beta::string() + "\', \'"
    + pde_r0::string() + "\', \'"
    + pde_ce::string() + "\', \'"
    + pde_kappa::string() + "\', \'"
    + bc_dirichlet::string() + "\', \'"
    + bc_sym::string() + "\', \'"
    + bc_inlet::string() + "\', \'"
    + bc_outlet::string() + "\', \'"
    + bc_extrapolate::string() + "\'."
    + R"(For an example multimat ... end block, see
      doc/html/inicter_example_multimat.html.)";
  }
};
using multimat = keyword< multimat_info, TAOCPP_PEGTL_STRING("multimat") >;

struct rcb_info {
  static std::string name() { return "recursive coordinate bisection"; }
  static std::string shortDescription() { return
    "Select recursive coordinate bisection mesh partitioner"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the recursive coordinate bisection (RCB)
    mesh partitioner. RCB is a geometry-based partitioner used to distribute an
    input mesh among processing elements. See
    Control/Options/PartitioningAlgorithm.hpp for other valid options.)"; }
};
using rcb = keyword< rcb_info, TAOCPP_PEGTL_STRING("rcb") >;

struct rib_info {
  static std::string name() { return "recursive inertial bisection"; }
  static std::string shortDescription() { return
    "Select recursive inertial bisection mesh partitioner"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the recursive inertial bisection (RIB)
    mesh partitioner. RIB is a geometry-based partitioner used to distribute an
    input mesh among processing elements. See
    Control/Options/PartitioningAlgorithm.hpp for other valid options.)"; }
};
using rib = keyword< rib_info, TAOCPP_PEGTL_STRING("rib") >;

struct hsfc_info {
  static std::string name() { return "Hilbert space filling curve"; }
  static std::string shortDescription() { return
    "Select Hilbert Space Filling Curve (HSFC) mesh partitioner"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Hilbert Space Filling Curve (HSFC)
    mesh partitioner. HSFC is a geometry-based partitioner used to distribute an
    input mesh among processing elements. See
    Control/Options/PartitioningAlgorithm.hpp for other valid options.)"; }
};
using hsfc = keyword< hsfc_info, TAOCPP_PEGTL_STRING("hsfc") >;

struct mj_info {
  static std::string name() { return "multi-jagged"; }
  static std::string shortDescription() { return
    "Select multi-jagged (MJ) mesh partitioner"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the multi-jagged (MJ) mesh partitioner.
    MJ is a geometry-based partitioner used to distribute an input mesh among
    processing elements. See
    Control/Options/PartitioningAlgorithm.hpp for other valid options.)"; }
};
using mj = keyword< mj_info, TAOCPP_PEGTL_STRING("mj") >;

struct phg_info {
  static std::string name() { return "hypergraph"; }
  static std::string shortDescription() { return
    "Select parallel hypergraph mesh partitioner"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the parallel hypergraph (PHG)
    mesh partitioner. PHG is a graph-based partitioner used to distribute an
    input mesh among processing elements. See
    Control/Options/PartitioningAlgorithm.hpp for other valid options.)"; }
};
using phg = keyword< phg_info, TAOCPP_PEGTL_STRING("phg") >;

struct algorithm_info {
  static std::string name() { return "algorithm"; }
  static std::string shortDescription() { return
    "Select mesh partitioning algorithm"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a mesh partitioning algorithm. See
    Control/Options/PartitioningAlgorithm.hpp for valid options.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + rcb::string() + "\' | \'"
                  + rib::string() + "\' | \'"
                  + hsfc::string() + "\' | \'"
                  + mj::string() + "\' | \'"
                  + phg::string() + '\'';
    }
  };
};
using algorithm = keyword< algorithm_info, TAOCPP_PEGTL_STRING("algorithm") >;

struct partitioning_info {
  static std::string name() { return "partitioning"; }
  static std::string shortDescription() { return
    "Start configuration block for mesh partitioning"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a partitioning ... end block, used to
    specify the configuration for mesh partitioning. Keywords allowed
    in a partitioning ... end block: )" + std::string("\'")
    + algorithm::string() + "\'.";
  }
};
using partitioning = keyword< partitioning_info, TAOCPP_PEGTL_STRING("partitioning") >;

struct amr_uniform_info {
  using code = Code< u >;
  static std::string name() { return "uniform refine"; }
  static std::string shortDescription() { return
    "Select uniform initial mesh refinement"; }
  static std::string longDescription() { return
    R"(This keyword is used to select uniform initial mesh refinement.)"; }
};
using amr_uniform = keyword< amr_uniform_info, TAOCPP_PEGTL_STRING("uniform") >;

struct amr_uniform_derefine_info {
  using code = Code< d >;
  static std::string name() { return "uniform derefine"; }
  static std::string shortDescription() { return
    "Select uniform initial mesh de-refinement"; }
  static std::string longDescription() { return
    R"(This keyword is used to select uniform initial mesh de-refinement.)"; }
};
using amr_uniform_derefine =
  keyword< amr_uniform_derefine_info, TAOCPP_PEGTL_STRING("uniform_derefine") >;

struct amr_initial_conditions_info {
  using code = Code< i >;
  static std::string name() { return "initial conditions"; }
  static std::string shortDescription() { return
    "Select initial-conditions-based initial mesh refinement"; }
  static std::string longDescription() { return
    R"(This keyword is used to select initial-conditions-based initial mesh
       refinement.)"; }
};
using amr_initial_conditions =
  keyword< amr_initial_conditions_info, TAOCPP_PEGTL_STRING("ic") >;

struct amr_edgelist_info {
  using code = Code< e >;
  static std::string name() { return "edge list"; }
  static std::string shortDescription() { return
    "Configure edge-node pairs for initial refinement"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a list of edges that are explicitly
    tagged for initial refinement during setup in inciter. The keyword
    introduces an edgelist ... end block within an amr ... end block and must
    contain a list of integer pairs, i.e., the number of ids must be even,
    denoting the end-points of the nodes (=edge) which should be tagged for
    refinement.)"; }
  struct expect {
    using type = std::size_t;
    static constexpr type lower = 0;
    static std::string description() { return "two ints"; }
  };
};
using amr_edgelist =
  keyword< amr_edgelist_info, TAOCPP_PEGTL_STRING("edgelist") >;

struct amr_coords_info {
  using code = Code< c >;
  static std::string name() { return "coordinates"; }
  static std::string shortDescription() { return
    "Configure initial refinement using coordinate planes"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure entire volumes on a given side of a
    plane in 3D space. The keyword introduces an coords ... end block within
    an amr ... end block and must contain the either or multiple of the
    following keywords: x- <real>, x+ <real>, y- <real>, y+ <real>, z- <real>,
    z+ <real>. All edges of the input mesh will be tagged for refinement whose
    end-points lie less than (-) or larger than (+) the real number given.
    Example: 'x- 0.5' refines all edges whose end-point coordinates are less
    than 0.5. Multiple specifications are understood by combining with a logical
    AND. That is: 'x- 0.5 y+ 0.3' refines all edges whose end-point x
    coordinates are less than 0.5 AND y coordinates are larger than 0.3.)"; }
};
using amr_coords =
  keyword< amr_coords_info, TAOCPP_PEGTL_STRING("coords") >;

struct amr_initial_info {
  static std::string name() { return "Initial refinement typelist"; }
  static std::string shortDescription() { return
    "Configure initial mesh refinement (before time stepping)"; }
  static std::string longDescription() { return
    R"(This keyword is used to add to a list of initial mesh refinement types
    that happens before t = 0. Example: initial uniform initial ic inital
    uniform, which yiedls an initial uniform refinement, followed by a
    refinement based on the numerical error computed based on the initial
    conditions, followed by another step of unfirom refinement.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + amr_uniform::string() + "\' | \'"
                  + amr_uniform_derefine::string()  + "\' | \'"
                  + amr_initial_conditions::string() + "\' | \'"
                  + amr_edgelist::string() + "\' | \'"
                  + amr_coords::string() + '\'';
    }
  };
};
using amr_initial = keyword< amr_initial_info, TAOCPP_PEGTL_STRING("initial") >;

struct amr_refvar_info {
  static std::string name() { return "refinement variable(s)"; }
  static std::string shortDescription() { return
    "Configure dependent variables used for adaptive mesh refinement"; }
  static std::string longDescription() { return
    R"(This keyword is used to configured a list of dependent variables that
    trigger adaptive mesh refinement based on estimating their numerical error.
    These refinement variables are used for both initial (i.e., before time
    stepping) mesh refinement as well as during time stepping. Only previously
    (i.e., earlier in the input file) selected dependent variables can be
    configured as refinement variables. Dependent variables are required to be
    defined in all equation system configuration blocks, e.g., transport ...
    end, by using the 'depvar' keyword. Example: transport depvar c end amr
    refvar c end end. Selecting a particular scalar component in a system is
    done by appending the equation number to the refvar: Example: transport
    depvar q ncomp 3 end amr refvar q1 q2 end end, which configures two
    refinement variables: the first and third scalar component of the previously
    configured transport equation system.)"; }
  struct expect {
    static std::string description() { return "strings"; }
  };
};
using amr_refvar = keyword< amr_refvar_info, TAOCPP_PEGTL_STRING("refvar") >;

struct amr_xminus_info {
  static std::string name() { return "initial refinement: x-"; }
  static std::string shortDescription() { return "Configure initial refinement "
    "for coordinates lower than an x-normal plane"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a mesh refinement volume for edges
    whose end-points are less than the x coordinate of a plane perpendicular to
    coordinate x in 3D space. The keyword must be used in a coords ... end
    block within an amr ... end block with syntax 'x- <real>'. All edges of the
    input mesh will be tagged for refinement whose end-points lie less than (-)
    the real number given. Example: 'x- 0.5' refines all edges whose end-point
    coordinates are less than 0.5.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using amr_xminus =
  keyword< amr_xminus_info, TAOCPP_PEGTL_STRING("x-") >;

struct amr_xplus_info {
  static std::string name() { return "initial refinement: x+"; }
  static std::string shortDescription() { return "Configure initial refinement "
    "for coordinates larger than an x-normal plane"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a mesh refinement volume for edges
    whose end-points are larger than the x coordinate of a plane perpendicular
    to coordinate x in 3D space. The keyword must be used in a coords ... end
    block within an amr ... end block with syntax 'x+ <real>'. All edges of the
    input mesh will be tagged for refinement whose end-points lie larger than
    (+) the real number given. Example: 'x+ 0.5' refines all edges whose
    end-point coordinates are larger than 0.5.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using amr_xplus =
  keyword< amr_xplus_info, TAOCPP_PEGTL_STRING("x+") >;

struct amr_yminus_info {
  static std::string name() { return "initial refinement: y-"; }
  static std::string shortDescription() { return "Configure initial refinement "
    "for coordinates lower than an y-normal plane"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a mesh refinement volume for edges
    whose end-points are less than the y coordinate of a plane perpendicular to
    coordinate y in 3D space. The keyword must be used in a coords ... end
    block within an amr ... end block with syntax 'y- <real>'. All edges of the
    input mesh will be tagged for refinement whose end-points lie less than (-)
    the real number given. Example: 'y- 0.5' refines all edges whose end-point
    coordinates are less than 0.5.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using amr_yminus =
  keyword< amr_yminus_info, TAOCPP_PEGTL_STRING("y-") >;

struct amr_yplus_info {
  static std::string name() { return "initial refinement: y+"; }
  static std::string shortDescription() { return "Configure initial refinement "
    "for coordinates larger than an y-normal plane"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a mesh refinement volume for edges
    whose end-points are larger than the y coordinate of a plane perpendicular
    to coordinate y in 3D space. The keyword must be used in a coords ... end
    block within an amr ... end block with syntax 'y+ <real>'. All edges of the
    input mesh will be tagged for refinement whose end-points lie larger than
    (+) the real number given. Example: 'y+ 0.5' refines all edges whose
    end-point coordinates are larger than 0.5.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using amr_yplus =
  keyword< amr_yplus_info, TAOCPP_PEGTL_STRING("y+") >;

struct amr_zminus_info {
  static std::string name() { return "initial refinement: z-"; }
  static std::string shortDescription() { return "Configure initial refinement "
    "for coordinates lower than an z-normal plane"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a mesh refinement volume for edges
    whose end-points are less than the z coordinate of a plane perpendicular to
    coordinate z in 3D space. The keyword must be used in a coords ... end
    block within an amr ... end block with syntax 'z- <real>'. All edges of the
    input mesh will be tagged for refinement whose end-points lie less than (-)
    the real number given. Example: 'z- 0.5' refines all edges whose end-point
    coordinates are less than 0.5.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using amr_zminus =
  keyword< amr_zminus_info, TAOCPP_PEGTL_STRING("z-") >;

struct amr_zplus_info {
  static std::string name() { return "initial refinement: z+"; }
  static std::string shortDescription() { return "Configure initial refinement "
    "for coordinates larger than an z-normal plane"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a mesh refinement volume for edges
    whose end-points are larger than the z coordinate of a plane perpendicular
    to coordinate z in 3D space. The keyword must be used in a coords ... end
    block within an amr ... end block with syntax 'z+ <real>'. All edges of the
    input mesh will be tagged for refinement whose end-points lie larger than
    (+) the real number given. Example: 'z+ 0.5' refines all edges whose
    end-point coordinates are larger than 0.5.)"; }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real"; }
  };
};
using amr_zplus =
  keyword< amr_zplus_info, TAOCPP_PEGTL_STRING("z+") >;

struct amr_jump_info {
  static std::string name() { return "jump"; }
  static std::string shortDescription() { return
    "Error estimation based on the jump in the solution normalized by solution";
  }
  static std::string longDescription() { return
    R"(This keyword is used to select the jump-based error indicator for
    solution-adaptive mesh refinement. The error is estimated by computing the
    magnitude of the jump in the solution value normalized by the solution
    value.)"; }
};
using amr_jump =
  keyword< amr_jump_info, TAOCPP_PEGTL_STRING("jump") >;

struct amr_hessian_info {
  static std::string name() { return "Hessian"; }
  static std::string shortDescription() { return
    "Error estimation based on the Hessian normalized by solution value"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Hessian-based error indicator for
    solution-adaptive mesh refinement. The error is estimated by computing the
    Hessian (2nd derivative matrix) of the solution normalized by sum of the
    absolute values of the gradients at edges-end points.)"; }
};
using amr_hessian = keyword< amr_hessian_info, TAOCPP_PEGTL_STRING("hessian") >;

struct amr_error_info {
  static std::string name() { return "Error estimator"; }
  static std::string shortDescription() { return
    "Configure the error type for solution-adaptive mesh refinement"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the algorithm used to estimate the error
    for solution-adaptive mesh refinement.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + amr_jump::string() + "\' | \'"
                  + amr_hessian::string() + '\'';
    }
  };
};
using amr_error = keyword< amr_error_info, TAOCPP_PEGTL_STRING("error") >;

struct amr_t0ref_info {
  static std::string name() { return "Mesh refinement at t<0"; }
  static std::string shortDescription() { return
    "Enable mesh refinement at t<0"; }
  static std::string longDescription() { return
    R"(This keyword is used to enable initial mesh refinement, which can be
    configured to perform multiple levels of mesh refinement based on various
    refinement criteria and configuration settings.)";
  }
  struct expect {
    using type = bool;
    static std::string choices() { return "true | false"; }
    static std::string description() { return "string"; }
  };
};
using amr_t0ref = keyword< amr_t0ref_info, TAOCPP_PEGTL_STRING("t0ref") >;

struct amr_dtref_info {
  static std::string name() { return "Mesh refinement at t>0"; }
  static std::string shortDescription() { return
    "Enable mesh refinement at t>0"; }
  static std::string longDescription() { return
    R"(This keyword is used to enable soution-adaptive mesh refinement during "
    "time stepping.)";
  }
  struct expect {
    using type = bool;
    static std::string choices() { return "true | false"; }
    static std::string description() { return "string"; }
  };
};
using amr_dtref = keyword< amr_dtref_info, TAOCPP_PEGTL_STRING("dtref") >;

struct amr_dtref_uniform_info {
  static std::string name() { return "Uniform-only mesh refinement at t>0"; }
  static std::string shortDescription() { return
    "Enable mesh refinement at t>0 but only perform uniform refinement"; }
  static std::string longDescription() { return R"(This keyword is used to force
    uniform-only soution-adaptive mesh refinement during time stepping.)";
  }
  struct expect {
    using type = bool;
    static std::string choices() { return "true | false"; }
    static std::string description() { return "string"; }
  };
};
using amr_dtref_uniform =
  keyword< amr_dtref_uniform_info, TAOCPP_PEGTL_STRING("dtref_uniform") >;

struct amr_dtfreq_info {
  static std::string name() { return "Mesh refinement frequency"; }
  static std::string shortDescription() { return
    "Set mesh refinement frequency during time stepping"; }
  static std::string longDescription() { return
    R"(This keyword is used to configure the frequency of mesh refinement
    during time stepping. The default is 3, which means that mesh refinement
    will be performed every 3rd time step.)";
  }
  struct expect {
    using type = std::size_t;
    static constexpr type lower = 1;
    static constexpr type upper = std::numeric_limits< type >::max();
    static std::string description() { return "int"; }
    static std::string choices() {
      return "integer between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using amr_dtfreq = keyword< amr_dtfreq_info, TAOCPP_PEGTL_STRING("dtfreq") >;

struct amr_tolref_info {
  static std::string name() { return "refine tolerance"; }
  static std::string shortDescription() { return "Configure refine tolerance"; }
  static std::string longDescription() { return
    R"(This keyword is used to set the tolerance used to tag an edge for
    refinement if the relative error exceeds this value.)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static constexpr type upper = 1.0;
    static std::string description() { return "real"; }
    static std::string choices() {
      return "integer between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using amr_tolref =
  keyword< amr_tolref_info, TAOCPP_PEGTL_STRING("tol_refine") >;

struct amr_tolderef_info {
  static std::string name() { return "derefine tolerance"; }
  static std::string shortDescription() {
    return "Configure derefine tolerance"; }
  static std::string longDescription() { return
    R"(This keyword is used to set the tolerance used to tag an edge for
    derefinement if the relative error is below this value.)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static constexpr type upper = 1.0;
    static std::string description() { return "real"; }
    static std::string choices() {
      return "integer between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using amr_tolderef =
  keyword< amr_tolderef_info, TAOCPP_PEGTL_STRING("tol_derefine") >;

struct amr_info {
  static std::string name() { return "AMR"; }
  static std::string shortDescription() { return
    "Start configuration block configuring adaptive mesh refinement"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the amr ... end block, used to
    configure adaptive mesh refinement. Keywords allowed
    in this block: )" + std::string("\'")
    + amr_t0ref::string() + "\' | \'"
    + amr_dtref::string() + "\' | \'"
    + amr_dtref_uniform::string() + "\' | \'"
    + amr_dtfreq::string() + "\' | \'"
    + amr_initial::string() + "\' | \'"
    + amr_refvar::string() + "\' | \'"
    + amr_tolref::string() + "\' | \'"
    + amr_tolderef::string() + "\' | \'"
    + amr_error::string() + "\' | \'"
    + amr_coords::string() + "\' | \'"
    + amr_edgelist::string() + "\'.";
  }
};
using amr = keyword< amr_info, TAOCPP_PEGTL_STRING("amr") >;

struct pref_spectral_decay_info {
  static std::string name() { return "spectral decay"; }
  static std::string shortDescription() { return "Select the spectral-decay"
    " indicator for p-adaptive DG scheme"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the spectral-decay indicator used for
    p-adaptive discontinuous Galerkin (DG) discretization used in inciter.
    See Control/Inciter/Options/PrefIndicator.hpp for other valid options.)"; }
};
using pref_spectral_decay = keyword< pref_spectral_decay_info,
                                     TAOCPP_PEGTL_STRING("spectral_decay") >;

struct pref_non_conformity_info {
  static std::string name() { return "non-conformity"; }
  static std::string shortDescription() { return "Select the non-conformity"
    " indicator for p-adaptive DG scheme"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the non-conformity indicator used for
    p-adaptive discontinuous Galerkin (DG) discretization used in inciter.
    See Control/Inciter/Options/PrefIndicator.hpp for other valid options.)"; }
};
using pref_non_conformity = keyword< pref_non_conformity_info,
                                     TAOCPP_PEGTL_STRING("non_conformity") >;

struct pref_indicator_info {
  static std::string name() { return "the choice of adaptive indicator"; }
  static std::string shortDescription() { return "Configure the specific "
    " adaptive indicator for p-adaptive DG scheme"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a specific type of adaptive
    indicator for p-adaptive refinement  of the DG scheme. The keyword must
    be used in pref ... end block. Example specification: 'indicator 1'.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + pref_spectral_decay::string() + "\' | \'"
                  + pref_non_conformity::string() + '\'';
    }
  };
};
using pref_indicator =
          keyword< pref_indicator_info, TAOCPP_PEGTL_STRING("indicator") >;

struct pref_ndofmax_info {
  static std::string name() { return "Maximum ndof for p-refinement"; }
  static std::string shortDescription() { return "Configure the maximum "
    "number of degree of freedom for p-adaptive DG scheme"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a maximum number of degree of
    freedom for p-adaptive refinement  of the DG scheme. The keyword must
    be used in pref ... end block. Example specification: 'ndofmax 10'.)"; }
  struct expect {
    using type = std::size_t;
    static constexpr type lower = 4;
    static constexpr type upper = 10;
    static std::string description() { return "int"; }
    static std::string choices() {
      return "int either 4 or 10";
    }
  };
};
using pref_ndofmax =
          keyword< pref_ndofmax_info, TAOCPP_PEGTL_STRING("ndofmax") >;

struct pref_tolref_info {
  static std::string name() { return "Tolerance for p-refinement"; }
  static std::string shortDescription() { return "Configure the tolerance for "
    "p-refinement for the p-adaptive DG scheme"; }
  static std::string longDescription() { return
    R"(This keyword can be used to configure a tolerance for p-adaptive
    refinement  for the DG scheme. The keyword must be used in pref ... end
    block. All elements with a refinement indicator larger than this tolerance
    will be p-refined. Example specification: 'tolref 0.1'.)"; }
  struct expect {
    using type = tk::real;
    static constexpr type lower = 0.0;
    static constexpr type upper = 1.0;
    static std::string description() { return "real"; }
    static std::string choices() {
      return "real between [" + std::to_string(lower) + "..." +
             std::to_string(upper) + "] (both inclusive)";
    }
  };
};
using pref_tolref = keyword< pref_tolref_info, TAOCPP_PEGTL_STRING("tolref") >;

struct pref_info {
  static std::string name() { return "pref"; }
  static std::string shortDescription() { return
    "Start configuration block configuring p-adaptive refinement"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the pref ... end block, used to
    configure p-adaptive refinement. Keywords allowed
    in this block: )" + std::string("\'")
    + pref_indicator::string() + "\' | \'"
    + pref_ndofmax::string() + "\' | \'"
    + pref_tolref::string() + "\' | \'";
  }
};
using pref = keyword< pref_info, TAOCPP_PEGTL_STRING("pref") >;

struct diagcg_info {
  static std::string name() { return "CG+LW"; }
  static std::string shortDescription() { return "Select continuous Galerkin "
    "+ Lax Wendroff with a lumped-mass matrix LHS"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the lumped-mass matrix continuous Galerkin
    (CG) finite element spatial discretiztaion used in inciter. CG is combined
    with a Lax-Wendroff scheme for time discretization and flux-corrected
    transport (FCT) for treating discontinuous solutions. This option selects
    the scheme that stores the left-hand side matrix lumped, i.e., only the
    diagonal elements stored and thus does not require a linear solver. See
    Control/Inciter/Options/Scheme.hpp for other valid options.)"; }
};
using diagcg = keyword< diagcg_info, TAOCPP_PEGTL_STRING("diagcg") >;

struct alecg_info {
  static std::string name() { return "ALECG+RK"; }
  static std::string shortDescription() { return "Select continuous Galerkin "
    "with ALE + Runge-Kutta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the continuous Galerkin finite element
    scheme in the arbitrary Lagrangian-Eulerian (ALE) reference frame combined
    with Runge-Kutta (RK) time stepping. See Control/Inciter/Options/Scheme.hpp
    for other valid options.)"; }
};
using alecg = keyword< alecg_info, TAOCPP_PEGTL_STRING("alecg") >;

struct dg_info {
  static std::string name() { return "DG(P0)+RK"; }
  static std::string shortDescription() { return
    "Select 1st-order discontinuous Galerkin discretization + Runge-Kutta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the first-order accurate discontinuous
    Galerkin, DG(P0), spatial discretiztaion used in Inciter. As this is first
    order accurate, it is intended for testing and debugging purposes only.
    Selecting this spatial discretization also selects the Runge-Kutta scheme
    for time discretization. See Control/Inciter/Options/Scheme.hpp for other
    valid options.)"; }
};
using dg = keyword< dg_info, TAOCPP_PEGTL_STRING("dg") >;

struct p0p1_info {
  static std::string name() { return "P0P1+RK"; }
  static std::string shortDescription() { return
    "Select 2nd-order finite volume discretization + Runge-Kutta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the second-order accurate finite volume,
    P0P1, spatial discretiztaion used in Inciter. This method uses a
    least-squares procedure to reconstruct the second-order solution from the
    first-order one. Selecting this spatial discretization also selects the
    Runge-Kutta scheme for time discretization.
    See Control/Inciter/Options/Scheme.hpp for other valid options.)"; }
};
using p0p1 = keyword< p0p1_info, TAOCPP_PEGTL_STRING("p0p1") >;

struct dgp1_info {
  static std::string name() { return "DG(P1)+RK"; }
  static std::string shortDescription() { return
    "Select 2nd-order discontinuous Galerkin discretization + Runge-Kutta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the second-order accurate discontinuous
    Galerkin, DG(P1), spatial discretiztaion used in Inciter. Selecting this
    spatial discretization also selects the Runge-Kutta scheme for time
    discretization. See Control/Inciter/Options/Scheme.hpp for other
    valid options.)"; }
};
using dgp1 = keyword< dgp1_info, TAOCPP_PEGTL_STRING("dgp1") >;

struct dgp2_info {
  static std::string name() { return "DG(P2)+RK"; }
  static std::string shortDescription() { return
    "Select 3nd-order discontinuous Galerkin discretization + Runge-Kutta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the third-order accurate discontinuous
    Galerkin, DG(P2), spatial discretiztaion used in Inciter. Selecting this
    spatial discretization also selects the Runge-Kutta scheme for time
    discretization. See Control/Inciter/Options/Scheme.hpp for other
    valid options.)"; }
};
using dgp2 = keyword< dgp2_info, TAOCPP_PEGTL_STRING("dgp2") >;

struct pdg_info {
  static std::string name() { return "pDG+RK"; }
  static std::string shortDescription() { return
    "Select adaptive discontinuous Galerkin discretization + Runge-Kutta"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the adaptive discontinuous Galerkin
    spatial discretizaion used in Inciter. Selecting this spatial
    discretization also selects the Runge-Kutta scheme for time
    discretization. See Control/Inciter/Options/Scheme.hpp for other valid
    options.)"; }
};
using pdg = keyword< pdg_info, TAOCPP_PEGTL_STRING("pdg") >;

struct scheme_info {
  static std::string name() { return "Discretization scheme"; }
  static std::string shortDescription() { return
    "Select discretization scheme"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a spatial discretization scheme,
    necessarily connected to the teporal discretization scheme. See
    Control/Inciter/Options/Scheme.hpp for valid options.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + diagcg::string() + "\' | \'"
                  + dg::string() + '\'';
    }
  };
};
using scheme = keyword< scheme_info, TAOCPP_PEGTL_STRING("scheme") >;

struct laxfriedrichs_info {
  static std::string name() { return "Lax-Friedrichs"; }
  static std::string shortDescription() { return
    "Select Lax-Friedrichs flux function"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Lax-Friedrichs flux function used for
    discontinuous Galerkin (DG) spatial discretization used in inciter. See
    Control/Inciter/Options/Flux.hpp for other valid options.)"; }
};
using laxfriedrichs =
  keyword< laxfriedrichs_info, TAOCPP_PEGTL_STRING("laxfriedrichs") >;

struct hllc_info {
  static std::string name() { return "HLLC"; }
  static std::string shortDescription() { return
    "Select the Harten-Lax-van Leer-Contact (HLLC) flux function"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Harten-Lax-van Leer-Contact flux
    function used for discontinuous Galerkin (DG) spatial discretization
    used in inciter. See Control/Inciter/Options/Flux.hpp for other valid
    options.)"; }
};
using hllc = keyword< hllc_info, TAOCPP_PEGTL_STRING("hllc") >;

struct upwind_info {
  static std::string name() { return "Upwind"; }
  static std::string shortDescription() { return
    "Select the upwind flux function"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the upwind flux
    function used for discontinuous Galerkin (DG) spatial discretization
    used in inciter. It is really only useful for scalar transport, it is thus
    not selectable for anything else, and for scalar transport it is the
    hardcoded flux type. See Control/Inciter/Options/Flux.hpp for other valid
    options.)"; }
};
using upwind = keyword< upwind_info, TAOCPP_PEGTL_STRING("upwind") >;

struct ausm_info {
  static std::string name() { return "AUSM"; }
  static std::string shortDescription() { return
    "Select the Advection Upstream Splitting Method (AUSM) flux function"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the AUSM flux
    function used for discontinuous Galerkin (DG) spatial discretization
    used in inciter. It is only used for for multi-material hydro, it is thus
    not selectable for anything else, and for multi-material hydro it is the
    hardcoded flux type.)"; }
};
using ausm = keyword< ausm_info, TAOCPP_PEGTL_STRING("ausm") >;

struct hll_info {
  static std::string name() { return "HLL"; }
  static std::string shortDescription() { return
    "Select the Harten-Lax-vanLeer (HLL) flux function"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the HLL flux
    function used for discontinuous Galerkin (DG) spatial discretization
    used in inciter. It is only used for for multi-material hydro, it is thus
    not selectable for anything else, and for multi-material hydro it is the
    hardcoded flux type.)"; }
};
using hll = keyword< hll_info, TAOCPP_PEGTL_STRING("hll") >;

struct flux_info {
  static std::string name() { return "Flux function"; }
  static std::string shortDescription() { return
    "Select flux function"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a flux function, used for
    discontinuous Galerkin (DG) spatial discretization used in inciter. See
    Control/Inciter/Options/Flux.hpp for valid options.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + laxfriedrichs::string() + "\' | \'"
                  + hllc::string() + "\' | \'"
                  + upwind::string() + "\' | \'"
                  + ausm::string() + "\' | \'"
                  + hll::string() + '\'';
    }
  };
};
using flux = keyword< flux_info, TAOCPP_PEGTL_STRING("flux") >;

struct none_info {
  static std::string name() { return "none"; }
  static std::string shortDescription() { return "Select none option"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the 'none' option from a list of
    configuration options.)"; }
};
using none = keyword< none_info, TAOCPP_PEGTL_STRING("none") >;

struct fluid_info {
  static std::string name() { return "fluid"; }
  static std::string shortDescription() { return
    "Select the fluid velocity for ALE"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the 'fluid' velocity as the mesh velocity
       for Arbitrary-Lagrangian-Eulerian (ALE) mesh motion.)"; }
};
using fluid = keyword< fluid_info, TAOCPP_PEGTL_STRING("fluid") >;

struct helmholtz_info {
  static std::string name() { return "Helmholtz"; }
  static std::string shortDescription() { return
    "Select the Helmholtz velocity for ALE"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the a velocity, computed from the
       Helmholtz-decomposition as the mesh velocity for
       Arbitrary-Lagrangian-Eulerian (ALE) mesh motion. See J. Bakosi, J. Waltz,
       N. Morgan, Improved ALE mesh velocities for complex flows, Int. J. Numer.
       Meth. Fl., 1-10, 2017, https://doi.org/10.1002/fld.4403.)"; }
};
using helmholtz = keyword< helmholtz_info, TAOCPP_PEGTL_STRING("helmholtz") >;

struct meshvelocity_info {
  static std::string name() { return "Mesh velocity"; }
  static std::string shortDescription() { return
    "Select mesh velocity"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a mesh velocity option, used for
       Arbitrary-Lagrangian-Eulerian (ALE) mesh motion.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + none::string() + "\' | \'"
                  + fluid::string() + "\' | \'"
                  + helmholtz::string() + '\'';
    }
  };
};
using meshvelocity =
  keyword< meshvelocity_info, TAOCPP_PEGTL_STRING("mesh_velocity") >;

struct ale_info {
  static std::string name() { return "ALE"; }
  static std::string shortDescription() { return "Start configuration block "
    "configuring ALE"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce the ale ... end block, used to
    configure arbitrary Lagrangian-Eulerian (ALE) mesh movement. Keywords
    allowed in this block: )" + std::string("\'")
    + meshvelocity::string() + "\'.";
  }
};
using ale = keyword< ale_info, TAOCPP_PEGTL_STRING("ale") >;

struct filename_info {
  static std::string name() { return "filename"; }
  static std::string shortDescription() { return "Set filename"; }
  static std::string longDescription() { return
    R"(Set filename, e.g., mesh filename for solver coupling.)";
  }
  struct expect {
    using type = std::string;
    static std::string description() { return "string"; }
  };
};
using filename = keyword< filename_info, TAOCPP_PEGTL_STRING("filename") >;

struct location_info {
  static std::string name() { return "location"; }
  static std::string shortDescription() { return "Configure location"; }
  static std::string longDescription() { return
    R"(Configure location of a mesh relative to another, e.g., for solver
       coupling.)";
  }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using location = keyword< location_info, TAOCPP_PEGTL_STRING("location") >;

struct orientation_info {
  static std::string name() { return "orientation"; }
  static std::string shortDescription() { return "Configure orientation"; }
  static std::string longDescription() { return
    R"(Configure orientation of a mesh relative to another, e.g., for solver
       coupling.)";
  }
  struct expect {
    using type = tk::real;
    static std::string description() { return "real(s)"; }
  };
};
using orientation =
  keyword< orientation_info, TAOCPP_PEGTL_STRING("orientation") >;

struct mesh_info {
  static std::string name() { return "Mesh specification block"; }
  static std::string shortDescription() { return
    "Start configuration block assigning a mesh to a solver"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a mesh ... end block, used to
    assign and configure a mesh to a solver.)";
  }
};
using mesh = keyword< mesh_info, TAOCPP_PEGTL_STRING("mesh") >;

struct reference_info {
  static std::string name() { return "Mesh transformation"; }
  static std::string shortDescription() { return
    "Specify mesh transformation relative to a mesh of another solver"; }
  static std::string longDescription() { return
    R"(This keyword is used to specify a solver, given with a dependent
       variable, configured upstream in the input file, whose mesh is used as a
       reference to which the mesh being configured is transformed relative
       to.)";
  }
  struct expect {
    using type = char;
    static std::string description() { return "character"; }
  };
};
using reference = keyword< reference_info, TAOCPP_PEGTL_STRING("reference") >;

struct couple_info {
  static std::string name() { return "Couple solvers"; }
  static std::string shortDescription() { return
    "Specify coupling of solvers on different meshes"; }
  static std::string longDescription() { return
    R"(This keyword is used to introduce a couple ... end block, used to
       specify coupling of solvers operating on different meshes.)";
  }
};
using couple = keyword< couple_info, TAOCPP_PEGTL_STRING("couple") >;

struct nolimiter_info {
  static std::string name() { return "No limiter"; }
  static std::string shortDescription() { return
    "No limiter used"; }
  static std::string longDescription() { return
    R"(This keyword is used for discontinuous Galerkin (DG) spatial
    discretization without any limiter in inciter. See
    Control/Inciter/Options/Limiter.hpp for other valid options.)"; }
};
using nolimiter =
  keyword< nolimiter_info, TAOCPP_PEGTL_STRING("nolimiter") >;

struct wenop1_info {
  static std::string name() { return "WENOP1"; }
  static std::string shortDescription() { return
    "Select the Weighted Essentially Non-Oscillatory (WENO) limiter for DGP1"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Weighted Essentially Non-Oscillatory
    limiter used for discontinuous Galerkin (DG) P1 spatial discretization
    used in inciter. See Control/Inciter/Options/Limiter.hpp for other valid
    options.)"; }
};
using wenop1 = keyword< wenop1_info, TAOCPP_PEGTL_STRING("wenop1") >;

struct superbeep1_info {
  static std::string name() { return "SUPERBEEP1"; }
  static std::string shortDescription() { return
    "Select the Superbee limiter for DGP1"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the Superbee limiter used for
    discontinuous Galerkin (DG) P1 spatial discretization used in inciter.
    See Control/Inciter/Options/Limiter.hpp for other valid options.)"; }
};
using superbeep1 = keyword< superbeep1_info, TAOCPP_PEGTL_STRING("superbeep1") >;

struct vertexbasedp1_info {
  static std::string name() { return "VERTEXBASEDP1"; }
  static std::string shortDescription() { return
    "Select the vertex-based limiter for DGP1"; }
  static std::string longDescription() { return
    R"(This keyword is used to select the vertex-based limiter used for
    discontinuous Galerkin (DG) P1 spatial discretization used in inciter.
    Ref. Kuzmin, D. (2010). A vertex-based hierarchical slope limiter for
    p-adaptive discontinuous Galerkin methods. Journal of computational and
    applied mathematics, 233(12), 3077-3085.
    See Control/Inciter/Options/Limiter.hpp for other valid options.)"; }
};
using vertexbasedp1 = keyword< vertexbasedp1_info, TAOCPP_PEGTL_STRING("vertexbasedp1") >;

struct limiter_info {
  static std::string name() { return "Limiter function"; }
  static std::string shortDescription() { return
    "Select limiter function"; }
  static std::string longDescription() { return
    R"(This keyword is used to select a limiter function, used for
    discontinuous Galerkin (DG) spatial discretization used in inciter. See
    Control/Inciter/Options/Limiter.hpp for valid options.)"; }
  struct expect {
    static std::string description() { return "string"; }
    static std::string choices() {
      return '\'' + nolimiter::string() + "\' | \'"
                  + wenop1::string() + "\' | \'"
                  + superbeep1::string() + "\' | \'"
                  + vertexbasedp1::string() + '\'';
    }
  };
};
using limiter = keyword< limiter_info, TAOCPP_PEGTL_STRING("limiter") >;

struct fct_info {
  static std::string name() { return "Flux-corrected transport"; }
  static std::string shortDescription() { return
    "Turn flux-corrected transport on/off"; }
  static std::string longDescription() { return
    R"(This keyword can be used to turn on/off flux-corrected transport (FCT).
    Note that FCT is only used in conjunction with continuous Galerkin finite
    element discretization, configured by scheme diagcg and it has no
    effect when the discontinuous Galerkin (DG) scheme is used, configured by
    'scheme dg'. Also note that even if FCT is turned off, it is still
    performed, only its result is not applied.)"; }
  struct expect {
    using type = bool;
    static std::string description() { return "string"; }
    static std::string choices() { return "true | false"; }
  };
};
using fct = keyword< fct_info, TAOCPP_PEGTL_STRING("fct") >;

struct fctclip_info {
  static std::string name() { return "Clipping Flux-corrected transport"; }
  static std::string shortDescription() { return
    "Turn on clipping flux-corrected transport on/off"; }
  static std::string longDescription() { return
    R"(This keyword can be used to turn on/off the clipping limiter used for
    flux-corrected transport (FCT). The clipping limiter only looks at the
    current low order solution to determine the allowed solution minima and
    maxima, instead of the minimum and maximum of the low order solution and
    the previous solution.)"; }
  struct expect {
    using type = bool;
    static std::string description() { return "string"; }
    static std::string choices() { return "true | false"; }
  };
};
using fctclip = keyword< fctclip_info, TAOCPP_PEGTL_STRING("fctclip") >;

struct sysfct_info {
  static std::string name() { return "Flux-corrected transport for systems"; }
  static std::string shortDescription() { return
    "Turn on system nature of flux-corrected transport"; }
  static std::string longDescription() { return
    R"(This keyword can be used to enable a system-nature for flux-corrected
    transport (FCT). Note that FCT is only used in conjunction with continuous
    Galerkin finite element discretization, configured by scheme diagcg and it
    has no effect when the discontinuous Galerkin (DG) scheme is used,
    configured by 'scheme dg'. Enabling the system-nature for FCT will choose
    the limiter coefficients for a system of equations, e.g., compressible flow,
    in way that takes the system-nature of the equations into account. An
    example is assinging the minimum of the limit coefficient to all variables
    limited in a computational cell, e.g., density, momentum, and specitic total
    energy. This yields better, more monotonic, results.)"; }
  struct expect {
    using type = bool;
    static std::string description() { return "string"; }
    static std::string choices() { return "true | false"; }
  };
};
using sysfct = keyword< sysfct_info, TAOCPP_PEGTL_STRING("sysfct") >;

struct sysfctvar_info {
  static std::string name() { return "Variables considered for system FCT"; }
  static std::string shortDescription() { return
    "Specify a list of scalar component indices that considered for system FCT";
  }
  static std::string longDescription() { return
    R"(This keyword is used to specify a list of integers that are considered
    for computing the system-nature of flux-corrected transport. Example:
    'sysfctvar 0 1 2 3 end', which means ignoring the energy (by not listing 4)
    when computing the coupled limit coefficient for a system of mass, momentum,
    and energy for single-material compressible flow.)";
  }
  struct expect {
    using type = std::size_t;
    static std::string description() { return "integers"; }
  };
};
using sysfctvar = keyword< sysfctvar_info, TAOCPP_PEGTL_STRING("sysfctvar") >;

////////// NOT YET FULLY DOCUMENTED //////////

struct mix_iem_info {
  static std::string name() { return "IEM"; }
  static std::string shortDescription() { return
    "Interaction by exchange with the mean"; }
  static std::string longDescription() { return
    R"(Material mix model, 'mix_iem', is short for interaction by exchange with
    the mean (IEM). It is a relaxation-type material mix model intended
    shear-driven flows.)";
  }
};
using mix_iem = keyword<mix_iem_info,  TAOCPP_PEGTL_STRING("mix_iem") >;

struct mix_iecm_info {
  static std::string name() { return "IECM"; }
  static std::string shortDescription()
  { return "Interaction by exchange with the conditional mean"; }
  static std::string longDescription() { return
    R"(Material mix model, 'mix_iecm', is short for interaction by exchange with
    the conditional mean (IECM). It is a relaxation-type material mix model
    intended shear-driven flows.)";
  }
};
using mix_iecm = keyword<mix_iecm_info,  TAOCPP_PEGTL_STRING("mix_iecm") >;

struct mix_dir_info {
  static std::string name() { return "Dirichlet"; }
  static std::string shortDescription() { return "Dirichlet"; }
  static std::string longDescription() { return
    R"(Material mix model, 'mix_dir', is short for Dirichlet. It is a material
    mix model that explicitly satisfies the unit-sum requirement for all
    statistical samples.)";
  }
};
using mix_dir = keyword<mix_dir_info,  TAOCPP_PEGTL_STRING("mix_dir") >;

struct mix_gendir_info {
  static std::string name() { return "Generalized Dirichlet"; }
  static std::string shortDescription() { return "Generalized Dirichlet"; }
  static std::string longDescription() { return
    R"(Material mix model, 'mix_gendir', is short for Lochner's generalized
    Dirichlet. It is a material mix model that explicitly satisfies the
    unit-sum requirement for all statistical samples.)";
  }
};
using mix_gendir = keyword<mix_gendir_info,  TAOCPP_PEGTL_STRING("mix_gendir") >;

struct hommix_info {
  static std::string name() { return "HomMix"; }
  static std::string shortDescription()
  { return "Homogeneous material mixing"; }
  static std::string longDescription() { return
    R"(Physics option, 'hommix', is short for homogeneous material mixing. It is
    the simplest physics option that can be used to research, develop, and
    test material mixing models independent, i.e., decoupled from other
    equations. Only a set of scalar equations are advanced which can be
    coupled to each other. The keyword 'hommix' introduces the hommix ... end
    block, selecting and describing the parameters of the mixing model(s).
    The common physics keywords are recognized.)";
  }
};
using hommix = keyword<hommix_info, TAOCPP_PEGTL_STRING("hommix") >;

struct homhydro_info {
  static std::string name() { return "HomHydro"; }
  static std::string shortDescription() { return "Homogeneous hydrodynamics"; }
  static std::string longDescription() { return
    R"(Physics option, 'homhydro', is short for homogeneous hydrodynamics. It is
    the simplest physics option that can be used to research, develop, and
    test hydrodynamics models independent of, i.e., decoupled from other
    equations. Only a set of momentum equations are advanced whose components
    can be coupled to each other. The keyword 'homhydro' introduces the
    homhydro ... end block, selecting and describing the parameters of the
    hydrodynamics model(s). The common physics keywords are recognized.)";
  }
};
using homhydro = keyword<homhydro_info,  TAOCPP_PEGTL_STRING("homhydro") >;

struct homrt_info {
  static std::string name() { return "HomRT"; }
  static std::string shortDescription()
  { return "Homogeneous Rayleigh-Taylor"; }
  static std::string longDescription() { return
    R"(Physics option, 'homrt', is short for homogeneous Rayleigh-Taylor. It is
    the simplest physics option that can be used to research, develop, and
    test hydrodynamics models for variable-density hydrodynamics and coupled
    material mixing, independent, i.e., decoupled from other equations. Only
    a set of mass and momentum conservation equations are advanced whose
    components can be coupled to each other. The keyword 'homrt' introduces
    the homrt ... end block, selecting and describing the parameters of the
    mass and hydrodynamics model(s). The common physics keywords are
    recognized.)";
  }
};
using homrt = keyword<homrt_info,  TAOCPP_PEGTL_STRING("homrt") >;

struct spinsflow_info {
  static std::string name() { return "SPINSFlow"; }
  static std::string shortDescription() { return
    "Standalone-particle incompressible Navier-Stokes flow";
  }
  static std::string longDescription() { return
    R"(Physics option, 'spinsflow', is short for standalone-particle
    incompressible Navier-Stokes flow. It is a physics option intended for
    inhomogeneous constant-density flow. The transport equations solved are
    the momentum and optionally, energy, and a set of scalars. The
    divergence-constraint is enforced by solving a Poisson equation and
    projection scheme. The keyword 'spinsflow' introduces the spinsflow ...
    end block, selecting and describing the parameters of the above transport
    equations and their models. The common physics keywords are recognized.)";
  }
};
using spinsflow = keyword<spinsflow_info,  TAOCPP_PEGTL_STRING("spinsflow") >;

// This will go away once all the keywords below are documented
struct undefined_info {
  static std::string name() { return "undef"; }
  static std::string shortDescription() { return "undefined"; }
  static std::string longDescription() { return "Undefined."; }
};

} // kw::

#endif // Keywords_h

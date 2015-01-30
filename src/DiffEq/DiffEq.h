//******************************************************************************
/*!
  \file      src/DiffEq/DiffEq.h
  \author    J. Bakosi
  \date      Wed 28 Jan 2015 04:10:00 PM MST
  \copyright 2012-2015, Jozsef Bakosi.
  \brief     Differential equation
  \details   This file defines a generic differential equation class. The class
    uses runtime polymorphism without client-side inheritance: inheritance is
    confined to the internals of the class, inivisble to client-code. The class
    exclusively deals with ownership enabling client-side value semantics.
    Credit goes to Sean Parent at Adobe: https://github.com/sean-parent/
    sean-parent.github.com/wiki/Papers-and-Presentations.
*/
//******************************************************************************
#ifndef DiffEq_h
#define DiffEq_h

#include <string>
#include <functional>

#include <Types.h>
#include <make_unique.h>
#include <ParticleProperties.h>
#include <Statistics.h>

namespace walker {

//! \brief Differential equation
//! \details This class uses runtime polymorphism without client-side
//!   inheritance: inheritance is confined to the internals of the this class,
//!   inivisble to client-code. The class exclusively deals with ownership
//!   enabling client-side value semantics. Credit goes to Sean Parent at Adobe:
//!   https://github.com/sean-parent/sean-parent.github.com/wiki/
//!   Papers-and-Presentations. For example client code that models a DiffEq,
//!   see walker::Beta.
//! \author J. Bakosi
class DiffEq {

  public:
    //! \brief Constructor taking an object modeling Concept.
    //! \details The object of class T comes pre-constructed.
    //! \param[in] x Instantiated object of type T given by the template
    //!   argument.
    template< typename T > explicit DiffEq( T x ) :
      self( tk::make_unique< Model<T> >( std::move(x) ) ) {}

    //! \brief Constructor taking a function pointer to a constructor of an
    //!   object modeling Concept.
    //! \details Passing std::function allows late execution of the constructor,
    //!   i.e., as late as inside this class' constructor, and thus usage from
    //!   a factory. Note that there are at least two different ways of using
    //!   this constructor:
    //!   - Bind T's constructor arguments and place it in std::function<T()>
    //!   and passing no arguments as args.... This case then instantiates the
    //!   model via its constructor and stores it in here.
    //!   - Bind a single placeholder argument to T's constructor and pass it in
    //!   as host's args..., which then forwards it to model's constructor. This
    //!   allows late binding, i.e., binding the argument only here.
    //! \see See also the wrapper tk::recordModel() which does the former and
    //!   tk::recordModelLate() which does the latter, both defined in
    //!   src/Base/Factory.h.
    //! \param[in] x Function pointer to a constructor of an object modeling
    //!    Concept.
    //! \param[in] args... Constructor arguments
    template< typename T, typename...Args >
    explicit DiffEq( std::function<T(Args...)> x, Args... args ) :
      self( tk::make_unique< Model<T> >( std::move(x(args...)) ) ) {}

    //! Public interface to setting the initial conditions for the diff eq
    void initialize( tk::ParProps& particles, const tk::Statistics& stat ) const
    { self->initialize( particles, stat ); }

    //! Public interface to advancing particles in time by the diff eq
    void advance( tk::ParProps& particles, int stream, tk::real dt ) const
    { self->advance( particles, stream, dt ); }

    //! Copy assignment
    DiffEq& operator=( const DiffEq& x )
    { DiffEq tmp(x); *this = std::move(tmp); return *this; }
    //! Copy constructor
    DiffEq( const DiffEq& x ) : self( x.self->copy() ) {}
    //! Move assignment
    DiffEq& operator=( DiffEq&& ) noexcept = default;
    //! Move constructor
    DiffEq( DiffEq&& ) noexcept = default;

  private:
    //! Concept is a pure virtual base class specifying the requirements of
    //! polymorphic objects deriving from it
    struct Concept {
      virtual ~Concept() = default;
      virtual Concept* copy() const = 0;
      virtual void initialize( tk::ParProps&, const tk::Statistics& ) = 0;
      virtual void advance( tk::ParProps&, int, tk::real ) const = 0;
    };

    //! Model models the Concept above by deriving from it and overriding the
    //! the virtual functions required by Concept
    template< typename T >
    struct Model : Concept {
      Model( T x ) : data( std::move(x) ) {}
      Concept* copy() const { return new Model( *this ); }
      void initialize( tk::ParProps& particles, const tk::Statistics& stat )
        override { data.initialize( particles, stat ); }
      void advance( tk::ParProps& particles, int stream, tk::real dt ) const
        override { data.advance( particles, stream, dt ); }
      T data;
    };

    std::unique_ptr< Concept > self;    //!< Base pointer used polymorphically
};

} // walker::

#endif // DiffEq_h

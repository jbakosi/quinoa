//******************************************************************************
/*!
  \file      src/SDE/Hydro.h
  \author    J. Bakosi
  \date      Wed 01 Jan 2014 01:20:16 PM MST
  \copyright Copyright 2005-2012, Jozsef Bakosi, All rights reserved.
  \brief     Hydrodynamics models base
  \details   Hydro base
*/
//******************************************************************************
#ifndef Hydro_h
#define Hydro_h

#include <Model.h>

namespace quinoa {

//! Hydro model base
class Hydro : public Model {

  public:
    //! Constructor
    explicit Hydro() {}
//     explicit Hydro(const Base& base, tk::real* const particles) :
//         Model(base,
//               particles,
//               base.control.get<ctr::component, ctr::npar>(),
//               base.control.nprop()),
//         m_offset(base.control.velocityOffset()),
//         m_nvelocity(base.control.get<ctr::component, ctr::nvelocity>()) {
//         ErrChk(m_nvelocity > 0,
//                tk::ExceptType::FATAL,
//                "Wrong number of velocities");
//       }

    //! Destructor
    ~Hydro() noexcept override = default;

//     //! Initialize particles
//     virtual void init() = 0;
// 
//     //! Advance particles in hydro model
//     virtual void advance(int p, int tid, tk::real dt) = 0;
// 
//   protected:
//     const int m_offset;             //!< Velocity-offset relative to base
//     const int m_nvelocity;          //!< Number of particle velocities

  private:
    //! Don't permit copy constructor
    Hydro(const Hydro&) = delete;
    //! Don't permit copy assigment
    Hydro& operator=(const Hydro&) = delete;
    //! Don't permit move constructor
    Hydro(Hydro&&) = delete;
    //! Don't permit move assigment
    Hydro& operator=(Hydro&&) = delete;
};

} // quinoa::

#endif // Hydro_h
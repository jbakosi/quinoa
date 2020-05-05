// *****************************************************************************
/*!
  \file      src/PDE/MultiMat/Problem/InterfaceAdvection.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Problem configuration for the compressible flow equations
  \details   This file defines a Problem policy class for the compressible flow
    equations, defined in PDE/MultiMat/MultiMat.h. See PDE/MultiMat/Problem.h
    for general requirements on Problem policy classes for MultiMat.
*/
// *****************************************************************************

#include "InterfaceAdvection.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"
#include "EoS/EoS.hpp"
#include "MultiMat/MultiMatIndexing.hpp"
#include "FieldOutput.hpp"

using inciter::MultiMatProblemInterfaceAdvection;

tk::SolutionFn::result_type
MultiMatProblemInterfaceAdvection::solution( ncomp_t system,
                                             ncomp_t ncomp,
                                             tk::real x,
                                             tk::real y,
                                             tk::real /*z*/,
                                             tk::real t,
                                             int& )
// *****************************************************************************
//! Evaluate analytical solution at (x,y,z,t) for all components
//! \param[in] system Equation system index, i.e., which compressible
//!   flow equation system we operate on among the systems of PDEs
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] x X coordinate where to evaluate the solution
//! \param[in] y Y coordinate where to evaluate the solution
//! \param[in] t Time where to evaluate the solution
//! \return Values of all components evaluated at (x)
//! \note The function signature must follow tk::SolutionFn
// *****************************************************************************
{
  auto nmat =
    g_inputdeck.get< tag::param, eq, tag::nmat >()[system];

  // see also Control/Inciter/InputDeck/Grammar.hpp
  Assert( ncomp == 3*nmat+3, "Incorrect number of components in multi-material "
          "system" );

  std::vector< tk::real > s( ncomp, 0.0 );
  auto u = std::sqrt(50.0);
  auto v = std::sqrt(50.0);
  auto w = 0.0;
  auto alphamin = 1.0e-12;

  // center of the cylinder
  auto x0 = 0.45 + u*t;
  auto y0 = 0.45 + v*t;

  // radii of the material-rings
  std::vector< tk::real > r0(nmat, 0.0);
  r0[nmat-1] = 0.0;
  r0[nmat-2] = 0.1;
  r0[0] = 0.35;
  for (std::size_t k=1; k<nmat-2; ++k)
    r0[k] = r0[k-1] - (r0[0]-r0[nmat-2])
                      /(std::max( 1.0, static_cast<tk::real>(nmat-2)) );

  for (std::size_t k=0; k<nmat; ++k)
    s[volfracIdx(nmat, k)] = alphamin;

  // interface location
  auto r = std::sqrt( (x-x0)*(x-x0) + (y-y0)*(y-y0) );
  bool is_mat(false);
  for (std::size_t k=0; k<nmat-1; ++k)
  {
    if (r<r0[k] && r>=r0[k+1])
    {
      s[volfracIdx(nmat, k)] = 1.0 - static_cast<tk::real>(nmat-1)*alphamin;
      is_mat = true;
    }
  }
  if (!is_mat)
  {
    s[volfracIdx(nmat, nmat-1)] = 1.0 - static_cast<tk::real>(nmat-1)*alphamin;
  }

  auto rhob = 0.0;
  for (std::size_t k=0; k<nmat; ++k)
  {
    auto rhok = eos_density< eq >( system, 1.0e5, 300.0, k );
    s[densityIdx(nmat, k)] = s[volfracIdx(nmat, k)] * rhok;
    s[energyIdx(nmat, k)] = s[volfracIdx(nmat, k)]
      * eos_totalenergy< eq >( system, rhok, u, v, w, 1.0e5, k );
    rhob += s[densityIdx(nmat, k)];
  }
  s[momentumIdx(nmat, 0)] = rhob * u;
  s[momentumIdx(nmat, 1)] = rhob * v;
  s[momentumIdx(nmat, 2)] = rhob * w;

  return s;
}

std::vector< std::string >
MultiMatProblemInterfaceAdvection::fieldNames( ncomp_t )
// *****************************************************************************
// Return field names to be output to file
//! \return Vector of strings labelling fields output in file
// *****************************************************************************
{
  auto nmat =
    g_inputdeck.get< tag::param, eq, tag::nmat >()[0];

  return MultiMatFieldNames(nmat);
}

std::vector< std::vector< tk::real > >
MultiMatProblemInterfaceAdvection::fieldOutput(
  ncomp_t system,
  ncomp_t,
  ncomp_t offset,
  std::size_t nunk,
  tk::real /*t*/,
  tk::real,
  const std::vector< tk::real >&,
  const std::array< std::vector< tk::real >, 3 >& /*coord*/,
  tk::Fields& U,
  const tk::Fields& P )
// *****************************************************************************
//  Return field output going to file
//! \param[in] system Equation system index, i.e., which compressible
//!   flow equation system we operate on among the systems of PDEs
//! \param[in] offset System offset specifying the position of the system of
//!   PDEs among other systems
//! \param[in] nunk Number of unknowns to extract
//! \param[in] t Physical time
//! \param[in] coord Mesh node coordinates
//! \param[in] U Solution vector at recent time step
//! \param[in] P Vector of primitive quantities at recent time step
//! \return Vector of vectors to be output to file
// *****************************************************************************
{
  // number of degrees of freedom
  const std::size_t rdof =
    g_inputdeck.get< tag::discr, tag::rdof >();

  // number of materials
  auto nmat =
    g_inputdeck.get< tag::param, eq, tag::nmat >()[system];

  return MultiMatFieldOutput( system, nmat, offset, nunk, rdof, U, P );
}

std::vector< std::string >
MultiMatProblemInterfaceAdvection::names( ncomp_t )
// *****************************************************************************
//  Return names of integral variables to be output to diagnostics file
//! \return Vector of strings labelling integral variables output
// *****************************************************************************
{
  return { "r", "ru", "rv", "rw", "re" };
}

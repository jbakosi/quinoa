// *****************************************************************************
/*!
  \file      src/PDE/Integrate/MultiMatTerms.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Functions for computing volume integrals of multi-material terms
     using DG methods
  \details   This file contains functionality for computing volume integrals of
     non-conservative and pressure relaxation terms that appear in the
     multi-material hydrodynamic equations, using the discontinuous Galerkin
     method for various orders of numerical representation.
*/
// *****************************************************************************
#ifndef MultiMatTerms_h
#define MultiMatTerms_h

#include "Basis.hpp"
#include "Types.hpp"
#include "Fields.hpp"
#include "UnsMesh.hpp"

namespace tk {

using ncomp_t = kw::ncomp::info::expect::type;

//! Compute volume integrals of non-conservative terms for multi-material DG
void
nonConservativeInt( ncomp_t system,
                    std::size_t nmat,
                    ncomp_t offset,
                    const std::size_t ndof,
                    const std::size_t rdof,
                    const std::size_t nelem,
                    const std::vector< std::size_t >& inpoel,
                    const UnsMesh::Coords& coord,
                    const Fields& geoElem,
                    const Fields& U,
                    const Fields& P,
                    const std::vector< std::vector< tk::real > >& riemannDeriv,
                    const std::vector< std::vector< tk::real > >& vriempoly,
                    const std::vector< std::size_t >& ndofel,
                    Fields& R,
                    int intsharp );

//! Update the rhs by adding the non-conservative term integrals
void
updateRhsNonCons( ncomp_t ncomp,
                ncomp_t offset,
                const std::size_t nmat,
                const std::size_t ndof,
                const std::size_t ndof_el,
                const tk::real wt,
                const std::size_t e,
                const std::vector<tk::real>& B,
                const std::array< std::vector<tk::real>, 3 >& dBdx,
                const std::vector< std::vector< tk::real > >& ncf,
                Fields& R );

//! Compute volume integrals of pressure relaxation terms in multi-material DG
void
pressureRelaxationInt( ncomp_t system,
                       std::size_t nmat,
                       ncomp_t offset,
                       const std::size_t ndof,
                       const std::size_t rdof,
                       const std::size_t nelem,
                       const std::vector< std::size_t >& inpoel,
                       const UnsMesh::Coords& coord,
                       const Fields& geoElem,
                       const Fields& U,
                       const Fields& P,
                       const std::vector< std::size_t >& ndofel,
                       const tk::real ct,
                       Fields& R,
                       int intsharp );

//! Compute volume change due to pressure relaxation in the multi-material PDEs
std::vector< real >
getRelaxationVolumeChange(ncomp_t system,
  std::size_t ncomp,
  std::size_t nmat,
  const tk::real ct,
  const tk::real dx,
  const std::vector< real >& state);

//! Update the rhs by adding the pressure relaxation integrals
void
updateRhsPre(
  ncomp_t ncomp,
  ncomp_t offset,
  const std::size_t ndof,
  const std::size_t ndof_el,
  const tk::real wt,
  const std::size_t e,
  const std::vector< tk::real >& B,
  std::vector< tk::real >& ncf,
  Fields& R );

//! Solve the reconstruct velocity used for volume fraction equation
std::vector< std::vector< tk::real > >
solvevriem( std::size_t nelem,
            const std::vector< std::vector< tk::real > >& vriem,
            const std::vector< std::vector< tk::real > >& riemannLoc );

//! Compute the riemann velociry at the interface
void evaluRiemann( ncomp_t ncomp,
                   const int e_left,
                   const int e_right,
                   const std::size_t nmat,
                   const std::vector< tk::real >& fl,
                   const std::array< tk::real, 3 >& fn,
                   const std::array< tk::real, 3 >& gp,
                   const std::array< std::vector< tk::real >, 2 >& state,
                   std::vector< std::vector< tk::real > >& vriem,
                   std::vector< std::vector< tk::real > >& riemannLoc );
} // tk::

#endif // MultiMatTerms_h

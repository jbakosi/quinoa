// *****************************************************************************
/*!
  \file      src/PDE/Limiter.h
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Limiters for discontinous Galerkin methods
  \details   This file contains functions that provide limiter function
    calculations for maintaining monotonicity near solution discontinuities
    for the DG discretization.
*/
// *****************************************************************************
#ifndef Limiter_h
#define Limiter_h

#include "Types.h"
#include "Inciter/InputDeck/InputDeck.h"
#include "Fields.h"
#include "UnsMesh.h"
#include "DerivedData.h"
#include "Vector.h"
#include "Integrate/Quadrature.h"
#include "Integrate/Basis.h"

namespace inciter {

using ncomp_t = kw::ncomp::info::expect::type;

//! Weighted Essentially Non-Oscillatory (WENO) limiter for DGP1
void
WENO_P1( const std::vector< int >& esuel,
         inciter::ncomp_t offset,
         tk::Fields& U );

//! Superbee limiter for DGP1
void
Superbee_P1( const std::vector< int >& esuel,
             const std::vector< std::size_t >& inpoel,
             const std::vector< std::size_t >& ndofel,
             inciter::ncomp_t offset,
             const tk::UnsMesh::Coords& coord,
             tk::Fields& U );

} // inciter::

#endif // Limiter_h

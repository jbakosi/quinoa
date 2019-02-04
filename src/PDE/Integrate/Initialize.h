// *****************************************************************************
/*!
  \file      src/PDE/Integrate/Initialize.h
  \copyright 2016-2018, Los Alamos National Security, LLC.
  \brief     Functions for initialization of system of PDEs in DG methods
  \details   This file contains functionality for setting initial conditions
     and evaluating known solutions used in discontinuous Galerkin methods for
     various orders of numerical representation.
*/
// *****************************************************************************
#ifndef Initialize_h
#define Initialize_h

#include "Basis.h"
#include "Types.h"
#include "Fields.h"
#include "UnsMesh.h"
#include "FunctionPrototypes.h"

namespace tk {

//! Initalize a PDE system for DG
void
initialize( ncomp_t system,
            ncomp_t ncomp,
            ncomp_t offset,
            const Fields& L,
            const std::vector< std::size_t >& inpoel,
            const UnsMesh::Coords& coord,
            const SolutionFn& solution,
            Fields& unk,
            real t );

//! Update the rhs by adding the initial analytical solution term
void
update_rhs( ncomp_t ncomp,
            const std::size_t ndof,
            const tk::real wt,
            const std::vector< tk::real >& B,
            const std::vector< tk::real >& s,
            std::vector< tk::real >& R );


//! Compute the initial conditions
void
eval_init( ncomp_t ncomp,
           ncomp_t offset,
           const std::size_t ndof,
           const std::size_t e,
           const std::vector< tk::real >& R,
           const Fields& L,
           Fields& unk );

} // tk::

#endif // Initialize_h

// *****************************************************************************
/*!
  \file      src/Inciter/DiagCG.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     DiagCG for a PDE system with continuous Galerkin without a matrix
  \details   DiagCG advances a system of partial differential equations (PDEs)
    using continuous Galerkin (CG) finite element (FE) spatial discretization
    (using linear shapefunctions on tetrahedron elements) combined with a time
    stepping scheme that is equivalent to the Lax-Wendroff (LW) scheme within
    the unstructured-mesh FE context and treats discontinuities with
    flux-corrected transport (FCT). The left-hand side (lumped-mass) matrix
    is diagonal thus this scheme does not use a matrix-based linear solver.

    There are a potentially large number of DiagCG Charm++ chares created by
    Transporter. Each DiagCG gets a chunk of the full load (part of the mesh)
    and does the same: initializes and advances a number of PDE systems in time.

    The implementation uses the Charm++ runtime system and is fully
    asynchronous, overlapping computation and communication. The algorithm
    utilizes the structured dagger (SDAG) Charm++ functionality. The high-level
    overview of the algorithm structure and how it interfaces with Charm++ is
    discussed in the Charm++ interface file src/Inciter/diagcg.ci.
*/
// *****************************************************************************
#ifndef DiagCG_h
#define DiagCG_h

#include <vector>
#include <map>
#include <unordered_set>

#include "QuinoaConfig.hpp"
#include "Types.hpp"
#include "Fields.hpp"
#include "DerivedData.hpp"
#include "FluxCorrector.hpp"
#include "NodeDiagnostics.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"

#include "NoWarning/diagcg.decl.h"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

//! DiagCG Charm++ chare array used to advance PDEs in time with DiagCG+LW+FCT
class DiagCG : public CBase_DiagCG {

  public:
    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wunused-parameter"
      #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wunused-parameter"
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__INTEL_COMPILER)
      #pragma warning( push )
      #pragma warning( disable: 1478 )
    #endif
    // Include Charm++ SDAG code. See http://charm.cs.illinois.edu/manuals/html/
    // charm++/manual.html, Sec. "Structured Control Flow: Structured Dagger".
    DiagCG_SDAG_CODE
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic pop
    #elif defined(__INTEL_COMPILER)
      #pragma warning( pop )
    #endif

    //! Constructor
    explicit DiagCG( const CProxy_Discretization& disc,
                     const std::map< int, std::vector< std::size_t > >& /* bface */,
                     const std::map< int, std::vector< std::size_t > >& bnode,
                     const std::vector< std::size_t >& /* triinpoel */ );

    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
    #endif
    //! Migrate constructor
    // cppcheck-suppress uninitMemberVar
    explicit DiagCG( CkMigrateMessage* ) {}
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #endif

    //! Configure Charm++ custom reduction types initiated from this chare array
    static void registerReducers();

    //! Return from migration
    void ResumeFromSync() override;

    //! Size communication buffers (no-op)
    void resizeComm() {}

    //! Setup: query boundary conditions, output mesh, etc.
    void setup();

    // Initially compute left hand side diagonal matrix
    void init();

    //! Advance equations to next time step
    void advance( tk::real newdt );

    //! Compute left-hand side of transport equations
    void lhs();

    //! Receive contributions to left-hand side matrix on chare-boundaries
    void comlhs( const std::vector< std::size_t >& gid,
                 const std::vector< std::vector< tk::real > >& L );

    //! Receive contributions to right-hand side vector on chare-boundaries
    void comrhs( const std::vector< std::size_t >& gid,
                 const std::vector< std::vector< tk::real > >& R,
                 const std::vector< std::vector< tk::real > >& D );

    //! Update solution at the end of time step
    void update( const tk::Fields& a, tk::Fields&& dul );

    //! Optionally refine/derefine mesh
    void refine();

    //! Receive new mesh from refiner
    void resizePostAMR(
      const std::vector< std::size_t >& ginpoel,
      const tk::UnsMesh::Chunk& chunk,
      const tk::UnsMesh::Coords& coord,
      const std::unordered_map< std::size_t, tk::UnsMesh::Edge >& addedNodes,
      const std::unordered_map< std::size_t, std::size_t >& addedTets,
      const std::unordered_map< int, std::vector< std::size_t > >& msum,
      const std::map< int, std::vector< std::size_t > >& /* bface */,
      const std::map< int, std::vector< std::size_t > >& bnode,
      const std::vector< std::size_t >& /* triinpoel */ );

    //! Const-ref access to current solution
    //! \return Const-ref to current solution
    const tk::Fields& solution() const { return m_u; }

    //! Resizing data sutrctures after mesh refinement has been completed
    void resized();

    //! Evaluate whether to continue with next time step
    void step();

    /** @name Charm++ pack/unpack serializer member functions */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er &p ) override {
      p | m_disc;
      p | m_initial;
      p | m_nsol;
      p | m_nlhs;
      p | m_nrhs;
      p | m_bnode;
      p | m_u;
      p | m_ul;
      p | m_du;
      p | m_ue;
      p | m_lhs;
      p | m_rhs;
      p | m_bc;
      p | m_lhsc;
      p | m_rhsc;
      p | m_difc;
      p | m_vol;
      p | m_diag;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] i DiagCG object reference
    friend void operator|( PUP::er& p, DiagCG& i ) { i.pup(p); }
    //@}

  private:
    using ncomp_t = kw::ncomp::info::expect::type;

    //! Discretization proxy
    CProxy_Discretization m_disc;
    //! 1 if starting time stepping, 0 if during time stepping
    int m_initial;
    //! Counter for high order solution vector nodes updated
    std::size_t m_nsol;
    //! Counter for left-hand side matrix (vector) nodes updated
    std::size_t m_nlhs;
    //! Counter for right-hand side vector nodes updated
    std::size_t m_nrhs;
    //! Boundary node lists mapped to side set ids
    std::map< int, std::vector< std::size_t > > m_bnode;
    //! Unknown/solution vector at mesh nodes
    tk::Fields m_u;
    //! Unknown/solution vector at mesh nodes (low orderd)
    tk::Fields m_ul;
    //! Unknown/solution vector increment (high order)
    tk::Fields m_du;
    //! Unknown/solution vector at mesh cells
    tk::Fields m_ue;
    //! Lumped lhs mass matrix
    tk::Fields m_lhs;
    //! Right-hand side vector (for the high order system)
    tk::Fields m_rhs;
    //! Boundary conditions evaluated and assigned to mesh node IDs
    //! \details Vector of pairs of bool and boundary condition value associated
    //!   to meshnode IDs at which the user has set Dirichlet boundary
    //!   conditions for all PDEs integrated. The bool indicates whether the BC
    //!   is set at the node for that component the if true, the real value is
    //!   the increment (from t to dt) in the BC specified for a component.
    std::unordered_map< std::size_t,
      std::vector< std::pair< bool, tk::real > > > m_bc;
    //! Receive buffer for communication of the left hand side
    //! \details Key: chare id, value: lhs for all scalar components per node
    std::unordered_map< std::size_t, std::vector< tk::real > > m_lhsc;
    //! Receive buffer for communication of the right hand side
    //! \details Key: chare id, value: rhs for all scalar components per node
    std::unordered_map< std::size_t, std::vector< tk::real > > m_rhsc;
    //! Receive buffer for communication of mass diffusion on the hand side
    //! \details Key: chare id, value: dif for all scalar components per node
    std::unordered_map< std::size_t, std::vector< tk::real > > m_difc;
    //! Total mesh volume
    tk::real m_vol;
    //! Diagnostics object
    NodeDiagnostics m_diag;

    //! Access bound Discretization class pointer
    Discretization* Disc() const {
      Assert( m_disc[ thisIndex ].ckLocal() != nullptr, "ckLocal() null" );
      return m_disc[ thisIndex ].ckLocal();
    }

    //! Output mesh fields to files
    void out();

    //! Output mesh-based fields to file
    void writeFields( CkCallback c ) const;

    //! The own and communication portion of the left-hand side is complete
    void lhsdone();

    //! Combine own and communicated contributions to left hand side
    void lhsmerge();

    //! Compute righ-hand side vector of transport equations
    void rhs();

    //! Start time stepping
    void start();

    //! Solve low and high order diagonal systems
    void solve( tk::Fields& dif );

    //! Compute time step size
    void dt();
};

} // inciter::

#endif // DiagCG_h
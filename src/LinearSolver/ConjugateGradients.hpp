// *****************************************************************************
/*!
  \file      src/LinearSolver/ConjugateGradients.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Charm++ chare array for distributed conjugate gradients
  \details   Charm++ chare array for asynchronous distributed
    conjugate gradients linear solver.

    There are a potentially large number of ConjugateGradients Charm++ chares.
    Each ConjugateGradient chare gets a chunk of the full load (due to partiting
    the mesh on which the solve is performed.

    The implementation uses the Charm++ runtime system and is fully
    asynchronous, overlapping computation and communication. The algorithm
    utilizes the structured dagger (SDAG) Charm++ functionality.
*/
// *****************************************************************************
#ifndef ConjugateGradients_h
#define ConjugateGradients_h

#include "Types.hpp"
#include "CSR.hpp"

#include "NoWarning/conjugategradients.decl.h"

namespace tk {

//! \brief ConjugateGradients Charm++ chare array used to perform a distributed
//!   linear solve with the conjugate gradients algorithm
class ConjugateGradients : public CBase_ConjugateGradients {

  public:
    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wunused-parameter"
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wunused-parameter"
    #endif
    // Include Charm++ SDAG code. See http://charm.cs.illinois.edu/manuals/html/
    // charm++/manual.html, Sec. "Structured Control Flow: Structured Dagger".
    ConjugateGradients_SDAG_CODE
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic pop
    #endif

    //! Constructor
    explicit ConjugateGradients(
      const CSR& A,
      const std::vector< tk::real >& x,
      const std::vector< tk::real >& b,
      std::size_t maxit,
      tk::real stop_tol,
      const std::vector< std::size_t >& gid,
      const std::unordered_map< std::size_t, std::size_t >& lid,
      const NodeCommMap& nodecommmap );

    //! Migrate constructor
    //explicit ConjugateGradients( CkMigrateMessage* ) {}

    //! Initialize solver
    void init( CkCallback c );

    //! Solve linear system
    void solve( CkCallback c );

    //! Compute the norm of the right hand side
    void normb( tk::real n );

    //! Compute rho = (r,r)
    void rho( tk::real r );

    //! Receive contributions to r = b - A * x on chare-boundaries
    void comres( const std::vector< std::size_t >& gid,
                 const std::vector< std::vector< tk::real > >& rc );

    //! Receive contributions to q = A * p on chare-boundaries
    void comq( const std::vector< std::size_t >& gid,
               const std::vector< std::vector< tk::real > >& qc );

    //! Compute the dot product (p,q)
    void pq( tk::real n );

    //! Compute the norm of the residual: (r,r)
    void normres( tk::real r );

    /** @name Pack/unpack (Charm++ serialization) routines */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er &p ) override {
      p | m_A;
      p | m_x;
      p | m_b;
      p | m_gid;
      p | m_lid;
      p | m_nodeCommMap;
      p | m_r;
      p | m_rc;
      p | m_nr;
      p | m_p;
      p | m_q;
      p | m_qc;
      p | m_nq;
      p | m_initialized;
      p | m_solved;
      p | m_normb;
      p | m_it;
      p | m_maxit;
      p | m_tol;
      p | m_rho;
      p | m_rho0;
      p | m_alpha;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] c ConjugateGradients object reference
    friend void operator|( PUP::er& p, ConjugateGradients& c ) { c.pup(p); }
    ///@}

  private:
    //! Sparse matrix
    CSR m_A;
    //! Solution/unknown
    std::vector< tk::real > m_x;
    //! Right hand side
    std::vector< tk::real > m_b;
    //! Global node IDs
    std::vector< std::size_t > m_gid;
    //! Local node IDs associated to global ones
    std::unordered_map< std::size_t, std::size_t > m_lid;
    //! Global mesh node IDs shared with other chares associated to chare IDs
    NodeCommMap m_nodeCommMap;
    //! Auxiliary vector for CG solve
    std::vector< tk::real > m_r;
    //! Receive buffer for communication of r = b - A * x
    std::unordered_map< std::size_t, std::vector< tk::real > > m_rc;
    //! Counter for assembling m_r
    std::size_t m_nr;
    //! Auxiliary vector for CG solve
    std::vector< tk::real > m_p;
    //! Auxiliary vector for CG solve
    std::vector< tk::real > m_q;
    //! Receive buffer for communication of q = A * p
    std::unordered_map< std::size_t, std::vector< tk::real > > m_qc;
    //! Counter for assembling m_q
    std::size_t m_nq;
    //! Charm++ callback to continue with when the initialization is complete
    CkCallback m_initialized;
    //! Charm++ callback to continue with when the solve is complete
    CkCallback m_solved;
    //! L2 norm of the right hand side
    tk::real m_normb;
    //! Iteration count
    std::size_t m_it;
    //! Max iteration count
    std::size_t m_maxit;
    //! Stop tolerance
    tk::real m_tol;
    //! Helper scalar for CG algorithm
    tk::real m_rho;
    //! Helper scalar for CG algorithm
    tk::real m_rho0;
    //! Helper scalar for CG algorithm
    tk::real m_alpha;

    //! Initiate computationa of dot product of two vectors
    void dot( const std::vector< tk::real >& a,
              const std::vector< tk::real >& b,
              CkCallback c );

    //! Initiate A * x for computing the initial residual, r = b - A * x
    void residual();
    //! Compute the initial residual, r = b - A * x
    void initres();

    //! Initiate computing q = A * p
    void qAp();
    //! Finish computing q = A * p
    void q();

    //! Start next linear solver iteration
    void next();
};

} // tk::

#endif // ConjugateGradients_h

// *****************************************************************************
/*!
  \file      src/Inciter/FluxCorrector.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     FluxCorrector performs limiting for transport equations
  \details   FluxCorrector performs limiting for transport equations. Each
    FluxCorrector object performs the limiting procedure, according to a
    flux-corrected transport algorithm, on a chunk of the full load (part of the
    mesh).
*/
// *****************************************************************************
#ifndef FluxCorrector_h
#define FluxCorrector_h

#include <utility>
#include <unordered_map>

#include "NoWarning/pup.hpp"

#include "Keywords.hpp"
#include "Fields.hpp"
#include "Vector.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

//! FluxCorrector is used to perform flux-corrected transport
//! \see Löhner, R., Morgan, K., Peraire, J. and Vahdati, M. (1987), Finite
//!   element flux-corrected transport (FEM–FCT) for the Euler and Navier–Stokes
//!   equations. Int. J. Numer. Meth. Fluids, 7: 1093–1109.
//!   doi:10.1002/fld.1650071007
class FluxCorrector {

  private:
    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Constructor
    //! \param[in] is Size of the mesh element connectivity vector (inpoel size)
    explicit FluxCorrector( std::size_t is = 0 ) :
      m_aec( is, g_inputdeck.get< tag::component >().nprop() ),
      m_sys( findsys< tag::compflow >() ),
      m_vel( findvel< tag::compflow >() ) {}

    //! Collect scalar comonent indices for equation systems
    //! \tparam Eq Equation types to consider as equation systems
    //! \return List of component indices to treat as a system
    template< class... Eq >
    std::vector< std::vector< ncomp_t > >
    findsys() {
      std::vector< std::vector< ncomp_t > > sys;
      ( ... , [&](){
        // Access system-FCT variable indices for all systems of type Eq
        const auto& sv = g_inputdeck.get< tag::param, Eq, tag::sysfctvar >();
        // Access number of scalar components in all systems of type Eq
        const auto& ncompv = g_inputdeck.get< tag::component >().get< Eq >();
        // Assign variable indices for system FCT for each Eq system
        for (std::size_t e=0; e<ncompv.size(); ++e) {
          if (g_inputdeck.get< tag::param, Eq, tag::sysfct >().at(e)) {
            auto offset = g_inputdeck.get< tag::component >().offset< Eq >( e );
            sys.push_back( std::vector< ncomp_t >() );
            for (auto c : sv.at(e)) {
              sys.back().push_back( offset + c );
            }
          }
        } }() );
      for ([[maybe_unused]] const auto& s : sys) {
        Assert( std::all_of( begin(s), end(s), [&]( std::size_t i ){
                  return i < g_inputdeck.get< tag::component >().nprop(); } ),
                "Eq system index larger than total number of components" );
      }
      return sys;
    }

    //! Find components of a velocity for equation systems
    //! \tparam Eq Equation types to consider as equation systems
    //! \return List of 3 component indices to treat as a velocity
    //! \warning Currently, this is only a punt for single-material flow: we
    //!   simply take the components 1,2,3 as the velocity for each system of
    //!   type Eq
    template< class... Eq >
    std::vector< std::array< ncomp_t, 3 > >
    findvel() {
      std::vector< std::array< ncomp_t, 3 > > vel;
      ( ... , [&](){
        // Access number of scalar components in all systems of type Eq
        const auto& ncompv = g_inputdeck.get< tag::component >().get< Eq >();
        // Assign variable indices for system FCT for each Eq system
        for (std::size_t e=0; e<ncompv.size(); ++e) {
          auto offset = g_inputdeck.get< tag::component >().offset< Eq >( e );
          vel.push_back( { offset+1, offset+2, offset+3 } );
        } }() );
      for ([[maybe_unused]] const auto& v : vel) {
        Assert( std::all_of( begin(v), end(v), [&]( std::size_t i ){
                  return i < g_inputdeck.get< tag::component >().nprop(); } ),
                "Eq system index larger than total number of components" );
      }
      return vel;
    }

    //! Resize state (e.g., after mesh refinement)
    void resize( std::size_t is ) {
      m_aec.resize( is, g_inputdeck.get< tag::component >().nprop() );
    }

    //! Compute antidiffusive element contributions (AEC)
    void aec(
      const std::array< std::vector< tk::real >, 3 >& coord,
      const std::vector< std::size_t >& inpoel,
      const std::vector< tk::real >& vol,
      const std::unordered_map< std::size_t,
              std::vector< std::pair< bool, tk::real > > >& bc,
      const std::unordered_map< std::size_t, std::array< tk::real, 4 > >& bnorm,
      const tk::Fields& Un,
      tk::Fields& P );

    //! Verify the assembled antidiffusive element contributions
    bool verify( std::size_t nchare,
                 const std::vector< std::size_t >& inpoel,
                 const tk::Fields& dUh,
                 const tk::Fields& dUl ) const;

    //! Compute lumped mass matrix lhs for low order system
    tk::Fields lump( const std::array< std::vector< tk::real >, 3 >& coord,
                     const std::vector< std::size_t >& inpoel ) const;

    //! Compute mass diffusion contribution to the rhs of the low order system
    void diff( const std::array< std::vector< tk::real >, 3 >& coord,
               const std::vector< std::size_t >& inpoel,
               const std::vector< std::size_t >& bndel,
               const std::vector< std::size_t >& gid,
               const std::unordered_map< std::size_t, std::size_t >& bid,
               const tk::Fields& Un,
               tk::Fields& D ) const;

    //! \brief Compute the maximum and minimum unknowns of all elements
    //!   surrounding nodes
    void alw( const std::vector< std::size_t >& inpoel,
              const tk::Fields& Un,
              const tk::Fields& Ul,
              tk::Fields& Q ) const;

    //! Compute limited antiffusive element contributions and apply to mesh nodes
    void lim( const std::vector< std::size_t >& inpoel,
              const std::unordered_map< std::size_t,
                      std::vector< std::pair< bool, tk::real > > >& bcdir,
              const tk::Fields& P,
              const tk::Fields& Ul,
              tk::Fields& Q,
              tk::Fields& A ) const;

    // Collect mesh output fields from FCT
    std::tuple< std::vector< std::string >,
                std::vector< std::vector< tk::real > > >
    fields( const std::vector< std::size_t >& inpoel ) const;

    /** @name Charm++ pack/unpack serializer member functions */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er& p ) {
      p | m_aec;
      p | m_sys;
      p | m_vel;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] i FluxCorrector object reference
    friend void operator|( PUP::er& p, FluxCorrector& i ) { i.pup(p); }
    //@}

  private:
   //! Antidiffusive element contributions for all scalar components
   tk::Fields m_aec;
   //! Component indices to treat as a system for multiple systems
   std::vector< std::vector< ncomp_t > > m_sys;
   //! Component indices to treat as a velocity vector for multiple systems
   std::vector< std::array< ncomp_t, 3 > > m_vel;

   //! Compute mass diffusion contribution to the RHS of the low order system
   //! \tparam Op Operation to specify boundary vs internal nodes contribution
   //! \param[in] coord Mesh node coordinates
   //! \param[in] inpoel Mesh element connectivity
   //! \param[in] gid Local->global node id map
   //! \param[in] bid Local chare-boundary node ids (value) associated to
   //!    global node ids (key)
   //! \param[in] Un Solution at the previous time step
   template< class Op >
   void diffusion( std::size_t e,
                   const std::array< std::vector< tk::real >, 3 >& coord,
                   const std::vector< std::size_t >& inpoel,
                   const std::vector< std::size_t >& gid,
                   const std::unordered_map< std::size_t, std::size_t >& bid,
                   const tk::Fields& Un,
                   tk::Fields& D,
                   Op op ) const
   {
     const std::array< std::size_t, 4 >
       N{{ inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] }};

     const auto& x = coord[0];
     const auto& y = coord[1];
     const auto& z = coord[2];

     auto ncomp = g_inputdeck.get< tag::component >().nprop();
     auto ctau = g_inputdeck.get< tag::discr, tag::ctau >();

     // compute element Jacobi determinant
     const std::array< tk::real, 3 >
       ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
       ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
       da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
     const auto J = tk::triple( ba, ca, da );   // J = 6V
     Assert( J > 0, "Element Jacobian non-positive" );

     // lumped - consistent mass
     std::array< std::array< tk::real, 4 >, 4 > m;       // nnode*nnode [4][4]
     m[0][0] = m[1][1] = m[2][2] = m[3][3] = 3.0*J/120.0;// diagonal
     m[0][1] = m[0][2] = m[0][3] =                       // off-diagonal
     m[1][0] = m[1][2] = m[1][3] =
     m[2][0] = m[2][1] = m[2][3] =
     m[3][0] = m[3][1] = m[3][2] = -J/120.0;

     // access solution at element nodes at time n
     std::vector< std::array< tk::real, 4 > > un( ncomp );
     for (ncomp_t c=0; c<ncomp; ++c) un[c] = Un.extract( c, 0, N );
     // access pointer to mass diffusion right hand side at element nodes
     std::vector< const tk::real* > d( ncomp );
     for (ncomp_t c=0; c<ncomp; ++c) d[c] = D.cptr( c, 0 );

     // scatter-add mass diffusion element contributions to rhs nodes
     for (std::size_t a=0; a<4; ++a) {
       if (op( bid.find(gid[N[a]]), end(bid) ))
         for (ncomp_t c=0; c<ncomp; ++c)
           for (std::size_t b=0; b<4; ++b)
             D.var(d[c],N[a]) -= ctau * m[a][b] * un[c][b];
     }
   }
};

} // inciter::

#endif // FluxCorrector_h

// *****************************************************************************
/*!
  \file      src/PDE/CompFlow/CGCompFlow.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Compressible single-material flow using continuous Galerkin
  \details   This file implements the physics operators governing compressible
    single-material flow using continuous Galerkin discretization.
*/
// *****************************************************************************
#ifndef CGCompFlow_h
#define CGCompFlow_h

#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "DerivedData.hpp"
#include "Exception.hpp"
#include "Vector.hpp"
#include "EoS/EoS.hpp"
#include "Mesh/Around.hpp"
#include "Reconstruction.hpp"
#include "Problem/FieldOutput.hpp"
#include "Problem/BoxInitialization.hpp"
#include "Riemann/Rusanov.hpp"
#include "NodeBC.hpp"
#include "EoS/EoS.hpp"
#include "History.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

namespace cg {

//! \brief CompFlow used polymorphically with tk::CGPDE
//! \details The template arguments specify policies and are used to configure
//!   the behavior of the class. The policies are:
//!   - Physics - physics configuration, see PDE/CompFlow/Physics.h
//!   - Problem - problem configuration, see PDE/CompFlow/Problems.h
//! \note The default physics is Euler, set in inciter::deck::check_compflow()
template< class Physics, class Problem >
class CompFlow {

  private:
    using ncomp_t = kw::ncomp::info::expect::type;
    using eq = tag::compflow;
    using real = tk::real;
    using param = tag::param;

    static constexpr std::size_t m_ncomp = 5;
    static constexpr real muscl_eps = 1.0e-9;
    static constexpr real muscl_const = 1.0/3.0;
    static constexpr real muscl_m1 = 1.0 - muscl_const;
    static constexpr real muscl_p1 = 1.0 + muscl_const;

  public:
    //! \brief Constructor
    //! \param[in] c Equation system index (among multiple systems configured)
    explicit CompFlow( ncomp_t c ) :
      m_physics(),
      m_problem(),
      m_system( c ),
      m_offset( g_inputdeck.get< tag::component >().offset< eq >(c) ),
      m_stagCnf( g_inputdeck.specialBC< eq, tag::bcstag >( c ) ),
      m_skipCnf( g_inputdeck.specialBC< eq, tag::bcskip >( c ) ),
      m_fr( g_inputdeck.get< param, eq, tag::farfield_density >().size() > c ?
            g_inputdeck.get< param, eq, tag::farfield_density >()[c] : 1.0 ),
      m_fp( g_inputdeck.get< param, eq, tag::farfield_pressure >().size() > c ?
            g_inputdeck.get< param, eq, tag::farfield_pressure >()[c] : 1.0 ),
      m_fu( g_inputdeck.get< param, eq, tag::farfield_velocity >().size() > c ?
            g_inputdeck.get< param, eq, tag::farfield_velocity >()[c] :
            std::vector< real >( 3, 0.0 ) )
    {
      Assert( g_inputdeck.get< tag::component >().get< eq >().at(c) == m_ncomp,
       "Number of CompFlow PDE components must be " + std::to_string(m_ncomp) );
    }

    //! Determine nodes that lie inside the user-defined IC box
    //! \param[in] coord Mesh node coordinates
    //! \param[in,out] inbox List of nodes at which box user ICs are set
    void IcBoxNodes( const tk::UnsMesh::Coords& coord,
      std::unordered_set< std::size_t >& inbox ) const
    {
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // Detect if user has configured a box IC
      const auto& ic = g_inputdeck.get< tag::param, eq, tag::ic >();
      const auto& icbox = ic.get< tag::box >();
      std::vector< tk::real >
        box{ icbox.get< tag::xmin >(), icbox.get< tag::xmax >(),
             icbox.get< tag::ymin >(), icbox.get< tag::ymax >(),
             icbox.get< tag::zmin >(), icbox.get< tag::zmax >() };
      const auto eps = std::numeric_limits< tk::real >::epsilon();

      // Determine which nodes lie in the IC box
      if ( std::any_of( begin(box), end(box), [=](auto p)
        {return abs(p) > eps;} )) {
        for (ncomp_t i=0; i<x.size(); ++i) {
          if ( x[i]>box[0] && x[i]<box[1] && y[i]>box[2] && y[i]<box[3] &&
            z[i]>box[4] && z[i]<box[5] )
          {
            inbox.insert( i );
          }
        }
      }
    }

    //! Initalize the compressible flow equations, prepare for time integration
    //! \param[in] coord Mesh node coordinates
    //! \param[in,out] unk Array of unknowns
    //! \param[in] t Physical time
    //! \param[in] V Discrete volume of user-defined IC box
    //! \param[in] inbox List of nodes at which box user ICs are set
    void initialize( const std::array< std::vector< real >, 3 >& coord,
                     tk::Fields& unk,
                     real t,
                     real V,
                     const std::unordered_set< std::size_t >& inbox ) const
    {
      Assert( coord[0].size() == unk.nunk(), "Size mismatch" );

      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      const auto& ic = g_inputdeck.get< tag::param, eq, tag::ic >();
      const auto& icbox = ic.get< tag::box >();
      std::array< tk::real, 6 >
        boxdim{ icbox.get< tag::xmin >(), icbox.get< tag::xmax >(),
                icbox.get< tag::ymin >(), icbox.get< tag::ymax >(),
                icbox.get< tag::zmin >(), icbox.get< tag::zmax >() };
      auto V_ex = (boxdim[1]-boxdim[0]) * (boxdim[3]-boxdim[2]) *
        (boxdim[5]-boxdim[4]);
      const auto eps = 1000.0 * std::numeric_limits< tk::real >::epsilon();
      // if an ic box was not specified, avoid division by zero by setting V
      if (V_ex < eps) V = 1.0;
      const auto& bgpreic = ic.get< tag::pressure >();
      const auto& cv = g_inputdeck.get< tag::param, eq, tag::cv >();

      // Set initial and boundary conditions using problem policy
      for (ncomp_t i=0; i<x.size(); ++i) {
        auto s = Problem::initialize( m_system, m_ncomp, x[i], y[i], z[i], t );

        // initialize the user-defined box IC
        if (inbox.find(i) != inbox.end())
          initializeBox(m_system, V_ex/V, t, icbox, bgpreic, cv, s);

        unk(i,0,m_offset) = s[0]; // rho
        if (!skipPoint(x[i],y[i],z[i]) && stagPoint(x[i],y[i],z[i])) {
          unk(i,1,m_offset) = unk(i,2,m_offset) = unk(i,3,m_offset) = 0.0;
        } else {
          unk(i,1,m_offset) = s[1]; // rho * u
          unk(i,2,m_offset) = s[2]; // rho * v
          unk(i,3,m_offset) = s[3]; // rho * w
        }
        unk(i,4,m_offset) = s[4]; // rho * e, e: total = kinetic + internal
      }
    }

    //! Return analytic solution (if defined by Problem) at xi, yi, zi, t
    //! \param[in] xi X-coordinate
    //! \param[in] yi Y-coordinate
    //! \param[in] zi Z-coordinate
    //! \param[in] t Physical time
    //! \return Vector of analytic solution at given location and time
    std::vector< real >
    analyticSolution( real xi, real yi, real zi, real t ) const
    { return Problem::analyticSolution( m_system, m_ncomp, xi, yi, zi, t ); }

    //! Return analytic solution for conserved variables
    //! \param[in] xi X-coordinate at which to evaluate the analytic solution
    //! \param[in] yi Y-coordinate at which to evaluate the analytic solution
    //! \param[in] zi Z-coordinate at which to evaluate the analytic solution
    //! \param[in] t Physical time at which to evaluate the analytic solution
    //! \return Vector of analytic solution at given location and time
    std::vector< tk::real >
    solution( tk::real xi, tk::real yi, tk::real zi, tk::real t ) const
    { return Problem::initialize( m_system, m_ncomp, xi, yi, zi, t ); }

    //! Compute right hand side for DiagCG (CG+FCT)
    //! \param[in] t Physical time
    //! \param[in] deltat Size of time step
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] U Solution vector at recent time step
    //! \param[in,out] Ue Element-centered solution vector at intermediate step
    //!    (used here internally as a scratch array)
    //! \param[in,out] R Right-hand side vector computed
    void rhs( real t,
              real deltat,
              const std::array< std::vector< real >, 3 >& coord,
              const std::vector< std::size_t >& inpoel,
              const tk::Fields& U,
              tk::Fields& Ue,
              tk::Fields& R ) const
    {
      Assert( U.nunk() == coord[0].size(), "Number of unknowns in solution "
              "vector at recent time step incorrect" );
      Assert( R.nunk() == coord[0].size(),
              "Number of unknowns and/or number of components in right-hand "
              "side vector incorrect" );

      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // 1st stage: update element values from node values (gather-add)
      for (std::size_t e=0; e<inpoel.size()/4; ++e) {
        // access node IDs
        const std::array< std::size_t, 4 >
          N{{ inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] }};
        // compute element Jacobi determinant
        const std::array< real, 3 >
          ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
          ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
          da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
        const auto J = tk::triple( ba, ca, da );        // J = 6V
        Assert( J > 0, "Element Jacobian non-positive" );

        // shape function derivatives, nnode*ndim [4][3]
        std::array< std::array< real, 3 >, 4 > grad;
        grad[1] = tk::crossdiv( ca, da, J );
        grad[2] = tk::crossdiv( da, ba, J );
        grad[3] = tk::crossdiv( ba, ca, J );
        for (std::size_t i=0; i<3; ++i)
          grad[0][i] = -grad[1][i]-grad[2][i]-grad[3][i];

        // access solution at element nodes
        std::array< std::array< real, 4 >, m_ncomp > u;
        for (ncomp_t c=0; c<m_ncomp; ++c) u[c] = U.extract( c, m_offset, N );

        // apply stagnation BCs
        for (std::size_t a=0; a<4; ++a)
          if ( !skipPoint(x[N[a]],y[N[a]],z[N[a]]) &&
               stagPoint(x[N[a]],y[N[a]],z[N[a]]) )
          {
            u[1][a] = u[2][a] = u[3][a] = 0.0;
          }

        // access solution at elements
        std::array< const real*, m_ncomp > ue;
        for (ncomp_t c=0; c<m_ncomp; ++c) ue[c] = Ue.cptr( c, m_offset );

        // pressure
        std::array< real, 4 > p;
        for (std::size_t a=0; a<4; ++a)
          p[a] = eos_pressure< eq >
                   ( m_system, u[0][a], u[1][a]/u[0][a], u[2][a]/u[0][a],
                     u[3][a]/u[0][a], u[4][a] );

        // sum flux contributions to element
        real d = deltat/2.0;
        for (std::size_t j=0; j<3; ++j)
          for (std::size_t a=0; a<4; ++a) {
            // mass: advection
            Ue.var(ue[0],e) -= d * grad[a][j] * u[j+1][a];
            // momentum: advection
            for (std::size_t i=0; i<3; ++i)
              Ue.var(ue[i+1],e) -= d * grad[a][j] * u[j+1][a]*u[i+1][a]/u[0][a];
            // momentum: pressure
            Ue.var(ue[j+1],e) -= d * grad[a][j] * p[a];
            // energy: advection and pressure
            Ue.var(ue[4],e) -= d * grad[a][j] *
                              (u[4][a] + p[a]) * u[j+1][a]/u[0][a];
          }

        // add (optional) source to all equations
        for (std::size_t a=0; a<4; ++a) {
          real s[m_ncomp];
          Problem::src( m_system, x[N[a]], y[N[a]], z[N[a]], t,
                        s[0], s[1], s[2], s[3], s[4] );
          for (std::size_t c=0; c<m_ncomp; ++c)
            Ue.var(ue[c],e) += d/4.0 * s[c];
        }
      }

      // 2nd stage: form rhs from element values (scatter-add)
      for (std::size_t e=0; e<inpoel.size()/4; ++e) {
        // access node IDs
        const std::array< std::size_t, 4 >
          N{{ inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] }};
        // compute element Jacobi determinant
        const std::array< real, 3 >
          ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
          ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
          da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
        const auto J = tk::triple( ba, ca, da );        // J = 6V
        Assert( J > 0, "Element Jacobian non-positive" );

        // shape function derivatives, nnode*ndim [4][3]
        std::array< std::array< real, 3 >, 4 > grad;
        grad[1] = tk::crossdiv( ca, da, J );
        grad[2] = tk::crossdiv( da, ba, J );
        grad[3] = tk::crossdiv( ba, ca, J );
        for (std::size_t i=0; i<3; ++i)
          grad[0][i] = -grad[1][i]-grad[2][i]-grad[3][i];

        // access solution at elements
        std::array< real, m_ncomp > ue;
        for (ncomp_t c=0; c<m_ncomp; ++c) ue[c] = Ue( e, c, m_offset );
        // access pointer to right hand side at component and offset
        std::array< const real*, m_ncomp > r;
        for (ncomp_t c=0; c<m_ncomp; ++c) r[c] = R.cptr( c, m_offset );

        // pressure
        auto p = eos_pressure< eq >
                   ( m_system, ue[0], ue[1]/ue[0], ue[2]/ue[0], ue[3]/ue[0],
                     ue[4] );

        // scatter-add flux contributions to rhs at nodes
        real d = deltat * J/6.0;
        for (std::size_t j=0; j<3; ++j)
          for (std::size_t a=0; a<4; ++a) {
            // mass: advection
            R.var(r[0],N[a]) += d * grad[a][j] * ue[j+1];
            // momentum: advection
            for (std::size_t i=0; i<3; ++i)
              R.var(r[i+1],N[a]) += d * grad[a][j] * ue[j+1]*ue[i+1]/ue[0];
            // momentum: pressure
            R.var(r[j+1],N[a]) += d * grad[a][j] * p;
            // energy: advection and pressure
            R.var(r[4],N[a]) += d * grad[a][j] * (ue[4] + p) * ue[j+1]/ue[0];
          }

        // add (optional) source to all equations
        auto xc = (x[N[0]] + x[N[1]] + x[N[2]] + x[N[3]]) / 4.0;
        auto yc = (y[N[0]] + y[N[1]] + y[N[2]] + y[N[3]]) / 4.0;
        auto zc = (z[N[0]] + z[N[1]] + z[N[2]] + z[N[3]]) / 4.0;
        real s[m_ncomp];
        Problem::src( m_system, xc, yc, zc, t+deltat/2,
                      s[0], s[1], s[2], s[3], s[4] );
        for (std::size_t c=0; c<m_ncomp; ++c)
          for (std::size_t a=0; a<4; ++a)
            R.var(r[c],N[a]) += d/4.0 * s[c];
      }
//         // add viscous stress contribution to momentum and energy rhs
//         m_physics.viscousRhs( deltat, J, N, grad, u, r, R );
//         // add heat conduction contribution to energy rhs
//         m_physics.conductRhs( deltat, J, N, grad, u, r, R );
    }

    //! \brief Compute nodal gradients of primitive variables for ALECG along
    //!   chare-boundary
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bndel List of elements contributing to chare-boundary nodes
    //! \param[in] gid Local->global node id map
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in] U Solution vector at recent time step
    //! \param[in,out] G Nodal gradients of primitive variables
    //! \details This function only computes local contributions to gradients
    //!   at chare-boundary nodes. Internal node gradients are calculated as
    //!   required, and do not need to be stored.
    void chBndGrad( const std::array< std::vector< real >, 3 >& coord,
                    const std::vector< std::size_t >& inpoel,
                    const std::vector< std::size_t >& bndel,
                    const std::vector< std::size_t >& gid,
                    const std::unordered_map< std::size_t, std::size_t >& bid,
                    const tk::Fields& U,
                    tk::Fields& G ) const
    {
      Assert( U.nunk() == coord[0].size(), "Number of unknowns in solution "
              "vector at recent time step incorrect" );

      // compute gradients of primitive variables in points
      G.fill( 0.0 );

      // access node cooordinates
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      for (auto e : bndel) {  // elements contributing to chare boundary nodes
        // access node IDs
        std::size_t N[4] =
          { inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] };
        // compute element Jacobi determinant, J = 6V
        real bax = x[N[1]]-x[N[0]];
        real bay = y[N[1]]-y[N[0]];
        real baz = z[N[1]]-z[N[0]];
        real cax = x[N[2]]-x[N[0]];
        real cay = y[N[2]]-y[N[0]];
        real caz = z[N[2]]-z[N[0]];
        real dax = x[N[3]]-x[N[0]];
        real day = y[N[3]]-y[N[0]];
        real daz = z[N[3]]-z[N[0]];
        auto J = tk::triple( bax, bay, baz, cax, cay, caz, dax, day, daz );
        auto J24 = J/24.0;
        // shape function derivatives, nnode*ndim [4][3]
        real g[4][3];
        tk::crossdiv( cax, cay, caz, dax, day, daz, J,
                      g[1][0], g[1][1], g[1][2] );
        tk::crossdiv( dax, day, daz, bax, bay, baz, J,
                      g[2][0], g[2][1], g[2][2] );
        tk::crossdiv( bax, bay, baz, cax, cay, caz, J,
                      g[3][0], g[3][1], g[3][2] );
        for (std::size_t i=0; i<3; ++i)
          g[0][i] = -g[1][i] - g[2][i] - g[3][i];
        // scatter-add gradient contributions to boundary nodes
        for (std::size_t a=0; a<4; ++a) {
          auto i = bid.find( gid[N[a]] );
          if (i != end(bid)) {
            real u[5];
            for (std::size_t b=0; b<4; ++b) {
              u[0] = U(N[b],0,m_offset);
              u[1] = U(N[b],1,m_offset)/u[0];
              u[2] = U(N[b],2,m_offset)/u[0];
              u[3] = U(N[b],3,m_offset)/u[0];
              u[4] = U(N[b],4,m_offset)/u[0]
                     - 0.5*(u[1]*u[1] + u[2]*u[2] + u[3]*u[3]);
              if ( !skipPoint(x[N[b]],y[N[b]],z[N[b]]) &&
                   stagPoint(x[N[b]],y[N[b]],z[N[b]]) )
              {
                u[1] = u[2] = u[3] = 0.0;
              }
              for (std::size_t c=0; c<5; ++c)
                for (std::size_t j=0; j<3; ++j)
                  G(i->second,c*3+j,0) += J24 * g[b][j] * u[c];
            }
          }
        }
      }
    }

    //! Compute right hand side for ALECG
    //! \param[in] t Physical time
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] triinpoel Boundary triangle face connecitivity with local ids
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in] gid Local->glocal node ids
    //! \param[in] lid Global->local node ids
    //! \param[in] dfn Dual-face normals
    //! \param[in] psup Points surrounding points
    //! \param[in] esup Elements surrounding points
    //! \param[in] symbctri Vector with 1 at symmetry BC boundary triangles
    //! \param[in] vol Nodal volumes
    //! \param[in] edgenode Local node IDs of edges
    //! \param[in] edgeid Edge ids in the order of access
    //! \param[in] boxnodes Mesh node ids within user-defined box
    //! \param[in] G Nodal gradients
    //! \param[in] U Solution vector at recent time step
    //! \param[in] tp Physical time for each mesh node
    //! \param[in] V Total box volume
    //! \param[in,out] R Right-hand side vector computed
    void rhs( real t,
              const std::array< std::vector< real >, 3 >& coord,
              const std::vector< std::size_t >& inpoel,
              const std::vector< std::size_t >& triinpoel,
              const std::vector< std::size_t >& gid,
              const std::unordered_map< std::size_t, std::size_t >& bid,
              const std::unordered_map< std::size_t, std::size_t >& lid,
              const std::vector< real >& dfn,
              const std::pair< std::vector< std::size_t >,
                               std::vector< std::size_t > >& psup,
              const std::pair< std::vector< std::size_t >,
                               std::vector< std::size_t > >& esup,
              const std::vector< int >& symbctri,
              const std::vector< real >& vol,
              const std::vector< std::size_t >& edgenode,
              const std::vector< std::size_t >& edgeid,
              const std::unordered_set< std::size_t >& boxnodes,
              const tk::Fields& G,
              const tk::Fields& U,
              const std::vector< tk::real >& tp,
              real V,
              tk::Fields& R ) const
    {
      Assert( G.nprop() == m_ncomp*3,
              "Number of components in gradient vector incorrect" );
      Assert( U.nunk() == coord[0].size(), "Number of unknowns in solution "
              "vector at recent time step incorrect" );
      Assert( R.nunk() == coord[0].size(),
              "Number of unknowns and/or number of components in right-hand "
              "side vector incorrect" );

      // compute/assemble gradients in points
      auto Grad = nodegrad( coord, inpoel, lid, bid, vol, esup, U, G );

      // zero right hand side for all components
      for (ncomp_t c=0; c<m_ncomp; ++c) R.fill( c, m_offset, 0.0 );

      // compute domain-edge integral
      domainint( coord, gid, edgenode, edgeid, psup, dfn, U, Grad, R );

      // compute boundary integrals
      bndint( coord, triinpoel, symbctri, U, R );

      // compute external (energy) sources
      const auto& ic = g_inputdeck.get< tag::param, eq, tag::ic >();
      const auto& icbox = ic.get< tag::box >();
      const auto& initiate = icbox.get< tag::initiate >();
      const auto& inittype = initiate.get< tag::init >();
      if (inittype.size() > m_system)
        if (inittype[m_system] == ctr::InitiateType::LINEAR)
          boxSrc( V, t, inpoel, esup, boxnodes, coord, R );

      // compute optional source integral
      src( coord, inpoel, t, tp, R );
    }

    //! Compute the minimum time step size
    //! \param[in] U Solution vector at recent time step
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] t Physical time
    //! \return Minimum time step size
    real dt( const std::array< std::vector< real >, 3 >& coord,
             const std::vector< std::size_t >& inpoel,
             tk::real t,
             const tk::Fields& U ) const
    {
      Assert( U.nunk() == coord[0].size(), "Number of unknowns in solution "
              "vector at recent time step incorrect" );

      // energy source propagation time and velocity
      const auto& ic = g_inputdeck.get< tag::param, eq, tag::ic >();
      const auto& icbox = ic.get< tag::box >();
      const auto& initiate = icbox.get< tag::initiate >();
      const auto& iv = initiate.get< tag::velocity >()[ m_system ];
      const auto& inittype = initiate.get< tag::init >();

      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];
      // ratio of specific heats
      auto g = g_inputdeck.get< tag::param, eq, tag::gamma >()[0][0];
      // compute the minimum dt across all elements we own
      real mindt = std::numeric_limits< real >::max();
      for (std::size_t e=0; e<inpoel.size()/4; ++e) {
        const std::array< std::size_t, 4 > N{{ inpoel[e*4+0], inpoel[e*4+1],
                                               inpoel[e*4+2], inpoel[e*4+3] }};
        // compute cubic root of element volume as the characteristic length
        const std::array< real, 3 >
          ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
          ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
          da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
        const auto L = std::cbrt( tk::triple( ba, ca, da ) / 6.0 );
        // access solution at element nodes at recent time step
        std::array< std::array< real, 4 >, m_ncomp > u;
        for (ncomp_t c=0; c<m_ncomp; ++c) u[c] = U.extract( c, m_offset, N );
        // compute the maximum length of the characteristic velocity (fluid
        // velocity + sound velocity) across the four element nodes
        real maxvel = 0.0;
        for (std::size_t j=0; j<4; ++j) {
          auto& r  = u[0][j];    // rho
          auto& ru = u[1][j];    // rho * u
          auto& rv = u[2][j];    // rho * v
          auto& rw = u[3][j];    // rho * w
          auto& re = u[4][j];    // rho * e
          auto p = eos_pressure< eq >( m_system, r, ru/r, rv/r, rw/r, re );
          if (p < 0) p = 0.0;
          auto c = eos_soundspeed< eq >( m_system, r, p );
          auto v = std::sqrt((ru*ru + rv*rv + rw*rw)/r/r) + c; // char. velocity

          // energy source propagation velocity
          if (inittype.size() > m_system)
            if (inittype[m_system] == ctr::InitiateType::LINEAR) {
              std::vector< tk::real >
                boxdim{ icbox.get< tag::xmin >(), icbox.get< tag::xmax >(),
                        icbox.get< tag::ymin >(), icbox.get< tag::ymax >(),
                        icbox.get< tag::zmin >(), icbox.get< tag::zmax >() };
              auto wFront = 0.08;
              auto tInit = 0.0;
              auto tFinal = tInit + (boxdim[5] - boxdim[4] - 2.0*wFront) /
                std::fabs(iv[0]);
              if (t >= tInit && t <= tFinal) v = std::max(v, std::fabs(iv[0]));
            }
          if (v > maxvel) maxvel = v;
        }
        // compute element dt for the Euler equations
        auto euler_dt = L / maxvel;
        // compute element dt based on the viscous force
        auto viscous_dt = m_physics.viscous_dt( L, u );
        // compute element dt based on thermal diffusion
        auto conduct_dt = m_physics.conduct_dt( L, g, u );
        // compute minimum element dt
        auto elemdt = std::min( euler_dt, std::min( viscous_dt, conduct_dt ) );
        // find minimum dt across all elements
        if (elemdt < mindt) mindt = elemdt;
      }
      return mindt * g_inputdeck.get< tag::discr, tag::cfl >();
    }

    //! Compute a time step size for each mesh node
    //! \param[in] U Solution vector at recent time step
    //! \param[in] vol Nodal volume (with contributions from other chares)
    //! \param[in,out] dtp Time step size for each mesh node
    void dt( uint64_t,
             const std::vector< tk::real >& vol,
             const tk::Fields& U,
             std::vector< tk::real >& dtp ) const
    {
      for (std::size_t i=0; i<U.nunk(); ++i) {
        // compute cubic root of element volume as the characteristic length
        const auto L = std::cbrt( vol[i] );
        // access solution at node p at recent time step
        const auto u = U[i];
        // compute pressure
        auto p = eos_pressure< eq >
                   ( m_system, u[0], u[1]/u[0], u[2]/u[0], u[3]/u[0], u[4] );
        if (p < 0) p = 0.0;
        auto c = eos_soundspeed< eq >( m_system, u[0], p );
        // characteristic velocity
        auto v = std::sqrt((u[1]*u[1] + u[2]*u[2] + u[3]*u[3])/u[0]/u[0]) + c;
        // compute dt for node
        dtp[i] = L / v * g_inputdeck.get< tag::discr, tag::cfl >();
      }
    }

    //! Extract the velocity field at cell nodes. Currently unused.
    //! \param[in] U Solution vector at recent time step
    //! \param[in] N Element node indices    
    //! \return Array of the four values of the velocity field
    std::array< std::array< real, 4 >, 3 >
    velocity( const tk::Fields& U,
              const std::array< std::vector< real >, 3 >&,
              const std::array< std::size_t, 4 >& N ) const
    {
      std::array< std::array< real, 4 >, 3 > v;
      v[0] = U.extract( 1, m_offset, N );
      v[1] = U.extract( 2, m_offset, N );
      v[2] = U.extract( 3, m_offset, N );
      auto r = U.extract( 0, m_offset, N );
      std::transform( r.begin(), r.end(), v[0].begin(), v[0].begin(),
                      []( real s, real& d ){ return d /= s; } );
      std::transform( r.begin(), r.end(), v[1].begin(), v[1].begin(),
                      []( real s, real& d ){ return d /= s; } );
      std::transform( r.begin(), r.end(), v[2].begin(), v[2].begin(),
                      []( real s, real& d ){ return d /= s; } );
      return v;
    }

    //! \brief Query Dirichlet boundary condition value on a given side set for
    //!    all components in this PDE system
    //! \param[in] t Physical time
    //! \param[in] deltat Time step size
    //! \param[in] tp Physical time for each mesh node
    //! \param[in] dtp Time step size for each mesh node
    //! \param[in] ss Pair of side set ID and (local) node IDs on the side set
    //! \param[in] coord Mesh node coordinates
    //! \return Vector of pairs of bool and boundary condition value associated
    //!   to mesh node IDs at which Dirichlet boundary conditions are set. Note
    //!   that instead of the actual boundary condition value, we return the
    //!   increment between t+deltat and t, since that is what the solution requires
    //!   as we solve for the soution increments and not the solution itself.
    std::map< std::size_t, std::vector< std::pair<bool,real> > >
    dirbc( real t,
           real deltat,
           const std::vector< tk::real >& tp,
           const std::vector< tk::real >& dtp,
           const std::pair< const int, std::vector< std::size_t > >& ss,
           const std::array< std::vector< real >, 3 >& coord ) const
    {
      using tag::param; using tag::bcdir;
      using NodeBC = std::vector< std::pair< bool, real > >;
      std::map< std::size_t, NodeBC > bc;
      const auto& ubc = g_inputdeck.get< param, eq, tag::bc, bcdir >();
      const auto steady = g_inputdeck.get< tag::discr, tag::steady_state >();
      if (!ubc.empty()) {
        Assert( ubc.size() > 0, "Indexing out of Dirichlet BC eq-vector" );
        const auto& x = coord[0];
        const auto& y = coord[1];
        const auto& z = coord[2];
        for (const auto& b : ubc[0])
          if (std::stoi(b) == ss.first)
            for (auto n : ss.second) {
              Assert( x.size() > n, "Indexing out of coordinate array" );
              if (steady) { t = tp[n]; deltat = dtp[n]; }
              auto s = solinc( m_system, m_ncomp, x[n], y[n], z[n],
                               t, deltat, Problem::initialize );
              bc[n] = {{ {true,s[0]}, {true,s[1]}, {true,s[2]}, {true,s[3]},
                         {true,s[4]} }};
            }
      }
      return bc;
    }

    //! Set symmetry boundary conditions at nodes
    //! \param[in] U Solution vector at recent time step
    //! \param[in] coord Mesh node coordinates
    //! \param[in] bnorm Face normals in boundary points, key local node id,
    //!   first 3 reals of value: unit normal, outer key: side set id
    //! \param[in] nodes Unique set of node ids at which to set symmetry BCs
    void
    symbc( tk::Fields& U,
           const std::array< std::vector< real >, 3 >& coord,
           const std::unordered_map< int,
             std::unordered_map< std::size_t, std::array< real, 4 > > >& bnorm,
           const std::unordered_set< std::size_t >& nodes ) const
    {
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];
      const auto& sbc = g_inputdeck.get< param, eq, tag::bc, tag::bcsym >();
      if (sbc.size() > m_system)               // use symbcs for this system
        for (auto p : nodes)                   // for all symbc nodes
          if (!skipPoint(x[p],y[p],z[p]))
            for (const auto& s : sbc[m_system]) {// for all user-def symbc sets
              auto j = bnorm.find(std::stoi(s));// find nodes & normals for side
              if (j != end(bnorm)) {
                auto i = j->second.find(p);      // find normal for node
                if (i != end(j->second)) {
                  std::array< real, 3 >
                    n{ i->second[0], i->second[1], i->second[2] },
                    v{ U(p,1,m_offset), U(p,2,m_offset), U(p,3,m_offset) };
                  auto v_dot_n = tk::dot( v, n );
                  U(p,1,m_offset) -= v_dot_n * n[0];
                  U(p,2,m_offset) -= v_dot_n * n[1];
                  U(p,3,m_offset) -= v_dot_n * n[2];
                }
              }
            }
    }

    //! Set farfield boundary conditions at nodes
    //! \param[in] U Solution vector at recent time step
    //! \param[in] coord Mesh node coordinates
    //! \param[in] bnorm Face normals in boundary points, key local node id,
    //!   first 3 reals of value: unit normal, outer key: side set id
    //! \param[in] nodes Unique set of node ids at which to set farfield BCs
    void
    farfieldbc(
      tk::Fields& U,
      const std::array< std::vector< real >, 3 >& coord,
      const std::unordered_map< int,
        std::unordered_map< std::size_t, std::array< real, 4 > > >& bnorm,
      const std::unordered_set< std::size_t >& nodes ) const
    {
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];
      const auto& fbc = g_inputdeck.get<param, eq, tag::bc, tag::bcfarfield>();
      if (fbc.size() > m_system)               // use farbcs for this system
        for (auto p : nodes)                   // for all farfieldbc nodes
          if (!skipPoint(x[p],y[p],z[p]))
            for (const auto& s : fbc[m_system]) {// for all user-def farbc sets
              auto j = bnorm.find(std::stoi(s));// find nodes & normals for side
              if (j != end(bnorm)) {
                auto i = j->second.find(p);      // find normal for node
                if (i != end(j->second)) {
                  auto& r  = U(p,0,m_offset);
                  auto& ru = U(p,1,m_offset);
                  auto& rv = U(p,2,m_offset);
                  auto& rw = U(p,3,m_offset);
                  auto& re = U(p,4,m_offset);
                  auto vn =
                    (ru*i->second[0] + rv*i->second[1] + rw*i->second[2]) / r;
                  auto a = eos_soundspeed< eq >( m_system, r,
                    eos_pressure< eq >( m_system, r, ru/r, rv/r, rw/r, re ) );
                  auto M = vn / a;
                  if (M <= -1.0) {                      // supersonic inflow
                    r  = m_fr;
                    ru = m_fr * m_fu[0];
                    rv = m_fr * m_fu[1];
                    rw = m_fr * m_fu[2];
                    re = eos_totalenergy< eq >
                           ( m_system, m_fr, m_fu[0], m_fu[1], m_fu[2], m_fp );
                  } else if (M > -1.0 && M < 0.0) {     // subsonic inflow
                    r  = m_fr;
                    ru = m_fr * m_fu[0];
                    rv = m_fr * m_fu[1];
                    rw = m_fr * m_fu[2];
                    re =
                    eos_totalenergy< eq >( m_system, m_fr, m_fu[0], m_fu[1],
                      m_fu[2], eos_pressure< eq >( m_system, r, ru/r, rv/r,
                                                   rw/r, re ) );
                  } else if (M >= 0.0 && M < 1.0) {     // subsonic outflow
                    re = eos_totalenergy< eq >( m_system, r, ru/r, rv/r, rw/r,
                                                m_fp );
                  }
                }
              }
            }
    }

    //! Return analytic field names to be output to file
    //! \return Vector of strings labelling analytic fields output in file
    std::vector< std::string > analyticFieldNames() const
    { return m_problem.analyticFieldNames( m_ncomp ); }

    //! Return surface field names to be output to file
    //! \return Vector of strings labelling surface fields output in file
    std::vector< std::string > surfNames() const
    { return CompFlowSurfNames(); }

    //! Return time history field names to be output to file
    //! \return Vector of strings labelling time history fields output in file
    std::vector< std::string > histNames() const
    { return CompFlowHistNames(); }

    //! Return surface field output going to file
    std::vector< std::vector< real > >
    surfOutput( const std::map< int, std::vector< std::size_t > >& bnd,
                tk::Fields& U ) const
    { return CompFlowSurfOutput( m_system, bnd, U ); }

    //! Return time history field output evaluated at time history points
    std::vector< std::vector< real > >
    histOutput( const std::vector< HistData >& h,
                const std::vector< std::size_t >& inpoel,
                const tk::Fields& U ) const
    { return CompFlowHistOutput( m_system, h, inpoel, U ); }

    //! Return names of integral variables to be output to diagnostics file
    //! \return Vector of strings labelling integral variables output
    std::vector< std::string > names() const
    { return m_problem.names( m_ncomp ); }

  private:
    const Physics m_physics;            //!< Physics policy
    const Problem m_problem;            //!< Problem policy
    const ncomp_t m_system;             //!< Equation system index
    const ncomp_t m_offset;             //!< Offset PDE operates from
    //! Stagnation BC user configuration: point coordinates and radii
    const std::tuple< std::vector< real >, std::vector< real > > m_stagCnf;
    //! Skip BC user configuration: point coordinates and radii
    const std::tuple< std::vector< real >, std::vector< real > > m_skipCnf;
    const real m_fr;                    //!< Farfield density
    const real m_fp;                    //!< Farfield pressure
    const std::vector< real > m_fu;     //!< Farfield velocity

    //! Decide if point is a stagnation point
    //! \param[in] x X mesh point coordinates to query
    //! \param[in] y Y mesh point coordinates to query
    //! \param[in] z Z mesh point coordinates to query
    //! \return True if point is configured as a stagnation point by the user
    #pragma omp declare simd
    bool
    stagPoint( real x, real y, real z ) const {
      const auto& pnt = std::get< 0 >( m_stagCnf );
      const auto& rad = std::get< 1 >( m_stagCnf );
      for (std::size_t i=0; i<rad.size(); ++i) {
        if (tk::length( x-pnt[i*3+0], y-pnt[i*3+1], z-pnt[i*3+2] ) < rad[i])
          return true;
      }
      return false;
    }

    //! Decide if point is a skip-BC point
    //! \param[in] x X mesh point coordinates to query
    //! \param[in] y Y mesh point coordinates to query
    //! \param[in] z Z mesh point coordinates to query
    //! \return True if point is configured as a skip-BC point by the user
    #pragma omp declare simd
    bool
    skipPoint( real x, real y, real z ) const {
      const auto& pnt = std::get< 0 >( m_skipCnf );
      const auto& rad = std::get< 1 >( m_skipCnf );
      for (std::size_t i=0; i<rad.size(); ++i) {
        if (tk::length( x-pnt[i*3+0], y-pnt[i*3+1], z-pnt[i*3+2] ) < rad[i])
          return true;
      }
      return false;
    }

    //! \brief Compute/assemble nodal gradients of primitive variables for
    //!   ALECG in all points
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] lid Global->local node ids
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in] vol Nodal volumes
    //! \param[in] U Solution vector at recent time step
    //! \param[in] G Nodal gradients of primitive variables in chare-boundary nodes
    //! \return Gradients of primitive variables in all mesh points
    tk::Fields
    nodegrad( const std::array< std::vector< real >, 3 >& coord,
              const std::vector< std::size_t >& inpoel,
              const std::unordered_map< std::size_t, std::size_t >& lid,
              const std::unordered_map< std::size_t, std::size_t >& bid,
              const std::vector< real >& vol,
              const std::pair< std::vector< std::size_t >,
                               std::vector< std::size_t > >& esup,
              const tk::Fields& U,
              const tk::Fields& G ) const
    {
      // allocate storage for nodal gradients of primitive variables
      tk::Fields Grad( U.nunk(), m_ncomp*3 );
      Grad.fill( 0.0 );

      // access node cooordinates
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // compute gradients of primitive variables in points
      auto npoin = U.nunk();
      #pragma omp simd
      for (std::size_t p=0; p<npoin; ++p)
        for (auto e : tk::Around(esup,p)) {
          // access node IDs
          std::size_t N[4] =
            { inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] };
          // compute element Jacobi determinant, J = 6V
          real bax = x[N[1]]-x[N[0]];
          real bay = y[N[1]]-y[N[0]];
          real baz = z[N[1]]-z[N[0]];
          real cax = x[N[2]]-x[N[0]];
          real cay = y[N[2]]-y[N[0]];
          real caz = z[N[2]]-z[N[0]];
          real dax = x[N[3]]-x[N[0]];
          real day = y[N[3]]-y[N[0]];
          real daz = z[N[3]]-z[N[0]];
          auto J = tk::triple( bax, bay, baz, cax, cay, caz, dax, day, daz );
          auto J24 = J/24.0;
          // shape function derivatives, nnode*ndim [4][3]
          real g[4][3];
          tk::crossdiv( cax, cay, caz, dax, day, daz, J,
                        g[1][0], g[1][1], g[1][2] );
          tk::crossdiv( dax, day, daz, bax, bay, baz, J,
                        g[2][0], g[2][1], g[2][2] );
          tk::crossdiv( bax, bay, baz, cax, cay, caz, J,
                        g[3][0], g[3][1], g[3][2] );
          for (std::size_t i=0; i<3; ++i)
            g[0][i] = -g[1][i] - g[2][i] - g[3][i];
          // scatter-add gradient contributions to boundary nodes
          real u[m_ncomp];
          for (std::size_t b=0; b<4; ++b) {
            u[0] = U(N[b],0,m_offset);
            u[1] = U(N[b],1,m_offset)/u[0];
            u[2] = U(N[b],2,m_offset)/u[0];
            u[3] = U(N[b],3,m_offset)/u[0];
            u[4] = U(N[b],4,m_offset)/u[0]
                   - 0.5*(u[1]*u[1] + u[2]*u[2] + u[3]*u[3]);
            if ( !skipPoint(x[N[b]],y[N[b]],z[N[b]]) &&
                 stagPoint(x[N[b]],y[N[b]],z[N[b]]) )
            {
              u[1] = u[2] = u[3] = 0.0;
            }
            for (std::size_t c=0; c<m_ncomp; ++c)
              for (std::size_t i=0; i<3; ++i)
                Grad(p,c*3+i,0) += J24 * g[b][i] * u[c];
          }
        }

      // put in nodal gradients of chare-boundary points
      for (const auto& [g,b] : bid) {
        auto i = tk::cref_find( lid, g );
        for (ncomp_t c=0; c<Grad.nprop(); ++c)
          Grad(i,c,0) = G(b,c,0);
      }

      // divide weak result in gradients by nodal volume
      for (std::size_t p=0; p<npoin; ++p)
        for (std::size_t c=0; c<m_ncomp*3; ++c)
          Grad(p,c,0) /= vol[p];

      return Grad;
    }

    //! Compute domain-edge integral for ALECG
    //! \param[in] coord Mesh node coordinates
    //! \param[in] gid Local->glocal node ids
    //! \param[in] edgenode Local node ids of edges
    //! \param[in] edgeid Local node id pair -> edge id map
    //! \param[in] psup Points surrounding points
    //! \param[in] dfn Dual-face normals
    //! \param[in] U Solution vector at recent time step
    //! \param[in] G Nodal gradients
    //! \param[in,out] R Right-hand side vector computed
    void domainint( const std::array< std::vector< real >, 3 >& coord,
                    const std::vector< std::size_t >& gid,
                    const std::vector< std::size_t >& edgenode,
                    const std::vector< std::size_t >& edgeid,
                    const std::pair< std::vector< std::size_t >,
                                     std::vector< std::size_t > >& psup,
                    const std::vector< real >& dfn,
                    const tk::Fields& U,
                    const tk::Fields& G,
                    tk::Fields& R ) const
    {
      // domain-edge integral: compute fluxes in edges
      std::vector< real > dflux( edgenode.size()/2 * m_ncomp );

      // access node coordinates
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      #pragma omp simd
      for (std::size_t e=0; e<edgenode.size()/2; ++e) {
        auto p = edgenode[e*2+0];
        auto q = edgenode[e*2+1];

        // compute primitive variables at edge-end points
        real rL  = U(p,0,m_offset);
        real ruL = U(p,1,m_offset) / rL;
        real rvL = U(p,2,m_offset) / rL;
        real rwL = U(p,3,m_offset) / rL;
        real reL = U(p,4,m_offset) / rL - 0.5*(ruL*ruL + rvL*rvL + rwL*rwL);
        real rR  = U(q,0,m_offset);
        real ruR = U(q,1,m_offset) / rR;
        real rvR = U(q,2,m_offset) / rR;
        real rwR = U(q,3,m_offset) / rR;
        real reR = U(q,4,m_offset) / rR - 0.5*(ruR*ruR + rvR*rvR + rwR*rwR);

        // apply stagnation BCs to primitive variables
        if ( !skipPoint(x[p],y[p],z[p]) && stagPoint(x[p],y[p],z[p]) )
          ruL = rvL = rwL = 0.0;
        if ( !skipPoint(x[q],y[q],z[q]) && stagPoint(x[q],y[q],z[q]) )
          ruR = rvR = rwR = 0.0;

        // compute MUSCL reconstruction in edge-end points
        muscl( p, q, coord, G, rL, ruL, rvL, rwL, reL,
               rR, ruR, rvR, rwR, reR );

        // convert back to conserved variables
        reL = (reL + 0.5*(ruL*ruL + rvL*rvL + rwL*rwL)) * rL;
        ruL *= rL;
        rvL *= rL;
        rwL *= rL;
        reR = (reR + 0.5*(ruR*ruR + rvR*rvR + rwR*rwR)) * rR;
        ruR *= rR;
        rvR *= rR;
        rwR *= rR;

        // compute Riemann flux using edge-end point states
        real f[5];
        Rusanov::flux( dfn[e*6+0], dfn[e*6+1], dfn[e*6+2],
                       dfn[e*6+3], dfn[e*6+4], dfn[e*6+5],
                       rL, ruL, rvL, rwL, reL,
                       rR, ruR, rvR, rwR, reR,
                       f[0], f[1], f[2], f[3], f[4] );
        // store flux in edges
        for (std::size_t c=0; c<m_ncomp; ++c) dflux[e*m_ncomp+c] = f[c];
      }

      // access pointer to right hand side at component and offset
      std::array< const real*, m_ncomp > r;
      for (ncomp_t c=0; c<m_ncomp; ++c) r[c] = R.cptr( c, m_offset );

      // domain-edge integral: sum flux contributions to points
      for (std::size_t p=0,k=0; p<U.nunk(); ++p)
        for (auto q : tk::Around(psup,p)) {
          auto s = gid[p] > gid[q] ? -1.0 : 1.0;
          auto e = edgeid[k++];
          // the 2.0 in the following expression is so that the RHS contribution
          // conforms with Eq 12 (Waltz et al. Computers & fluids (92) 2014);
          // The 1/2 in Eq 12 is extracted from the flux function (Rusanov).
          // However, Rusanov::flux computes the flux with the 1/2. This 2
          // cancels with the 1/2 in Rusanov::flux, so that the 1/2 can be
          // extracted out and multiplied as in Eq 12
          for (std::size_t c=0; c<m_ncomp; ++c)
            R.var(r[c],p) -= 2.0*s*dflux[e*m_ncomp+c];
        }

      tk::destroy(dflux);
    }

    //! \brief Compute MUSCL reconstruction in edge-end points using a MUSCL
    //!    procedure with van Leer limiting
    //! \param[in] p Left node id of edge-end
    //! \param[in] q Right node id of edge-end
    //! \param[in] coord Array of nodal coordinates
    //! \param[in] G Gradient of all unknowns in mesh points
    //! \param[in,out] rL Left density
    //! \param[in,out] uL Left X velocity
    //! \param[in,out] vL Left Y velocity
    //! \param[in,out] wL Left Z velocity
    //! \param[in,out] eL Left internal energy
    //! \param[in,out] rR Right density
    //! \param[in,out] uR Right X velocity
    //! \param[in,out] vR Right Y velocity
    //! \param[in,out] wR Right Z velocity
    //! \param[in,out] eR Right internal energy
    void muscl( std::size_t p,
                std::size_t q,
                const tk::UnsMesh::Coords& coord,
                const tk::Fields& G,
                real& rL, real& uL, real& vL, real& wL, real& eL,
                real& rR, real& uR, real& vR, real& wR, real& eR ) const
    {
      // access node coordinates
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // edge vector
      std::array< real, 3 > vw{ x[q]-x[p], y[q]-y[p], z[q]-z[p] };

      real delta1[5], delta2[5], delta3[5];
      std::array< real, 5 > ls{ rL, uL, vL, wL, eL };
      std::array< real, 5 > rs{ rR, uR, vR, wR, eR };
      auto url = ls;
      auto urr = rs;

      // MUSCL reconstruction of edge-end-point primitive variables
      for (std::size_t c=0; c<5; ++c) {
        // gradients
        std::array< real, 3 > g1{ G(p,c*3+0,0), G(p,c*3+1,0), G(p,c*3+2,0) },
                              g2{ G(q,c*3+0,0), G(q,c*3+1,0), G(q,c*3+2,0) };

        delta2[c] = rs[c] - ls[c];
        delta1[c] = 2.0 * tk::dot(g1,vw) - delta2[c];
        delta3[c] = 2.0 * tk::dot(g2,vw) - delta2[c];

        // MUSCL extrapolation option 1:
        // ---------------------------------------------------------------------
        // Uncomment the following 3 blocks of code if this version is required.
        // this reconstruction is from the following paper:
        // Waltz, J., Morgan, N. R., Canfield, T. R., Charest, M. R.,
        // Risinger, L. D., & Wohlbier, J. G. (2014). A three-dimensional
        // finite element arbitrary Lagrangian–Eulerian method for shock
        // hydrodynamics on unstructured grids. Computers & Fluids, 92,
        // 172-187.

        //// form limiters
        //auto rcL = (delta2[c] + muscl_eps) / (delta1[c] + muscl_eps);
        //auto rcR = (delta2[c] + muscl_eps) / (delta3[c] + muscl_eps);
        //auto rLinv = (delta1[c] + muscl_eps) / (delta2[c] + muscl_eps);
        //auto rRinv = (delta3[c] + muscl_eps) / (delta2[c] + muscl_eps);

        //// van Leer limiter
        //// any other symmetric limiter could be used instead too
        //auto phiL = (std::abs(rcL) + rcL) / (std::abs(rcL) + 1.0);
        //auto phiR = (std::abs(rcR) + rcR) / (std::abs(rcR) + 1.0);
        //auto phi_L_inv = (std::abs(rLinv) + rLinv) / (std::abs(rLinv) + 1.0);
        //auto phi_R_inv = (std::abs(rRinv) + rRinv) / (std::abs(rRinv) + 1.0);

        //// update unknowns with reconstructed unknowns
        //url[c] += 0.25*(delta1[c]*muscl_m1*phiL + delta2[c]*muscl_p1*phi_L_inv);
        //urr[c] -= 0.25*(delta3[c]*muscl_m1*phiR + delta2[c]*muscl_p1*phi_R_inv);

        // ---------------------------------------------------------------------

        // MUSCL extrapolation option 2:
        // ---------------------------------------------------------------------
        // The following 2 blocks of code.
        // this reconstruction is from the following paper:
        // Luo, H., Baum, J. D., & Lohner, R. (1994). Edge-based finite element
        // scheme for the Euler equations. AIAA journal, 32(6), 1183-1190.
        // Van Leer, B. (1974). Towards the ultimate conservative difference
        // scheme. II. Monotonicity and conservation combined in a second-order
        // scheme. Journal of computational physics, 14(4), 361-370.

        // get Van Albada limiter
        // the following form is derived from the flux limiter phi as:
        // s = phi_inv - (1 - phi)
        auto sL = std::max(0.0, (2.0*delta1[c]*delta2[c] + muscl_eps)
          /(delta1[c]*delta1[c] + delta2[c]*delta2[c] + muscl_eps));
        auto sR = std::max(0.0, (2.0*delta3[c]*delta2[c] + muscl_eps)
          /(delta3[c]*delta3[c] + delta2[c]*delta2[c] + muscl_eps));

        // update unknowns with reconstructed unknowns
        url[c] += 0.25*sL*(delta1[c]*(1.0-muscl_const*sL)
          + delta2[c]*(1.0+muscl_const*sL));
        urr[c] -= 0.25*sR*(delta3[c]*(1.0-muscl_const*sR)
          + delta2[c]*(1.0+muscl_const*sR));

        // ---------------------------------------------------------------------
      }

      // force first order if the reconstructions for density or internal energy
      // would have allowed negative values
      if (ls[0] < delta1[0] || ls[4] < delta1[4]) url = ls;
      if (rs[0] < -delta3[0] || rs[4] < -delta3[4]) urr = rs;

      rL = url[0];
      uL = url[1];
      vL = url[2];
      wL = url[3];
      eL = url[4];

      rR = urr[0];
      uR = urr[1];
      vR = urr[2];
      wR = urr[3];
      eR = urr[4];
    }

    //! Compute boundary integrals for ALECG
    //! \param[in] coord Mesh node coordinates
    //! \param[in] triinpoel Boundary triangle face connecitivity with local ids
    //! \param[in] symbctri Vector with 1 at symmetry BC boundary triangles
    //! \param[in] U Solution vector at recent time step
    //! \param[in,out] R Right-hand side vector computed
    void bndint( const std::array< std::vector< real >, 3 >& coord,
                 const std::vector< std::size_t >& triinpoel,
                 const std::vector< int >& symbctri,
                 const tk::Fields& U,
                 tk::Fields& R ) const
    {

      // access node coordinates
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // boundary integrals: compute fluxes in edges
      std::vector< real > bflux( triinpoel.size() * m_ncomp * 2 );

      #pragma omp simd
      for (std::size_t e=0; e<triinpoel.size()/3; ++e) {
        // access node IDs
        std::size_t N[3] =
          { triinpoel[e*3+0], triinpoel[e*3+1], triinpoel[e*3+2] };
        // access solution at element nodes
        real rA  = U(N[0],0,m_offset);
        real rB  = U(N[1],0,m_offset);
        real rC  = U(N[2],0,m_offset);
        real ruA = U(N[0],1,m_offset);
        real ruB = U(N[1],1,m_offset);
        real ruC = U(N[2],1,m_offset);
        real rvA = U(N[0],2,m_offset);
        real rvB = U(N[1],2,m_offset);
        real rvC = U(N[2],2,m_offset);
        real rwA = U(N[0],3,m_offset);
        real rwB = U(N[1],3,m_offset);
        real rwC = U(N[2],3,m_offset);
        real reA = U(N[0],4,m_offset);
        real reB = U(N[1],4,m_offset);
        real reC = U(N[2],4,m_offset);
        // apply stagnation BCs
        if ( !skipPoint(x[N[0]],y[N[0]],z[N[0]]) &&
             stagPoint(x[N[0]],y[N[0]],z[N[0]]) )
        {
          ruA = rvA = rwA = 0.0;
        }
        if ( !skipPoint(x[N[1]],y[N[1]],z[N[1]]) &&
             stagPoint(x[N[1]],y[N[1]],z[N[1]]) )
        {
          ruB = rvB = rwB = 0.0;
        }
        if ( !skipPoint(x[N[2]],y[N[2]],z[N[2]]) &&
             stagPoint(x[N[2]],y[N[2]],z[N[2]]) )
        {
          ruC = rvC = rwC = 0.0;
        }
        // compute face normal
        real nx, ny, nz;
        tk::normal( x[N[0]], x[N[1]], x[N[2]],
                    y[N[0]], y[N[1]], y[N[2]],
                    z[N[0]], z[N[1]], z[N[2]],
                    nx, ny, nz );
        // compute boundary flux
        real f[m_ncomp][3];
        real p, vn;
        int sym = symbctri[e];
        p = eos_pressure< eq >( m_system, rA, ruA/rA, rvA/rA, rwA/rA, reA );
        vn = sym ? 0.0 : (nx*ruA + ny*rvA + nz*rwA) / rA;
        f[0][0] = rA*vn;
        f[1][0] = ruA*vn + p*nx;
        f[2][0] = rvA*vn + p*ny;
        f[3][0] = rwA*vn + p*nz;
        f[4][0] = (reA + p)*vn;
        p = eos_pressure< eq >( m_system, rB, ruB/rB, rvB/rB, rwB/rB, reB );
        vn = sym ? 0.0 : (nx*ruB + ny*rvB + nz*rwB) / rB;
        f[0][1] = rB*vn;
        f[1][1] = ruB*vn + p*nx;
        f[2][1] = rvB*vn + p*ny;
        f[3][1] = rwB*vn + p*nz;
        f[4][1] = (reB + p)*vn;
        p = eos_pressure< eq >( m_system, rC, ruC/rC, rvC/rC, rwC/rC, reC );
        vn = sym ? 0.0 : (nx*ruC + ny*rvC + nz*rwC) / rC;
        f[0][2] = rC*vn;
        f[1][2] = ruC*vn + p*nx;
        f[2][2] = rvC*vn + p*ny;
        f[3][2] = rwC*vn + p*nz;
        f[4][2] = (reC + p)*vn;
        // compute face area
        auto A6 = tk::area( x[N[0]], x[N[1]], x[N[2]],
                            y[N[0]], y[N[1]], y[N[2]],
                            z[N[0]], z[N[1]], z[N[2]] ) / 6.0;
        auto A24 = A6/4.0;
        // store flux in boundary elements
        for (std::size_t c=0; c<m_ncomp; ++c) {
          auto eb = (e*m_ncomp+c)*6;
          auto Bab = A24 * (f[c][0] + f[c][1]);
          bflux[eb+0] = Bab + A6 * f[c][0];
          bflux[eb+1] = Bab;
          Bab = A24 * (f[c][1] + f[c][2]);
          bflux[eb+2] = Bab + A6 * f[c][1];
          bflux[eb+3] = Bab;
          Bab = A24 * (f[c][2] + f[c][0]);
          bflux[eb+4] = Bab + A6 * f[c][2];
          bflux[eb+5] = Bab;
        }
      }

      // access pointer to right hand side at component and offset
      std::array< const real*, m_ncomp > r;
      for (ncomp_t c=0; c<m_ncomp; ++c) r[c] = R.cptr( c, m_offset );

      // boundary integrals: sum flux contributions to points
      for (std::size_t e=0; e<triinpoel.size()/3; ++e)
        for (std::size_t c=0; c<m_ncomp; ++c) {
          auto eb = (e*m_ncomp+c)*6;
          R.var(r[c],triinpoel[e*3+0]) -= bflux[eb+0] + bflux[eb+5];
          R.var(r[c],triinpoel[e*3+1]) -= bflux[eb+1] + bflux[eb+2];
          R.var(r[c],triinpoel[e*3+2]) -= bflux[eb+3] + bflux[eb+4];
        }

      tk::destroy(bflux);
    }

    //! Compute optional source integral
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] t Physical time
    //! \param[in] tp Physical time for each mesh node
    //! \param[in,out] R Right-hand side vector computed
    void src( const std::array< std::vector< real >, 3 >& coord,
              const std::vector< std::size_t >& inpoel,
              real t,
              const std::vector< tk::real >& tp,
              tk::Fields& R ) const
    {
      // access node coordinates
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // access pointer to right hand side at component and offset
      std::array< const real*, m_ncomp > r;
      for (ncomp_t c=0; c<m_ncomp; ++c) r[c] = R.cptr( c, m_offset );

      // source integral
      for (std::size_t e=0; e<inpoel.size()/4; ++e) {
        std::size_t N[4] =
          { inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] };
        // compute element Jacobi determinant, J = 6V
        auto J24 = tk::triple(
          x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]],
          x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]],
          x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] ) / 24.0;
        // sum source contributions to nodes
        for (std::size_t a=0; a<4; ++a) {
          real s[m_ncomp];
          if (g_inputdeck.get< tag::discr, tag::steady_state >()) t = tp[N[a]];
          Problem::src( m_system, x[N[a]], y[N[a]], z[N[a]], t,
                        s[0], s[1], s[2], s[3], s[4] );
          for (std::size_t c=0; c<m_ncomp; ++c)
            R.var(r[c],N[a]) += J24 * s[c];
        }
      }
    }

    //! Compute sources corresponding to a propagating front in user-defined box
    //! \param[in] V Total box volume
    //! \param[in] t Physical time
    //! \param[in] inpoel Element point connectivity
    //! \param[in] esup Elements surrounding points
    //! \param[in] boxnodes Mesh node ids within user-defined box
    //! \param[in] coord Mesh node coordinates
    //! \param[in] R Right-hand side vector
    //! \details This function add the energy source corresponding to a planar
    //!   wave-front propagating along the z-direction with a user-specified
    //!   velocity, within a box initial condition, configured by the user.
    //!   Example (SI) units of the quantities involved:
    //!    * internal energy content (energy per unit volume): J/m^3
    //!    * specific energy (internal energy per unit mass): J/kg
    void boxSrc( real V,
      real t,
      const std::vector< std::size_t >& inpoel,
      const std::pair< std::vector< std::size_t >,
                       std::vector< std::size_t > >& esup,
      const std::unordered_set< std::size_t >& boxnodes,
      const std::array< std::vector< real >, 3 >& coord,
      tk::Fields& R ) const
    {
      const auto& ic = g_inputdeck.get< tag::param, eq, tag::ic >();
      const auto& icbox = ic.get< tag::box >();
      const auto& initiate = icbox.get< tag::initiate >();

      const auto& boxenc = icbox.get< tag::energy_content >();

      Assert( boxenc.size() > m_system && !boxenc[m_system].empty(),
        "Box energy content unspecified in input file" );
      std::vector< tk::real >
        boxdim{ icbox.get< tag::xmin >(), icbox.get< tag::xmax >(),
                icbox.get< tag::ymin >(), icbox.get< tag::ymax >(),
                icbox.get< tag::zmin >(), icbox.get< tag::zmax >() };
      auto V_ex = (boxdim[1]-boxdim[0]) * (boxdim[3]-boxdim[2]) *
        (boxdim[5]-boxdim[4]);

      // determine times at which sourcing is initialized and terminated
      const auto& iv = initiate.get< tag::velocity >()[ m_system ];
      Assert( iv.size() == 1, "Excess velocities in ic-box block" );
      auto wFront = 0.08;
      auto tInit = 0.0;
      auto tFinal = tInit + (boxdim[5] - boxdim[4] - 2.0*wFront) /
        std::fabs(iv[0]);
      auto aBox = (boxdim[1]-boxdim[0]) * (boxdim[3]-boxdim[2]);

      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      if (t >= tInit && t <= tFinal) {

        // The energy front is assumed to have a half-sine-wave shape. The half
        // wave-length is the width of the front. At t=0, the center of this
        // front (i.e. the peak of the partial-sine-wave) is at X_0 + W_0.
        // W_0 is calculated based on the width of the front and the direction
        // of propagation (which is assumed to be along the z-direction).
        // If the front propagation velocity is positive, it is assumed that the
        // initial position of the energy source is the minimum z-coordinate of
        // the box; whereas if this velocity is negative, the initial position
        // is the maximum z-coordinate of the box.

        // initial center of front
        tk::real zInit(boxdim[4]);
        if (iv[0] < 0.0) zInit = boxdim[5];
        // current location of front
        auto z0 = zInit + iv[0]*t;
        auto z1 = z0 + std::copysign(wFront, iv[0]);
        tk::real s0(z0), s1(z1);
        // if velocity of propagation is negative, initial position is z1
        if (iv[0] < 0.0) {
          s0 = z1;
          s1 = z0;
        }
        // Sine-wave (positive part of the wave) source term amplitude
        auto pi = 4.0 * std::atan(1.0);
        auto amplE = boxenc[m_system][0] * V_ex * pi
          / (aBox * wFront * 2.0 * (tFinal-tInit));
        //// Square wave (constant) source term amplitude
        //auto amplE = boxenc[m_system][0] * V_ex
        //  / (aBox * wFront * (tFinal-tInit));
        amplE *= V_ex / V;

        // add source
        for (auto p : boxnodes) {
          if (z[p] >= s0 && z[p] <= s1) {
            auto S = amplE * std::sin(pi*(z[p]-s0)/wFront);
            for (auto e : tk::Around(esup,p)) {
              // access node IDs
              std::size_t N[4] =
                { inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] };
              // compute element Jacobi determinant, J = 6V
              real bax = x[N[1]]-x[N[0]];
              real bay = y[N[1]]-y[N[0]];
              real baz = z[N[1]]-z[N[0]];
              real cax = x[N[2]]-x[N[0]];
              real cay = y[N[2]]-y[N[0]];
              real caz = z[N[2]]-z[N[0]];
              real dax = x[N[3]]-x[N[0]];
              real day = y[N[3]]-y[N[0]];
              real daz = z[N[3]]-z[N[0]];
              auto J = tk::triple( bax, bay, baz, cax, cay, caz, dax, day, daz );
              auto J24 = J/24.0;
              R(p,4,m_offset) += J24 * S;
            }
          }
        }
      }
    }

};

} // cg::

} // inciter::

#endif // CGCompFlow_h

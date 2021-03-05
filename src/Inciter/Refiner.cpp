// *****************************************************************************
/*!
  \file      src/Inciter/Refiner.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Mesh refiner for interfacing the mesh refinement library
  \see       Refiner.h for more info.
*/
// *****************************************************************************

#include <vector>
#include <algorithm>

#include "Refiner.hpp"
#include "Reorder.hpp"
#include "AMR/mesh_adapter.hpp"
#include "AMR/Error.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"
#include "CGPDE.hpp"
#include "DGPDE.hpp"
#include "DerivedData.hpp"
#include "UnsMesh.hpp"
#include "Centering.hpp"
#include "Around.hpp"
#include "Sorter.hpp"
#include "Discretization.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;
extern ctr::InputDeck g_inputdeck_defaults;
extern std::vector< CGPDE > g_cgpde;
extern std::vector< DGPDE > g_dgpde;

} // inciter::

using inciter::Refiner;

Refiner::Refiner( std::size_t meshid,
                  const CProxy_Transporter& transporter,
                  const CProxy_Sorter& sorter,
                  const tk::CProxy_MeshWriter& meshwriter,
                  const std::vector< Scheme >& scheme,
                  const tk::RefinerCallback& cbr,
                  const tk::SorterCallback& cbs,
                  const std::vector< std::size_t >& ginpoel,
                  const tk::UnsMesh::CoordMap& coordmap,
                  const std::map< int, std::vector< std::size_t > >& bface,
                  const std::vector< std::size_t >& triinpoel,
                  const std::map< int, std::vector< std::size_t > >& bnode,
                  int nchare ) :
  m_meshid( meshid ),
  m_host( transporter ),
  m_sorter( sorter ),
  m_meshwriter( meshwriter ),
  m_scheme( scheme ),
  m_cbr( cbr ),
  m_cbs( cbs ),
  m_ginpoel( ginpoel ),
  m_el( tk::global2local( ginpoel ) ),     // fills m_inpoel, m_gid, m_lid
  m_coordmap( coordmap ),
  m_coord( flatcoord(coordmap) ),
  m_bface( bface ),
  m_bnode( bnode ),
  m_triinpoel( triinpoel ),
  m_nchare( nchare ),
  m_mode( RefMode::T0REF ),
  m_initref( g_inputdeck.get< tag::amr, tag::init >() ),
  m_ninitref( g_inputdeck.get< tag::amr, tag::init >().size() ),
  m_refiner( m_inpoel ),
  m_nref( 0 ),
  m_nbnd( 0 ),
  m_extra( 0 ),
  m_ch(),
  m_edgech(),
  m_chedge(),
  m_localEdgeData(),
  m_remoteEdgeData(),
  m_nodeCommMap(),
  m_oldTets(),
  m_addedNodes(),
  m_addedTets(),
  m_oldntets( 0 ),
  m_coarseBndFaces(),
  m_coarseBndNodes(),
  m_rid( ginpoel.size() ),
  m_oldrid(),
  m_lref( ginpoel.size() ),
  m_parent(),
  m_writeCallback(),
  m_outref_ginpoel(),
  m_outref_el(),
  m_outref_coord(),
  m_outref_addedNodes(),
  m_outref_addedTets(),
  m_outref_nodeCommMap(),
  m_outref_bface(),
  m_outref_bnode(),
  m_outref_triinpoel()
// *****************************************************************************
//  Constructor
//! \param[in] meshid Mesh ID
//! \param[in] transporter Transporter (host) proxy
//! \param[in] sorter Mesh reordering (sorter) proxy
//! \param[in] meshwriter Mesh writer proxy
//! \param[in] scheme Discretization schemes (one per mesh)
//! \param[in] cbr Charm++ callbacks for Refiner
//! \param[in] cbs Charm++ callbacks for Sorter
//! \param[in] ginpoel Mesh connectivity (this chare) using global node IDs
//! \param[in] coordmap Mesh node coordinates (this chare) for global node IDs
//! \param[in] bface File-internal elem ids of side sets
//! \param[in] bnode Node lists of side sets
//! \param[in] triinpoel Triangle face connectivity with global IDs
//! \param[in] nchare Total number of refiner chares (chare array elements)
// *****************************************************************************
{
  Assert( !m_ginpoel.empty(), "No elements assigned to refiner chare" );
  Assert( tk::positiveJacobians( m_inpoel, m_coord ),
          "Input mesh to Refiner Jacobian non-positive" );
  Assert( !tk::leakyPartition(
            tk::genEsuelTet( m_inpoel, tk::genEsup(m_inpoel,4) ),
            m_inpoel, m_coord ),
          "Input mesh to Refiner leaky" );
  Assert( tk::conforming( m_inpoel, m_coord, true, m_rid ),
          "Input mesh to Refiner not conforming" );

  // Generate local -> refiner lib node id map and its inverse
  libmap();

  // Reverse initial mesh refinement type list (will pop from back)
  std::reverse( begin(m_initref), end(m_initref) );

  // Generate boundary data structures for coarse mesh
  coarseBnd();

  // If initial mesh refinement is configured, start initial mesh refinement.
  // See also tk::grm::check_amr_errors in Control/Inciter/InputDeck/Ggrammar.h.
  if (g_inputdeck.get< tag::amr, tag::t0ref >() && m_ninitref > 0)
    t0ref();
  else
    endt0ref();
}
void
Refiner::libmap()
// *****************************************************************************
// (Re-)generate local -> refiner lib node id map and its inverse
// *****************************************************************************
{
  // Fill initial (matching) mapping between local and refiner node ids
  std::iota( begin(m_rid), end(m_rid), 0 );

  // Fill in inverse, mapping refiner to local node ids
  std::size_t i = 0;
  for (auto r : m_rid) m_lref[r] = i++;
}

void
Refiner::coarseBnd()
// *****************************************************************************
// (Re-)generate boundary data structures for coarse mesh
// *****************************************************************************
{
  // Generate unique set of faces for each side set of the input (coarsest) mesh
  m_coarseBndFaces.clear();
  for (const auto& [ setid, faceids ] : m_bface) {
    auto& faces = m_coarseBndFaces[ setid ];
    for (auto f : faceids) {
      faces.insert(
        {{{ m_triinpoel[f*3+0], m_triinpoel[f*3+1], m_triinpoel[f*3+2] }}} );
    }
  }

  // Generate unique set of nodes for each side set of the input (coarsest) mesh
  m_coarseBndNodes.clear();
  for (const auto& [ setid, nodes ] : m_bnode) {
    m_coarseBndNodes[ setid ].insert( begin(nodes), end(nodes) );
  }
}

void
Refiner::sendProxy()
// *****************************************************************************
// Send Refiner proxy to Discretization objects
//! \details This should be called when bound Discretization chare array
//!   elements have already been created.
// *****************************************************************************
{
  // Make sure (bound) Discretization chare is already created and accessible
  Assert( m_scheme[m_meshid].disc()[thisIndex].ckLocal() != nullptr,
          "About to dereference nullptr" );

  // Pass Refiner Charm++ chare proxy to fellow (bound) Discretization object
  m_scheme[m_meshid].disc()[thisIndex].ckLocal()->setRefiner( thisProxy );
}

void
Refiner::reorder()
// *****************************************************************************
// Query Sorter and update local mesh with the reordered one
// *****************************************************************************
{
  m_sorter[thisIndex].ckLocal()->
    mesh( m_ginpoel, m_coordmap, m_triinpoel, m_bnode );

  // Update local mesh data based on data just received from Sorter
  m_el = tk::global2local( m_ginpoel );     // fills m_inpoel, m_gid, m_lid
  m_coord = flatcoord( m_coordmap );

  // Re-generate boundary data structures for coarse mesh
  coarseBnd();

  // WARNING: This re-creates the AMR lib which is probably not what we
  // ultimately want, beacuse this deletes its history recorded during initial
  // (t<0) refinement. However, this appears to correctly update the local mesh
  // based on the reordered one (from Sorter) at least when t0ref is off.
  m_refiner = AMR::mesh_adapter_t( m_inpoel );
}

tk::UnsMesh::Coords
Refiner::flatcoord( const tk::UnsMesh::CoordMap& coordmap )
// *****************************************************************************
// Generate flat coordinate data from coordinate map
//! \param[in] coordmap Coordinates associated to global node IDs of mesh chunk
//! \return Flat coordinate data
// *****************************************************************************
{
  tk::UnsMesh::Coords coord;

  // Convert node coordinates associated to global node IDs to a flat vector
  auto npoin = coordmap.size();
  Assert( m_gid.size() == npoin, "Size mismatch" );
  coord[0].resize( npoin );
  coord[1].resize( npoin );
  coord[2].resize( npoin );
  for (const auto& [ gid, coords ] : coordmap) {
    auto i = tk::cref_find( m_lid, gid );
    Assert( i < npoin, "Indexing out of coordinate map" );
    coord[0][i] = coords[0];
    coord[1][i] = coords[1];
    coord[2][i] = coords[2];
  }

  return coord;
}

void
Refiner::dtref( const std::map< int, std::vector< std::size_t > >& bface,
                const std::map< int, std::vector< std::size_t > >& bnode,
                const std::vector< std::size_t >& triinpoel,
                RefMode mode )
// *****************************************************************************
// Start mesh refinement (during time stepping, t>0)
//! \param[in] bface Boundary-faces mapped to side set ids
//! \param[in] bnode Boundary-node lists mapped to side set ids
//! \param[in] triinpoel Boundary-face connectivity
//! \param[in] mode Refinement mode
// *****************************************************************************
{
  m_mode = mode;

  // Update boundary node lists
  m_bface = bface;
  m_bnode = bnode;
  m_triinpoel = triinpoel;

  start();
}

void
Refiner::outref( const std::map< int, std::vector< std::size_t > >& bface,
                 const std::map< int, std::vector< std::size_t > >& bnode,
                 const std::vector< std::size_t >& triinpoel,
                 CkCallback c,
                 RefMode mode )
// *****************************************************************************
// Start mesh refinement (for field output)
//! \param[in] bface Boundary-faces mapped to side set ids
//! \param[in] bnode Boundary-node lists mapped to side set ids
//! \param[in] triinpoel Boundary-face connectivity
//! \param[in] c Function to continue with after the writing field output
//! \param[in] mode Refinement mode
// *****************************************************************************
{
  m_mode = mode;

  m_writeCallback = c;

  // Update boundary node lists
  m_bface = bface;
  m_bnode = bnode;
  m_triinpoel = triinpoel;

  start();
}

void
Refiner::t0ref()
// *****************************************************************************
// Output mesh to file before a new step mesh refinement
// *****************************************************************************
{
  Assert( m_ninitref > 0, "No initial mesh refinement steps configured" );
  // Output initial mesh to file
  auto l = m_ninitref - m_initref.size();  // num initref steps completed
  auto t0 = g_inputdeck.get< tag::discr, tag::t0 >();
  if (l == 0) {
    writeMesh( "t0ref", l, t0-1.0,
      CkCallback( CkIndex_Refiner::start(), thisProxy[thisIndex] ) );
  } else {
    start();
  }
}

void
Refiner::start()
// *****************************************************************************
//  Start new step of mesh refinement
// *****************************************************************************
{
  m_extra = 0;
  m_ch.clear();
  m_remoteEdgeData.clear();
  m_remoteEdges.clear();

  updateEdgeData();

  // Generate and communicate boundary edges
  bndEdges();
}

void
Refiner::bndEdges()
// *****************************************************************************
// Generate boundary edges and send them to all chares
//! \details Extract edges on the boundary only. The boundary edges (shared by
//!   multiple chares) will be agreed on a refinement that yields a conforming
//!   mesh across chares boundaries.
// *****************************************************************************
{
  // Compute the number of edges (chunksize) a chare will respond to when
  // computing shared edges
  auto N = static_cast< std::size_t >( m_nchare );
  std::size_t chunksize = std::numeric_limits< std::size_t >::max() / N;

  // Generate boundary edges of our mesh chunk
  std::unordered_map< int, EdgeSet > chbedges;
  auto esup = tk::genEsup( m_inpoel, 4 );         // elements surrounding points
  auto esuel = tk::genEsuelTet( m_inpoel, esup ); // elems surrounding elements
  for (std::size_t e=0; e<esuel.size()/4; ++e) {
    auto mark = e*4;
    for (std::size_t f=0; f<4; ++f) {
      if (esuel[mark+f] == -1) {
        auto A = m_ginpoel[ mark+tk::lpofa[f][0] ];
        auto B = m_ginpoel[ mark+tk::lpofa[f][1] ];
        auto C = m_ginpoel[ mark+tk::lpofa[f][2] ];
        Assert( m_lid.find( A ) != end(m_lid), "Local node ID not found" );
        Assert( m_lid.find( B ) != end(m_lid), "Local node ID not found" );
        Assert( m_lid.find( C ) != end(m_lid), "Local node ID not found" );
        // assign edges to bins a single chare will respond to when computing
        // shared edges
        auto bin = A / chunksize;
        Assert( bin < N, "Will index out of number of chares" );
        chbedges[ static_cast<int>(bin) ].insert( {A,B} );
        bin = B / chunksize;
        Assert( bin < N, "Will index out of number of chares" );
        chbedges[ static_cast<int>(bin) ].insert( {B,C} );
        bin = C / chunksize;
        Assert( bin < N, "Will index out of number of chares" );
        chbedges[ static_cast<int>(bin) ].insert( {C,A} );
      }
    }
  }

  // Send edges in bins to chares that will compute shared edges
  m_nbnd = chbedges.size();
  if (m_nbnd == 0)
    contribute( sizeof(std::size_t), &m_meshid, CkReduction::nop,
                m_cbr.get< tag::queried >() );
  else
    for (const auto& [ targetchare, bndedges ] : chbedges)
      thisProxy[ targetchare ].query( thisIndex, bndedges );
}

void
Refiner::query( int fromch, const EdgeSet& edges )
// *****************************************************************************
// Incoming query for a list boundary edges for which this chare compiles
// shared edges
//! \param[in] fromch Sender chare ID
//! \param[in] edges Chare-boundary edge list from another chare
// *****************************************************************************
{
  // Store incoming edges in edge->chare and its inverse, chare->edge, maps
  for (const auto& e : edges) m_edgech[ e ].push_back( fromch );
  m_chedge[ fromch ].insert( begin(edges), end(edges) );
  // Report back to chare message received from
  thisProxy[ fromch ].recvquery();
}

void
Refiner::recvquery()
// *****************************************************************************
// Receive receipt of boundary edge lists to query
// *****************************************************************************
{
  if (--m_nbnd == 0)
    contribute( sizeof(std::size_t), &m_meshid, CkReduction::nop,
                m_cbr.get< tag::queried >() );
}

void
Refiner::response()
// *****************************************************************************
//  Respond to boundary edge list queries
// *****************************************************************************
{
  std::unordered_map< int, std::vector< int > > exp;

  // Compute shared edges whose chare ids will be sent back to querying chares
  for (const auto& [ neighborchare, bndedges ] : m_chedge) {
    auto& e = exp[ neighborchare ];
    for (const auto& ed : bndedges)
      for (auto d : tk::cref_find(m_edgech,ed))
        if (d != neighborchare)
          e.push_back( d );
  }

  // Send chare ids of shared edges to chares that issued a query to us. Shared
  // boundary edges assigned to chare ids sharing the boundary edge were
  // computed above for those chares that queried this map from us. These
  // boundary edges form a distributed table and we only work on a chunk of it.
  // Note that we only send data back to those chares that have queried us. The
  // receiving sides do not know in advance if they receive messages or not.
  // Completion is detected by having the receiver respond back and counting
  // the responses on the sender side, i.e., this chare.
  m_nbnd = exp.size();
  if (m_nbnd == 0)
    contribute( sizeof(std::size_t), &m_meshid, CkReduction::nop,
                m_cbr.get< tag::responded >() );
  else
    for (const auto& [ targetchare, bndedges ] : exp)
      thisProxy[ targetchare ].bnd( thisIndex, bndedges );
}

void
Refiner::bnd( int fromch, const std::vector< int >& chares )
// *****************************************************************************
// Receive shared boundary edges for our mesh chunk
//! \param[in] fromch Sender chare ID
//! \param[in] chares Chare ids we share edges with
// *****************************************************************************
{
  // Store chare ids we share edges with
  m_ch.insert( begin(chares), end(chares) );

  // Report back to chare message received from
  thisProxy[ fromch ].recvbnd();
}

void
Refiner::recvbnd()
// *****************************************************************************
// Receive receipt of shared boundary edges
// *****************************************************************************
{
  if (--m_nbnd == 0)
    contribute( sizeof(std::size_t), &m_meshid, CkReduction::nop,
                m_cbr.get< tag::responded >() );
}

void
Refiner::refine()
// *****************************************************************************
//  Do a single step of mesh refinement (really, only tag edges)
//! \details During initial (t<0, t0ref) mesh refinement, this is a single step
//!   in a potentially multiple-entry list of initial adaptive mesh refinement
//!   steps. Distribution of the chare-boundary edges must have preceded this
//!   step, so that boundary edges (shared by multiple chares) can agree on a
//!   refinement that yields a conforming mesh across chare boundaries.
//!
//!   During-timestepping (t>0, dtref) mesh refinement this is called once, as
//!   we only do a single step during time stepping.
//!
//!   During field-output (outref) mesh refinement, this may be called multiple
//!   times, depending the number of refinement levels needed for the field
//!   output.
// *****************************************************************************
{
  // Free memory used for computing shared boundary edges
  tk::destroy( m_edgech );
  tk::destroy( m_chedge );

  // Perform leak test on old mesh
  Assert( !tk::leakyPartition(
            tk::genEsuelTet( m_inpoel, tk::genEsup(m_inpoel,4) ),
            m_inpoel, m_coord ),
          "Mesh partition before refinement leaky" );

  if (m_mode == RefMode::T0REF) {

    // Refine mesh based on next initial refinement type
    if (!m_initref.empty()) {
      auto r = m_initref.back();    // consume (reversed) list from its back
      if (r == ctr::AMRInitialType::UNIFORM)
        uniformRefine();
      else if (r == ctr::AMRInitialType::UNIFORM_DEREFINE)
        uniformDeRefine();
      else if (r == ctr::AMRInitialType::INITIAL_CONDITIONS)
        errorRefine();
      else if (r == ctr::AMRInitialType::COORDINATES)
        coordRefine();
      else if (r == ctr::AMRInitialType::EDGELIST)
        edgelistRefine();
      else Throw( "Initial AMR type not implemented" );
    }

  } else if (m_mode == RefMode::DTREF) {

    if (g_inputdeck.get< tag::amr, tag::dtref_uniform >())
      uniformRefine();
    else
      errorRefine();

  } else if (m_mode == RefMode::DTDEREF) {

    if (g_inputdeck.get< tag::amr, tag::dtref_uniform >())
      uniformDeRefine();
    else
      errorRefine(RefMode::DTDEREF);

  } else if (m_mode == RefMode::OUTREF) {

    uniformRefine();

  } else if (m_mode == RefMode::OUTDEREF) {

    uniformDeRefine();

  } else Throw( "RefMode not implemented" );

  // Communicate extra edges
  comExtra();
}

void
Refiner::comExtra()
// *****************************************************************************
// Communicate extra edges along chare boundaries
// *****************************************************************************
{
  // Export extra added nodes on our mesh chunk boundary to other chares
  if (m_ch.empty()) {
    correctref();
  } else {
    for (auto c : m_ch) {  // for all chares we share at least an edge with
      thisProxy[c].addRefBndEdges(thisIndex, m_localEdgeData, m_intermediates);
    }
  }
}

void
Refiner::addRefBndEdges(
  int fromch,
  const AMR::EdgeData& ed,
  const std::unordered_set< std::size_t >& intermediates )
// *****************************************************************************
//! Receive edges on our chare boundary from other chares
//! \param[in] fromch Chare call coming from
//! \param[in] ed Edges on chare boundary
//! \param[in] intermediates Intermediate nodes
// *****************************************************************************
{
  // Save/augment buffers of edge data for each sender chare
  auto& red = m_remoteEdgeData[ fromch ];
  auto& re = m_remoteEdges[ fromch ];
  using edge_data_t = std::tuple< Edge, int, AMR::Edge_Lock_Case >;
  for (const auto& [ edge, data ] : ed) {
    red.push_back( edge_data_t{ edge, data.first, data.second } );
    re.push_back( edge );
  }

  // Add intermediates to mesh refiner lib
  for (const auto g : intermediates) {
    auto l = m_lid.find( g ); // convert to local node ids
    if (l != end(m_lid)) {
      m_refiner.tet_store.intermediate_list.insert( m_rid[l->second] );
    }
  }

  // Heard from every worker we share at least a single edge with
  if (++m_nref == m_ch.size()) {
    m_nref = 0;
    // Add intermediates to refiner lib
    auto localedges_orig = m_localEdgeData;
    //auto intermediates_orig = m_intermediates;
    m_refiner.lock_intermediates();
    // Run compatibility algorithm
    m_refiner.mark_refinement();
    // Update edge data from mesh refiner
    updateEdgeData();
    // If refiner lib modified our edges, need to recommunicate
    auto modified = static_cast< std::size_t >(
                      localedges_orig != m_localEdgeData ? 1 : 0 );
    //int modified = ( (localedges_orig != m_localEdgeData ||
    //                  intermediates_orig != m_intermediates) ? 1 : 0 );
    std::vector< std::size_t > meshdata{ m_meshid, modified };
    contribute( meshdata, CkReduction::max_ulong,
                m_cbr.get< tag::compatibility >() );
  }
}

void
Refiner::correctref()
// *****************************************************************************
//  Correct extra edges to arrive at conforming mesh across chare boundaries
//! \details This function is called repeatedly until there is not a a single
//!    edge that needs correction for the whole distributed problem to arrive at
//!    a conforming mesh across chare boundaries during a mesh refinement step.
// *****************************************************************************
{
  auto unlocked = AMR::Edge_Lock_Case::unlocked;

  // Storage for edge data that need correction to yield a conforming mesh
  AMR::EdgeData extra;

  // loop through all edges shared with other chares
  for (const auto& [ neighborchare, edgedata ] : m_remoteEdgeData) {
    for (const auto& [edge,remote_needs_refining,remote_lock_case] : edgedata) {
      // find local data of remote edge
      auto it = m_localEdgeData.find( edge );
      if (it != end(m_localEdgeData)) {
        auto& local = it->second;
        auto& local_needs_refining = local.first;
        auto& local_lock_case = local.second;

        auto local_needs_refining_orig = local_needs_refining;
        auto local_lock_case_orig = local_lock_case;

        Assert( !(local_lock_case > unlocked && local_needs_refining),
                "Invalid local edge: locked & needs refining" );
        Assert( !(remote_lock_case > unlocked && remote_needs_refining),
                "Invalid remote edge: locked & needs refining" );

        // compute lock from local and remote locks as most restrictive
        local_lock_case = std::max( local_lock_case, remote_lock_case );

        if (local_lock_case > unlocked)
          local_needs_refining = 0;

        if (local_lock_case == unlocked && remote_needs_refining)
          local_needs_refining = 1;

        // if the remote sent us data that makes us change our local state,
        // e.g., local{0,0} + remote(1,0} -> local{1,0}, i.e., I changed my
        // state I need to tell the world about it
        if ( (local_lock_case != local_lock_case_orig ||
              local_needs_refining != local_needs_refining_orig) ||
        // or if the remote data is inconsistent with what I think, e.g.,
        // local{1,0} + remote(0,0} -> local{1,0}, i.e., the remote does not
        // yet agree, I need to tell the world about it
             (local_lock_case != remote_lock_case ||
              local_needs_refining != remote_needs_refining) )
        {
          auto l1 = tk::cref_find( m_lid, edge[0] );
          auto l2 = tk::cref_find( m_lid, edge[1] );
          Assert( l1 != l2, "Edge end-points local ids are the same" );
          auto r1 = m_rid[ l1 ];
          auto r2 = m_rid[ l2 ];
          Assert( r1 != r2, "Edge end-points refiner ids are the same" );
           extra[ {{ std::min(r1,r2), std::max(r1,r2) }} ] =
             { local_needs_refining, local_lock_case };
        }
      }
    }
  }

  m_remoteEdgeData.clear();
  m_extra = extra.size();

  if (!extra.empty()) {
    // Do refinement including edges that need to be corrected
    if (m_mode == RefMode::DTDEREF) {
      m_refiner.mark_error_refinement_corr( extra, true );
    }
    else {
      m_refiner.mark_error_refinement_corr( extra );
    }
    // Update our extra-edge store based on refiner
    updateEdgeData();
  }

  // Aggregate number of extra edges that still need correction and some
  // refinement/derefinement statistics
  const auto& tet_store = m_refiner.tet_store;
  std::vector< std::size_t >
    m{ m_meshid,
       m_extra,
       tet_store.marked_refinements.size(),
       tet_store.marked_derefinements.size(),
       static_cast< std::underlying_type_t< RefMode > >( m_mode ) };
  contribute( m, CkReduction::sum_ulong, m_cbr.get< tag::matched >() );
}

void
Refiner::updateEdgeData()
// *****************************************************************************
// Query AMR lib and update our local store of edge data
// *****************************************************************************
{
  m_localEdgeData.clear();
  m_intermediates.clear();

  // This currently takes ALL edges from the AMR lib, i.e., on the whole
  // domain. We should eventually only collect edges here that are shared with
  // other chares.
  const auto& ref_edges = m_refiner.tet_store.edge_store.edges;
  const auto& refinpoel = m_refiner.tet_store.get_active_inpoel();

  for (std::size_t e=0; e<refinpoel.size()/4; ++e) {
    auto A = refinpoel[e*4+0];
    auto B = refinpoel[e*4+1];
    auto C = refinpoel[e*4+2];
    auto D = refinpoel[e*4+3];
    std::array<Edge,6> edges{{ {{A,B}}, {{B,C}}, {{A,C}},
                               {{A,D}}, {{B,D}}, {{C,D}} }};
    for (const auto& ed : edges) {
      auto ae = AMR::edge_t{{{ std::min(ed[0],ed[1]), std::max(ed[0],ed[1]) }}};
      auto r = tk::cref_find( ref_edges, ae );
      const auto ged = Edge{{ m_gid[ tk::cref_find( m_lref, ed[0] ) ],
                              m_gid[ tk::cref_find( m_lref, ed[1] ) ] }};
      m_localEdgeData[ ged ] = { r.needs_refining, r.lock_case };
    }
  }

  // Build intermediates to send. This currently takes ALL intermediates from
  // the AMR lib, i.e., on the whole domain. We should eventually only collect
  // edges here that are shared with other chares.
  for (const auto& r : m_refiner.tet_store.intermediate_list) {
     m_intermediates.insert( m_gid[ tk::cref_find( m_lref, r ) ] );
  }
}

std::tuple< std::vector< std::string >,
            std::vector< std::vector< tk::real > >,
            std::vector< std::string >,
            std::vector< std::vector< tk::real > > >
Refiner::refinementFields() const
// *****************************************************************************
//  Collect mesh output fields from refiner lib
//! \return Names and fields of mesh refinement data in mesh cells and nodes
// *****************************************************************************
{
  // Find number of nodes in current mesh
  auto npoin = tk::npoin_in_graph( m_inpoel );
  // Generate edges surrounding points in current mesh
  auto esup = tk::genEsup( m_inpoel, 4 );

  // Update solution on current mesh
  auto u = solution( npoin, esup );
  Assert( u.nunk() == npoin, "Solution uninitialized or wrong size" );

  // Compute error in edges on current mesh
  auto edgeError = errorsInEdges( npoin, esup, u );

  // Transfer error from edges to cells for field output
  std::vector< tk::real > error( m_inpoel.size()/4, 0.0 );
  for (std::size_t e=0; e<m_inpoel.size()/4; ++e) {
    auto A = m_inpoel[e*4+0];
    auto B = m_inpoel[e*4+1];
    auto C = m_inpoel[e*4+2];
    auto D = m_inpoel[e*4+3];
    std::array<Edge,6> edges{{ {{A,B}}, {{B,C}}, {{A,C}},
                               {{A,D}}, {{B,D}}, {{C,D}} }};
    // sum error from edges to elements
    for (const auto& ed : edges) error[e] += tk::cref_find( edgeError, ed );
    error[e] /= 6.0;    // assign edge-average error to element
  }

  // Prepare element fields with mesh refinement data
  std::vector< std::string >
    elemfieldnames{ "refinement level", "cell type", "error" };
  auto& tet_store = m_refiner.tet_store;
  std::vector< std::vector< tk::real > > elemfields{
    tet_store.get_refinement_level_list(),
    tet_store.get_cell_type_list(),
    error };

  using tuple_t = std::tuple< std::vector< std::string >,
                              std::vector< std::vector< tk::real > >,
                              std::vector< std::string >,
                              std::vector< std::vector< tk::real > > >;
  return tuple_t{ elemfieldnames, elemfields, {}, {} };
}

void
Refiner::writeMesh( const std::string& basefilename,
                    uint64_t itr,
                    tk::real t,
                    CkCallback c ) const
// *****************************************************************************
//  Output mesh to file(s)
//! \param[in] basefilename File name to append to
//! \param[in] itr Iteration count since a new mesh
//! \param[in] t "Physical time" to write to file. "Time" here is used to
//!   designate a new time step at which the mesh is saved.
//! \param[in] c Function to continue with after the write
// *****************************************************************************
{
  auto r = refinementFields();
  auto& elemfieldnames = std::get< 0 >( r );
  auto& elemfields = std::get< 1 >( r );
  auto& nodefieldnames = std::get< 2 >( r );
  auto& nodefields = std::get< 3 >( r );

  // Prepare solution field names: depvar + component id for all eqs
  auto nprop = g_inputdeck.get< tag::component >().nprop();
  auto solfieldnames = g_inputdeck.get<tag::component>().depvar( g_inputdeck );
  Assert( solfieldnames.size() == nprop, "Size mismatch" );

  const auto scheme = g_inputdeck.get< tag::discr, tag::scheme >();
  const auto centering = ctr::Scheme().centering( scheme );
  auto t0 = g_inputdeck.get< tag::discr, tag::t0 >();

  // list of nodes/elements at which box ICs are defined
  std::unordered_set< std::size_t > inbox;
  tk::real V = 1.0;

  // Prepare node or element fields for output to file
  if (centering == tk::Centering::NODE) {

    // Augment element field names with solution variable names + field ids
    nodefieldnames.insert( end(nodefieldnames),
                           begin(solfieldnames), end(solfieldnames) );

    // Evaluate initial conditions on current mesh at t0
    tk::Fields u( m_coord[0].size(), nprop );
    for (auto& eq : g_cgpde) eq.initialize( m_coord, u, t0, V, inbox );

    // Extract all scalar components from solution for output to file
    for (std::size_t i=0; i<nprop; ++i)
      nodefields.push_back( u.extract( i, 0 ) );

  } else if (centering == tk::Centering::ELEM) {

    // Augment element field names with solution variable names + field ids
    elemfieldnames.insert( end(elemfieldnames),
                           begin(solfieldnames), end(solfieldnames) );

    auto ndof = g_inputdeck.get< tag::discr, tag::ndof >();
    tk::Fields lhs( m_inpoel.size()/4, ndof*nprop );

    // Generate left hand side for DG initialize
    auto geoElem = tk::genGeoElemTet( m_inpoel, m_coord );
    for (const auto& eq : g_dgpde) eq.lhs( geoElem, lhs );

    // Evaluate initial conditions on current mesh at t0
    auto u = lhs;
    for (const auto& eq : g_dgpde)
      eq.initialize( lhs, m_inpoel, m_coord, inbox, u, t0, m_inpoel.size()/4 );

    // Extract all scalar components from solution for output to file
    for (std::size_t i=0; i<nprop; ++i)
      elemfields.push_back( u.extract( i, 0 ) );
  }

  // Output mesh
  m_meshwriter[ CkNodeFirst( CkMyNode() ) ].
    write( m_meshid, /*meshoutput = */ true, /*fieldoutput = */ true, itr, 1, t,
           thisIndex, basefilename, m_inpoel, m_coord, m_bface,
           tk::remap(m_bnode,m_lid), tk::remap(m_triinpoel,m_lid),
           elemfieldnames, nodefieldnames, {}, elemfields, nodefields, {},
           {}, c );
}

void
Refiner::perform()
// *****************************************************************************
// Perform mesh refinement and decide how to continue
//! \details First the mesh refiner object is called to perform a single step
//!   of mesh refinement. Then, if this function is called during a step
//!   (potentially multiple levels of) initial AMR, it evaluates whether to do
//!   another one. If it is called during time stepping, this concludes the
//!   single mesh refinement step and the new mesh is sent to the PDE worker
//!   (Discretization).
// *****************************************************************************
{
  // Save old tets and their ids before performing refinement. Outref is always
  // followed by outderef, so to the outside world, the mesh is uchanged, thus
  // no update.
  if (m_mode != RefMode::OUTREF && m_mode != RefMode::OUTDEREF) {
    m_oldntets = m_oldTets.size();
    m_oldTets.clear();
    for (const auto& [ id, tet ] : m_refiner.tet_store.tets)
      m_oldTets.insert( tet );
  }

  if (m_mode == RefMode::T0REF) {

    // Refine mesh based on next initial refinement type
    if (!m_initref.empty()) {
      auto r = m_initref.back();    // consume (reversed) list from its back
      if (r == ctr::AMRInitialType::UNIFORM_DEREFINE)
        m_refiner.perform_derefinement();
      else
        m_refiner.perform_refinement();
    }

  }
  // TODO: dtref is not completely tested yet; verify that the following works
  else if (m_mode == RefMode::DTDEREF) {
    m_refiner.perform_derefinement();
  }
  else {
    m_refiner.perform_refinement();
  }

  //auto& tet_store = m_refiner.tet_store;
  //std::cout << "before ref: " << tet_store.marked_refinements.size() << ", " << tet_store.marked_derefinements.size() << ", " << tet_store.size() << ", " << tet_store.get_active_inpoel().size() << '\n';
  //std::cout << "after ref: " << tet_store.marked_refinements.size() << ", " << tet_store.marked_derefinements.size() << ", " << tet_store.size() << ", " << tet_store.get_active_inpoel().size() << '\n';
  //std::cout << "after deref: " << tet_store.marked_refinements.size() << ", " << tet_store.marked_derefinements.size() << ", " << tet_store.size() << ", " << tet_store.get_active_inpoel().size() << '\n';

  // Update volume and boundary mesh
  updateMesh();

  // Save mesh at every initial refinement step (mainly for debugging). Will
  // replace with just a 'next()' in production.
  if (m_mode == RefMode::T0REF) {

    auto l = m_ninitref - m_initref.size() + 1;  // num initref steps completed
    auto t0 = g_inputdeck.get< tag::discr, tag::t0 >();
    // Generate times equally subdividing t0-1...t0 to ninitref steps
    auto t =
      t0 - 1.0 + static_cast<tk::real>(l)/static_cast<tk::real>(m_ninitref);
    auto itr = l;
    // Output mesh after refinement step
    writeMesh( "t0ref", itr, t,
               CkCallback( CkIndex_Refiner::next(), thisProxy[thisIndex] ) );

  } else {

    next();

  }
}

void
Refiner::next()
// *****************************************************************************
// Continue after finishing a refinement step
// *****************************************************************************
{
  if (m_mode == RefMode::T0REF) {

    // Remove initial mesh refinement step from list
    if (!m_initref.empty()) m_initref.pop_back();
    // Continue to next initial AMR step or finish
    if (!m_initref.empty()) t0ref(); else endt0ref();

  } else if (m_mode == RefMode::DTREF) {

    // Augment node communication map with newly added nodes on chare-boundary
    for (const auto& [ neighborchare, edges ] : m_remoteEdges) {
      auto& nodes = tk::ref_find( m_nodeCommMap, neighborchare );
      for (const auto& e : edges) {
        // If parent nodes were part of the node communication map for chare
        if (nodes.find(e[0]) != end(nodes) && nodes.find(e[1]) != end(nodes)) {
          // Add new node if local id was generated for it
          auto n = Hash<2>()( e );
          if (m_lid.find(n) != end(m_lid)) nodes.insert( n );
        }
      }
    }

    // Follow up with a dt-derefinement step
    dtref( m_outref_bface, m_outref_bnode, m_outref_triinpoel,
           RefMode::DTDEREF );

  } else if (m_mode == RefMode::DTDEREF) {

    //TODO: Is the above chunk of code "Augment node communication map ..." needed here as well?
    // Send new mesh, solution, and communication data back to PDE worker
    m_scheme[m_meshid].ckLocal< Scheme::resizePostAMR >( thisIndex,  m_ginpoel,
      m_el, m_coord, m_addedNodes, m_addedTets, m_nodeCommMap, m_bface, m_bnode,
      m_triinpoel );

  } else if (m_mode == RefMode::OUTREF) {

    // Augment node communication map with newly added nodes on chare-boundary
    for (const auto& [ neighborchare, edges ] : m_remoteEdges) {
      auto& nodes = tk::ref_find( m_nodeCommMap, neighborchare );
      for (const auto& e : edges) {
        // If parent nodes were part of the node communication map for chare
        if (nodes.find(e[0]) != end(nodes) && nodes.find(e[1]) != end(nodes)) {
          // Add new node if local id was generated for it
          auto n = Hash<2>()( e );
          if (m_lid.find(n) != end(m_lid)) nodes.insert( n );
        }
      }
    }

    // Store field output mesh
    m_outref_ginpoel = m_ginpoel;
    m_outref_el = m_el;
    m_outref_coord = m_coord;
    m_outref_addedNodes = m_addedNodes;
    m_outref_addedTets = m_addedTets;
    m_outref_nodeCommMap = m_nodeCommMap;
    m_outref_bface = m_bface;
    m_outref_bnode = m_bnode;
    m_outref_triinpoel = m_triinpoel;

    // Derefine mesh to the state previous to field output
    outref( m_outref_bface, m_outref_bnode, m_outref_triinpoel, m_writeCallback,
            RefMode::OUTDEREF );

  } else if (m_mode == RefMode::OUTDEREF) {

    // Send field output mesh to PDE worker
    m_scheme[m_meshid].ckLocal< Scheme::extractFieldOutput >( thisIndex,
      m_outref_ginpoel, m_outref_el, m_outref_coord, m_outref_addedNodes,
      m_outref_addedTets, m_outref_nodeCommMap, m_outref_bface, m_outref_bnode,
      m_outref_triinpoel, m_writeCallback );

  } else Throw( "RefMode not implemented" );
}

void
Refiner::endt0ref()
// *****************************************************************************
// Finish initial mesh refinement
//! \details This function is called as after initial mesh refinement has
//!   finished. If initial mesh reifnement was not configured by the user, this
//!   is the point where we continue after the constructor, by computing the
//!   total number of elements across the whole problem.
// *****************************************************************************
{
  // create sorter Charm++ chare array elements using dynamic insertion
  m_sorter[ thisIndex ].insert( m_meshid, m_host, m_meshwriter, m_cbs, m_scheme,
    CkCallback(CkIndex_Refiner::reorder(), thisProxy[thisIndex]), m_ginpoel,
    m_coordmap, m_bface, m_triinpoel, m_bnode, m_nchare );

  // Compute final number of cells across whole problem
  std::vector< std::size_t >
    meshdata{ m_meshid, m_ginpoel.size()/4, m_coord[0].size() };
  contribute( meshdata, CkReduction::sum_ulong, m_cbr.get< tag::refined >() );

  // // Free up memory if no dtref
  // if (!g_inputdeck.get< tag::amr, tag::dtref >()) {
  //   tk::destroy( m_ginpoel );
  //   tk::destroy( m_el );
  //   tk::destroy( m_coordmap );
  //   tk::destroy( m_coord );
  //   tk::destroy( m_bface );
  //   tk::destroy( m_bnode );
  //   tk::destroy( m_triinpoel );
  //   tk::destroy( m_initref );
  //   tk::destroy( m_ch );
  //   tk::destroy( m_edgech );
  //   tk::destroy( m_chedge );
  //   tk::destroy( m_localEdgeData );
  //   tk::destroy( m_remoteEdgeData );
  //   tk::destroy( m_remoteEdges );
  //   tk::destroy( m_intermediates );
  //   tk::destroy( m_nodeCommMap );
  //   tk::destroy( m_oldTets );
  //   tk::destroy( m_addedNodes );
  //   tk::destroy( m_addedTets );
  //   tk::destroy( m_coarseBndFaces );
  //   tk::destroy( m_coarseBndNodes );
  //   tk::destroy( m_rid );
  //   tk::destroy( m_oldrid );
  //   tk::destroy( m_lref );
  //   tk::destroy( m_parent );
  // }
}

void
Refiner::uniformRefine()
// *****************************************************************************
// Do uniform mesh refinement
// *****************************************************************************
{
  // Do uniform refinement
  m_refiner.mark_uniform_refinement();

  // Update our extra-edge store based on refiner
  updateEdgeData();

  // Set number of extra edges to be zero, skipping correction (if this is the
  // only step in this refinement step)
  m_extra = 0;
}

void
Refiner::uniformDeRefine()
// *****************************************************************************
// Do uniform mesh derefinement
// *****************************************************************************
{
  // Do uniform derefinement
  m_refiner.mark_uniform_derefinement();

  // Update our extra-edge store based on refiner
  updateEdgeData();

  // Set number of extra edges to be zero, skipping correction (if this is the
  // only step in this refinement step)
  m_extra = 0;
}

Refiner::EdgeError
Refiner::errorsInEdges(
  std::size_t npoin,
  const std::pair< std::vector<std::size_t>, std::vector<std::size_t> >& esup,
  const tk::Fields& u ) const
// *****************************************************************************
//  Compute errors in edges
//! \param[in] npoin Number nodes in current mesh (partition)
//! \param[in] esup Elements surrounding points linked vectors
//! \param[in] u Solution evaluated at mesh nodes for all scalar components
//! \return A map associating errors (real values between 0.0 and 1.0 incusive)
//!   to edges (2 local node IDs)
// *****************************************************************************
{
  // Get the indices (in the system of systems) of refinement variables and the
  // error indicator configured
  const auto& refidx = g_inputdeck.get< tag::amr, tag::id >();
  auto errtype = g_inputdeck.get< tag::amr, tag::error >();

  // Compute points surrounding points
  auto psup = tk::genPsup( m_inpoel, 4, esup );

  // Compute errors in ICs and define refinement criteria for edges
  AMR::Error error;
  EdgeError edgeError;

  for (std::size_t p=0; p<npoin; ++p) { // for all mesh nodes on this chare
    for (auto q : tk::Around(psup,p)) { // for all nodes surrounding p
      tk::real cmax = 0.0;
      AMR::edge_t e(p,q);
      for (auto i : refidx) {          // for all refinement variables
        auto c = error.scalar( u, e, i, m_coord, m_inpoel, esup, errtype );
        if (c > cmax) cmax = c;        // find max error at edge
      }
      edgeError[ {{p,q}} ] = cmax;       // associate error to edge
    }
  }

  return edgeError;
}

tk::Fields
Refiner::solution( std::size_t npoin,
                   const std::pair< std::vector< std::size_t >,
                                    std::vector< std::size_t > >& esup ) const
// *****************************************************************************
//  Update (or evaluate) solution on current mesh
//! \param[in] npoin Number nodes in current mesh (partition)
//! \param[in] esup Elements surrounding points linked vectors
//! \return Solution updated/evaluated for all scalar components
// *****************************************************************************
{
  // Get solution whose error to evaluate
  tk::Fields u;

  if (m_mode == RefMode::T0REF) {

    // Evaluate initial conditions at mesh nodes
    u = nodeinit( npoin, esup );

  } else if (m_mode == RefMode::DTREF || m_mode == RefMode::DTDEREF) {

    // Query current solution
    u = m_scheme[m_meshid].ckLocal< Scheme::solution >( thisIndex );
 
    const auto scheme = g_inputdeck.get< tag::discr, tag::scheme >();
    const auto centering = ctr::Scheme().centering( scheme );
    if (centering == tk::Centering::ELEM) {

      // ...

    }

  } else if (m_mode == RefMode::OUTREF) {



  } else if (m_mode == RefMode::OUTDEREF) {



  } else Throw( "RefMode not implemented" );

  return u;
}

void
Refiner::errorRefine(RefMode mode)
// *****************************************************************************
// Do error-based mesh refinement or derefinement, as specified by ref-mode
//! \param[in] mode Refinement mode, defaults to RefMode::DTREF if unspecified
// *****************************************************************************
{
  // Find number of nodes in old mesh
  auto npoin = tk::npoin_in_graph( m_inpoel );
  // Generate edges surrounding points in old mesh
  auto esup = tk::genEsup( m_inpoel, 4 );

  // Update solution on current mesh
  auto u = solution( npoin, esup );
  Assert( u.nunk() == npoin, "Solution uninitialized or wrong size" );

  using AMR::edge_t;
  using AMR::edge_tag;

  // Compute error in edges. Tag edge for refinement if error exceeds
  // refinement tolerance, tag edge for derefinement if error is below
  // derefinement tolerance.
  auto tolref = g_inputdeck.get< tag::amr, tag::tolref >();
  auto tolderef = g_inputdeck.get< tag::amr, tag::tolderef >();
  std::vector< std::pair< edge_t, edge_tag > > tagged_edges;
  for (const auto& e : errorsInEdges(npoin,esup,u)) {
    if (mode == RefMode::DTREF && e.second > tolref) {
      tagged_edges.push_back( { edge_t( m_rid[e.first[0]], m_rid[e.first[1]] ),
                                edge_tag::REFINE } );
    } else if (mode == RefMode::DTDEREF && e.second < tolderef) {
      //TODO: be able to uncomment following code
      //tagged_edges.push_back( { edge_t( m_rid[e.first[0]], m_rid[e.first[1]] ),
      //                          edge_tag::DEREFINE } );
    }
  }

  // Do error-based refinement
  if (mode == RefMode::DTREF) {
    m_refiner.mark_error_refinement( tagged_edges );
  }
  else if (mode == RefMode::DTDEREF) {
    m_refiner.mark_error_refinement( tagged_edges, true );
  }

  // Update our extra-edge store based on refiner
  updateEdgeData();

  // Set number of extra edges to a nonzero number, triggering correction
  m_extra = 1;
}

void
Refiner::edgelistRefine()
// *****************************************************************************
// Do mesh refinement based on user explicitly tagging edges
// *****************************************************************************
{
  // Get user-defined node-pairs (edges) to tag for refinement
  const auto& edgenodelist = g_inputdeck.get< tag::amr, tag::edge >();

  if (!edgenodelist.empty()) {  // if user explicitly tagged edges
    // Find number of nodes in old mesh
    auto npoin = tk::npoin_in_graph( m_inpoel );
    // Generate edges surrounding points in old mesh
    auto esup = tk::genEsup( m_inpoel, 4 );
    auto psup = tk::genPsup( m_inpoel, 4, esup );

    EdgeSet useredges;
    for (std::size_t i=0; i<edgenodelist.size()/2; ++i)
      useredges.insert( {{ {edgenodelist[i*2+0], edgenodelist[i*2+1]} }} );

    using AMR::edge_t;
    using AMR::edge_tag;

    // Tag edges the user configured
    std::vector< std::pair< edge_t, edge_tag > > tagged_edges;
    for (std::size_t p=0; p<npoin; ++p)        // for all mesh nodes on this chare
      for (auto q : tk::Around(psup,p)) {      // for all nodes surrounding p
        Edge e{{ m_gid[p], m_gid[q] }};
        auto f = useredges.find(e);
        if (f != end(useredges)) { // tag edge if on user's list
          tagged_edges.push_back( { edge_t( m_rid[p], m_rid[q] ),
                                    edge_tag::REFINE } );
          useredges.erase( f );
        }
      }

    // Do error-based refinement
    m_refiner.mark_error_refinement( tagged_edges );

    // Update our extra-edge store based on refiner
    updateEdgeData();

    // Set number of extra edges to a nonzero number, triggering correction
    m_extra = 1;
  }
}

void
Refiner::coordRefine()
// *****************************************************************************
// Do mesh refinement based on tagging edges based on end-point coordinates
// *****************************************************************************
{
  // Get user-defined half-world coordinates
  auto xminus = g_inputdeck.get< tag::amr, tag::xminus >();
  auto xplus = g_inputdeck.get< tag::amr, tag::xplus >();
  auto yminus = g_inputdeck.get< tag::amr, tag::yminus >();
  auto yplus = g_inputdeck.get< tag::amr, tag::yplus >();
  auto zminus = g_inputdeck.get< tag::amr, tag::zminus >();
  auto zplus = g_inputdeck.get< tag::amr, tag::zplus >();

  // The default is the largest representable double
  auto eps =
    std::numeric_limits< kw::amr_xminus::info::expect::type >::epsilon();
  auto xminus_default = g_inputdeck_defaults.get< tag::amr, tag::xminus >();
  auto xplus_default = g_inputdeck_defaults.get< tag::amr, tag::xplus >();
  auto yminus_default = g_inputdeck_defaults.get< tag::amr, tag::yminus >();
  auto yplus_default = g_inputdeck_defaults.get< tag::amr, tag::yplus >();
  auto zminus_default = g_inputdeck_defaults.get< tag::amr, tag::zminus >();
  auto zplus_default = g_inputdeck_defaults.get< tag::amr, tag::zplus >();

  // Decide if user has configured the half-world
  bool xm = std::abs(xminus - xminus_default) > eps ? true : false;
  bool xp = std::abs(xplus - xplus_default) > eps ? true : false;
  bool ym = std::abs(yminus - yminus_default) > eps ? true : false;
  bool yp = std::abs(yplus - yplus_default) > eps ? true : false;
  bool zm = std::abs(zminus - zminus_default) > eps ? true : false;
  bool zp = std::abs(zplus - zplus_default) > eps ? true : false;

  using AMR::edge_t;
  using AMR::edge_tag;

  if (xm || xp || ym || yp || zm || zp) {       // if any half-world configured
    // Find number of nodes in old mesh
    auto npoin = tk::npoin_in_graph( m_inpoel );
    // Generate edges surrounding points in old mesh
    auto esup = tk::genEsup( m_inpoel, 4 );
    auto psup = tk::genPsup( m_inpoel, 4, esup );
    // Get access to node coordinates
    const auto& x = m_coord[0];
    const auto& y = m_coord[1];
    const auto& z = m_coord[2];
    // Compute edges to be tagged for refinement
    std::vector< std::pair< edge_t, edge_tag > > tagged_edges;
    for (std::size_t p=0; p<npoin; ++p)    // for all mesh nodes on this chare
      for (auto q : tk::Around(psup,p)) {  // for all nodes surrounding p
        Edge e{{p,q}};

        bool t = true;
        if (xm) { if (x[p]>xminus && x[q]>xminus) t = false; }
        if (xp) { if (x[p]<xplus && x[q]<xplus) t = false; }
        if (ym) { if (y[p]>yminus && y[q]>yminus) t = false; }
        if (yp) { if (y[p]<yplus && y[q]<yplus) t = false; }
        if (zm) { if (z[p]>zminus && z[q]>zminus) t = false; }
        if (zp) { if (z[p]<zplus && z[q]<zplus) t = false; }

        if (t) {
          tagged_edges.push_back( { edge_t( m_rid[e[0]], m_rid[e[1]] ),
                                    edge_tag::REFINE } );
        }
      }

    // Do error-based refinement
    m_refiner.mark_error_refinement( tagged_edges );

    // Update our extra-edge store based on refiner
    updateEdgeData();

    // Set number of extra edges to a nonzero number, triggering correction
    m_extra = 1;
  }
}

tk::Fields
Refiner::nodeinit( std::size_t npoin,
                   const std::pair< std::vector< std::size_t >,
                                    std::vector< std::size_t > >& esup ) const
// *****************************************************************************
// Evaluate initial conditions (IC) at mesh nodes
//! \param[in] npoin Number points in mesh (on this chare)
//! \param[in] esup Elements surrounding points as linked lists, see tk::genEsup
//! \return Initial conditions (evaluated at t0) at nodes
// *****************************************************************************
{
  auto t0 = g_inputdeck.get< tag::discr, tag::t0 >();
  auto nprop = g_inputdeck.get< tag::component >().nprop();

  // Will store nodal ICs
  tk::Fields u( m_coord[0].size(), nprop );

  // Evaluate ICs differently depending on nodal or cell-centered discretization
  const auto scheme = g_inputdeck.get< tag::discr, tag::scheme >();
  const auto centering = ctr::Scheme().centering( scheme );
  // list of nodes/elements at which box ICs are defined
  std::unordered_set< std::size_t > inbox;
  tk::real V = 1.0;

  if (centering == tk::Centering::NODE) {

    // Evaluate ICs for all scalar components integrated
    for (auto& eq : g_cgpde) eq.initialize( m_coord, u, t0, V, inbox );

  } else if (centering == tk::Centering::ELEM) {

    auto esuel = tk::genEsuelTet( m_inpoel, esup ); // elems surrounding elements
    // Initialize cell-based unknowns
    tk::Fields ue( m_inpoel.size()/4, nprop );
    auto lhs = ue;
    auto geoElem = tk::genGeoElemTet( m_inpoel, m_coord );
    for (const auto& eq : g_dgpde)
      eq.lhs( geoElem, lhs );
    for (const auto& eq : g_dgpde)
      eq.initialize( lhs, m_inpoel, m_coord, inbox, ue, t0, esuel.size()/4 );

    // Transfer initial conditions from cells to nodes
    for (std::size_t p=0; p<npoin; ++p) {    // for all mesh nodes on this chare
      std::vector< tk::real > up( nprop, 0.0 );
      tk::real vol = 0.0;
      for (auto e : tk::Around(esup,p)) {       // for all cells around node p
        // compute nodal volume: every element contributes their volume / 4
        vol += geoElem(e,0,0) / 4.0;
        // sum cell value to node weighed by cell volume / 4
        for (std::size_t c=0; c<nprop; ++c)
          up[c] += ue[e][c] * geoElem(e,0,0) / 4.0;
      }
      // store nodal value
      for (std::size_t c=0; c<nprop; ++c) u(p,c,0) = up[c] / vol;
    }

  } else Throw( "Scheme centring not handled for nodal initialization" );

  Assert( u.nunk() == m_coord[0].size(), "Size mismatch" );
  Assert( u.nprop() == nprop, "Size mismatch" );

  return u;
}

void
Refiner::updateMesh()
// *****************************************************************************
// Update old mesh after refinement
// *****************************************************************************
{
  // Get refined mesh connectivity
  const auto& refinpoel = m_refiner.tet_store.get_active_inpoel();
  Assert( refinpoel.size()%4 == 0, "Inconsistent refined mesh connectivity" );
  Assert( tk::conforming( m_inpoel, m_coord, true, m_rid ),
          "Mesh not conforming after refinement" );

  // Generate unique node lists of old and refined mesh using local ids
  auto rinpoel = m_inpoel;
  tk::remap( rinpoel, m_rid );
  std::unordered_set< std::size_t > old( begin(rinpoel), end(rinpoel) );
  std::unordered_set< std::size_t > ref( begin(refinpoel), end(refinpoel) );

  // Augment refiner id -> local node id map with newly added nodes
  std::size_t l = m_lref.size();
  for (auto r : ref) if (old.find(r) == end(old)) m_lref[r] = l++;

  // Get nodal communication map from Discretization worker
  if ( m_mode == RefMode::DTREF ||
       m_mode == RefMode::OUTREF ||
       m_mode == RefMode::OUTDEREF ) {
    m_nodeCommMap =
      m_scheme[m_meshid].disc()[thisIndex].ckLocal()->NodeCommMap();
  }

  // Update mesh and solution after refinement
  newVolMesh( old, ref );
  newBndMesh( ref );

  // Update mesh connectivity from refiner lib, remapping refiner to local ids
  m_inpoel = m_refiner.tet_store.get_active_inpoel();
  tk::remap( m_inpoel, m_lref );

  // Update mesh connectivity with new global node ids
  m_ginpoel = m_inpoel;
  Assert( tk::uniquecopy(m_ginpoel).size() == m_coord[0].size(),
          "Size mismatch" );
  tk::remap( m_ginpoel, m_gid );

  // Ensure valid mesh after refinement
  Assert( tk::positiveJacobians( m_inpoel, m_coord ),
          "Refined mesh cell Jacobian non-positive" );

  Assert( tk::conforming( m_inpoel, m_coord, true, m_rid ),
          "Mesh not conforming after updating mesh after mesh refinement" );

  // Perform leak test on new mesh
  Assert( !tk::leakyPartition(
            tk::genEsuelTet( m_inpoel, tk::genEsup(m_inpoel,4) ),
            m_inpoel, m_coord ),
          "Refined mesh partition leaky" );
}

void
Refiner::newVolMesh( const std::unordered_set< std::size_t >& old,
                     const std::unordered_set< std::size_t >& ref )
// *****************************************************************************
//  Compute new volume mesh after mesh refinement
//! \param[in] old Unique nodes of the old (unrefined) mesh using local ids
//! \param[in] ref Unique nodes of the refined mesh using local ids
// *****************************************************************************
{
  const auto& x = m_coord[0];
  const auto& y = m_coord[1];
  const auto& z = m_coord[2];

  // Generate coordinates and ids to newly added nodes after refinement
  std::unordered_map< std::size_t, std::size_t > gid_add;
  tk::destroy( m_addedNodes );
  for (auto r : ref) {               // for all unique nodes of the refined mesh
    if (old.find(r) == end(old)) {   // if node is newly added
      // get (local) parent ids of newly added node
      auto p = m_refiner.node_connectivity.get( r );
      Assert( old.find(p[0]) != end(old) && old.find(p[1]) != end(old),
              "Parent(s) not in old mesh" );
      Assert( r >= old.size(), "Attempting to overwrite node with added one" );
      // local parent ids
      decltype(p) lp{{tk::cref_find(m_lref,p[0]), tk::cref_find(m_lref,p[1])}};
      // global parent ids
      decltype(p) gp{{m_gid[lp[0]], m_gid[lp[1]]}};
      // generate new global ID for newly added node
      auto g = Hash<2>()( gp );

      // if node added by AMR lib has not yet been added to Refiner's new mesh
      if (m_coordmap.find(g) == end(m_coordmap)) {
        Assert( g >= old.size(), "Hashed id overwriting old id" );
        Assert( m_lid.find(g) == end(m_lid),
                "Overwriting entry global->local node ID map" );
        Assert( m_coordmap.find(g) == end(m_coordmap),
                "Overwriting entry already in coordmap" );
        auto l = tk::cref_find( m_lref, r );
        // store newly added node id and their parent ids (local ids)
        m_addedNodes[r] = lp;   // key = r for later update to local
        // assign new node to refiner->global map
        gid_add[r] = g; // key = r for later search
        // assign new node to global->local map
        m_lid[g] = l;
        // generate and store coordinates for newly added node
        m_coordmap.insert( {g, {{ (x[lp[0]] + x[lp[1]])/2.0,
                                  (y[lp[0]] + y[lp[1]])/2.0,
                                  (z[lp[0]] + z[lp[1]])/2.0 }} } );
      }
    }
  }
  tk::destroy( m_coord );

  // Remove coordinates and ids of removed nodes due to derefinement
  std::unordered_map< std::size_t, std::size_t > gid_rem;
  for (auto o : old) {               // for all unique nodes of the old mesh
    if (ref.find(o) == end(ref)) {   // if node is no longer in new mesh
      auto l = tk::cref_find( m_lref, o );
      auto g = m_gid[l];
      gid_rem[l] = g;
      m_lid.erase( g );
      m_coordmap.erase( g );
    }
  }

  // Save previous states of refiner-local node id maps before update
  m_oldrid = m_rid;
  //m_oldlref = m_lref;

  // Generate new node id maps for nodes kept
  tk::destroy( m_lref );
  std::vector< std::size_t > rid( ref.size() );
  std::vector< std::size_t > gid( ref.size() );
  std::size_t l = 0;    // will generate new local node id
  for (std::size_t i=0; i<m_gid.size(); ++i) {
    if (gid_rem.find(i) == end(gid_rem)) {
      gid[l] = m_gid[i];
      rid[l] = m_rid[i];
      m_lref[ m_rid[i] ] = l;
      ++l;
    }
  }
  // Add newly added nodes due to refinement to node id maps
  decltype(m_addedNodes) addedNodes( m_addedNodes.size() );
  for (const auto& n : gid_add) {
    auto r = n.first;
    auto g = n.second;
    gid[l] = g;
    rid[l] = r;
    m_lref[r] = l;
    auto it = m_addedNodes.find( r );
    Assert( it != end(m_addedNodes), "Cannot find added node" );
    addedNodes[l] = std::move(it->second);
    ++l;
  }
  Assert( m_lref.size() == ref.size(), "Size mismatch" );
  m_rid = std::move( rid );
  m_addedNodes = std::move( addedNodes );

  // Update node coordinates, ids, and id maps
  auto& rx = m_coord[0];
  auto& ry = m_coord[1];
  auto& rz = m_coord[2];
  rx.resize( ref.size() );
  ry.resize( ref.size() );
  rz.resize( ref.size() );
  for (std::size_t i=0; i<gid.size(); ++i) {
    tk::ref_find( m_lid, gid[i] ) = i;
    const auto& c = tk::cref_find( m_coordmap, gid[i] );
    rx[i] = c[0];
    ry[i] = c[1];
    rz[i] = c[2];
  }
  m_gid = std::move( gid );
  Assert( m_gid.size() == m_lid.size(), "Size mismatch" );
}

std::unordered_set< std::size_t >
Refiner::ancestors( std::size_t n )
// *****************************************************************************
// Find the oldest parents of a mesh node in the AMR hierarchy
//! \param[in] n Local node id whose ancestors to search
//! \return Parents of local node id from the coarsest (original) mesh
// *****************************************************************************
{
  auto d = m_refiner.node_connectivity.get( m_rid[n] );
  decltype(d) p{{ tk::cref_find( m_lref, d[0] ),
                  tk::cref_find( m_lref, d[1] ) }};

  std::unordered_set< std::size_t > s;

  if (p != AMR::node_pair_t{{n,n}}) {
    auto q = ancestors( p[0] );
    s.insert( begin(q), end(q) );
    auto r = ancestors( p[1] );
    s.insert( begin(r), end(r) );
  } else {
    s.insert( begin(p), end(p) );
  }

  return s;
}

Refiner::BndFaceData
Refiner::boundary()
// *****************************************************************************
//  Generate boundary data structures used to update refined/derefined boundary
//  faces and nodes of side sets
//! \return A tuple of boundary face data
//! \details The output of this function is used to regenerate physical boundary
//!   face and node data structures after refinement, see updateBndFaces() and
//!   updateBndNodes().
// *****************************************************************************
{
  // Generate the inverse of AMR's tet store
  std::unordered_map< Tet, std::size_t, Hash<4>, Eq<4> > invtets;
  for (const auto& [key, tet] : m_refiner.tet_store.tets)
    invtets[ tet ] = key;

  //std::cout << thisIndex << " invt: " << invtets.size() << '\n';
  //std::cout << thisIndex << " active inpoel size: " << m_refiner.tet_store.get_active_inpoel().size() << '\n';
  //std::cout << thisIndex << " marked derefinement size: " << m_refiner.tet_store.marked_derefinements.size() << '\n';

  // Generate data structure pcReFaceTets, that associates the id of a tet
  // adjacent to a refined boundary triangle face for all (physical and chare)
  // boundary faces in the old mesh (i.e., before the current
  // refinement/derefinement step). Also generate data structure pcDeFaceTets,
  // that associates the parent tetrahedron (given by four nodes) adjacent to a
  // derefined boundary face for all (physical and chare) boundary faces in the
  // old mesh (i.e., before the current refinement/derefinement step).
  std::unordered_map< Face, std::size_t, Hash<3>, Eq<3> > pcReFaceTets;
  std::unordered_map< Face, Tet, Hash<3>, Eq<3> > pcDeFaceTets;
  auto oldesuel = tk::genEsuelTet( m_inpoel, tk::genEsup(m_inpoel,4) );
  for (std::size_t e=0; e<oldesuel.size()/4; ++e) {
    auto m = e*4;
    for (std::size_t f=0; f<4; ++f) {
      if (oldesuel[m+f] == -1) {  // if a face does not have an adjacent tet
        Face b{{ m_ginpoel[ m+tk::lpofa[f][0] ],
                 m_ginpoel[ m+tk::lpofa[f][1] ],
                 m_ginpoel[ m+tk::lpofa[f][2] ] }};
        Assert( m_inpoel[m+0] < m_oldrid.size() &&
                m_inpoel[m+1] < m_oldrid.size() &&
                m_inpoel[m+2] < m_oldrid.size() &&
                m_inpoel[m+3] < m_oldrid.size(), "Indexing out of rid" );
        Tet t{{ m_oldrid[ m_inpoel[m+0] ], m_oldrid[ m_inpoel[m+1] ],
                m_oldrid[ m_inpoel[m+2] ], m_oldrid[ m_inpoel[m+3] ] }};
        //Tet t{{ m_inpoel[m+0], m_inpoel[m+1],
        //        m_inpoel[m+2], m_inpoel[m+3] }};
        // associate tet id to adjacent (physical or chare) boundary face
        auto i = invtets.find( t );
        if (i != end(invtets)) {
          pcReFaceTets[ b ] = i->second;
        } else {
          // find parent tet
          auto p = tk::cref_find( m_parent, t );
          // form all 4 faces of parent
          //auto A = p[0];
          //auto B = p[1];
          //auto C = p[2];
          //auto D = p[3];
          auto A = tk::cref_find( m_lref, p[0] );
          auto B = tk::cref_find( m_lref, p[1] );
          auto C = tk::cref_find( m_lref, p[2] );
          auto D = tk::cref_find( m_lref, p[3] );
          // assign parent tet faces to derefined child's face
          pcDeFaceTets[ b ] = {{ A, B, C, D }};
        }
      }
    }
  }

  // Generate child->parent tet and id maps after refinement/derefinement step
  tk::destroy( m_parent );
  m_addedTets.clear();
  std::size_t p = 0;
  std::size_t c = 0;
  const auto& tet_store = m_refiner.tet_store;
  for (const auto& t : tet_store.tets) {
    // query number of children of tet
    auto nc = tet_store.data( t.first ).children.size();
    for (decltype(nc) i=0; i<nc; ++i ) {      // for all child tets
      // get child tet id
      auto childtet = tet_store.get_child_id( t.first, i );
      auto ct = tet_store.tets.find( childtet );
      //auto cA = tk::cref_find( m_lref, ct->second[0] );
      //auto cB = tk::cref_find( m_lref, ct->second[1] );
      //auto cC = tk::cref_find( m_lref, ct->second[2] );
      //auto cD = tk::cref_find( m_lref, ct->second[3] );
      // get nodes of parent tet
      //auto pA = tk::cref_find( m_lref, t.second[0] );
      //auto pB = tk::cref_find( m_lref, t.second[1] );
      //auto pC = tk::cref_find( m_lref, t.second[2] );
      //auto pD = tk::cref_find( m_lref, t.second[3] );
      // assign parent tet to child tet
      //m_parent[ {{cA,cB,cC,cD}} ] = {{pA,pB,pC,pD}};
      m_parent[ ct->second ] = t.second; //{{pA,pB,pC,pD}};
      if (m_oldTets.find(ct->second) == end(m_oldTets)) {
        m_addedTets[ c++ ] = p - m_oldntets;
      }
    }
    ++p;
  }

  //std::cout << thisIndex << " added: " << m_addedTets.size() << '\n';
  //std::cout << thisIndex << " parent: " << m_parent.size() << '\n';
  //std::cout << thisIndex << " pcret: " << pcReFaceTets.size() << '\n';
  //std::cout << thisIndex << " pcdet: " << pcDeFaceTets.size() << '\n';

  // Generate unique set of faces for each side set
  std::unordered_map< int, FaceSet > bndFaces;
  for (const auto& [ setid, faceids ] : m_bface) {
    auto& faces = bndFaces[ setid ];
    for (auto f : faceids) {
      faces.insert(
        {{{ m_triinpoel[f*3+0], m_triinpoel[f*3+1], m_triinpoel[f*3+2] }}} );
    }
  }

  return BndFaceData{ pcReFaceTets, pcDeFaceTets, bndFaces };
}

void
Refiner::newBndMesh( const std::unordered_set< std::size_t >& ref )
// *****************************************************************************
// Update boundary data structures after mesh refinement
//! \param[in] ref Unique nodes of the refined mesh using local ids
// *****************************************************************************
{
  // Generate boundary face data structures used to regenerate boundary face
  // and node data after mesh refinement
  auto bnd = boundary();

  // Regerate boundary faces and nodes after mesh refinement
  updateBndFaces( ref, bnd );
  updateBndNodes( ref, bnd );
}

void
Refiner::updateBndFaces(
  [[maybe_unused]] const std::unordered_set< std::size_t >& ref,
  const BndFaceData& bnd )
// *****************************************************************************
// Regenerate boundary faces after mesh refinement step
//! \param[in] ref Unique nodes of the refined mesh using local ids
//! \param[in] bnd Boundary face data bundle
// *****************************************************************************
{
  // storage for boundary faces associated to side-set IDs of the refined mesh
  tk::destroy( m_bface );
  // storage for boundary faces-node connectivity of the refined mesh
  tk::destroy( m_triinpoel );

  // face id counter
  std::size_t facecnt = 0;
  // will collect unique faces added for each side set
  std::unordered_map< int, FaceSet > bf;

  // Lambda to associate a boundary face and connectivity to a side set.
  // Argument 's' is the list of faces (ids) to add the new face to. Argument
  // 'ss' is the side set id to which the face is added. Argument 'f' is the
  // triangle face connectivity to add.
  auto addBndFace = [&]( std::vector< std::size_t >& s, int ss, const Face& f )
  {
    // only add face if it has not yet been aded to this side set
    if (bf[ ss ].insert( f ).second) {
      s.push_back( facecnt++ );
      m_triinpoel.insert( end(m_triinpoel), begin(f), end(f) );
    }
  };

  // Regenerate boundary faces for refined tets along side sets. For all
  // refined faces associated to side sets, we find the ancestors (parents of
  // nodes in the original/coarsest mesh) of the nodes comprising the faces of
  // the tetrahedron adjacent to the refined face.
  const auto& pcReFaceTets = std::get< 0 >( bnd );
  const auto& bndFaces = std::get< 2 >( bnd );
  const auto& tet_store = m_refiner.tet_store;
  for (const auto& [ face, tetid ] : pcReFaceTets) {
    // for all side sets of the face, match children's faces to side sets
    for (const auto& ss : keys(bndFaces,face)) {
      // will associate to side set id of old (unrefined) mesh boundary face
      auto& faces = m_bface[ ss ];
      const auto& coarsefaces = tk::cref_find( m_coarseBndFaces, ss );
      // query number of children of boundary tet adjacent to boundary face
      auto nc = tet_store.data( tetid ).children.size();
      if (nc == 0) {    // if boundary tet is not refined, add its boundary face
        addBndFace( faces, ss, face );
      } else {          // if boundary tet is refined
        const auto& tets = tet_store.tets;
        for (decltype(nc) i=0; i<nc; ++i ) {      // for all child tets
          // get child tet id
          auto childtet = tet_store.get_child_id( tetid, i );
          auto ct = tets.find( childtet );
          Assert( ct != end(tets), "Child tet not found" );
          // ensure all nodes of child tet are in refined mesh
          Assert( std::all_of( begin(ct->second), end(ct->second),
                    [&]( std::size_t n ){ return ref.find(n) != end(ref); } ),
                  "Boundary child tet node id not found in refined mesh" );
          // get nodes of child tet
          auto A = tk::cref_find( m_lref, ct->second[0] );
          auto B = tk::cref_find( m_lref, ct->second[1] );
          auto C = tk::cref_find( m_lref, ct->second[2] );
          auto D = tk::cref_find( m_lref, ct->second[3] );
          // form all 4 faces of child tet
          std::array<Face,4> g{{{{A,C,B}}, {{A,B,D}}, {{A,D,C}}, {{B,C,D}}}};
          // search all faces of child tet and match them to side sets of the
          // original (coarsest) mesh
          for (const auto& rf : g) {   // for all faces of child tet
            auto a = ancestors( rf[0] );
            auto b = ancestors( rf[1] );
            auto c = ancestors( rf[2] );
            a.insert( begin(b), end(b) );
            a.insert( begin(c), end(c) );
            if (a.size() == 3) {
              std::vector< std::size_t > p( begin(a), end(a) );
              Face par{{ m_gid[p[0]], m_gid[p[1]], m_gid[p[2]] }};
              auto it = coarsefaces.find( par );
              if (it != end(coarsefaces))
                addBndFace(faces,ss,{{m_gid[rf[0]],m_gid[rf[1]],m_gid[rf[2]]}});
            }
          }
        }
      }
    }
  }

  // Regenerate boundary faces for derefined tets along side sets. For all
  // derefined faces associated to side sets, we find the ancestors (parents of
  // nodes in the original/coarsest mesh) of the nodes comprising the faces of
  // the parent tetrahedron (previously) associated to the derefined face.
  const auto& pcDeFaceTets = std::get< 1 >( bnd );
  for (const auto& f : pcDeFaceTets) {
    for (const auto& ss : keys(bndFaces,f.first)) {
      // will associate to side set id of old (refined) mesh boundary face
      auto& faces = m_bface[ ss ];
      const auto& coarsefaces = tk::cref_find( m_coarseBndFaces, ss );
      // form all 4 faces of parent tet
      auto A = f.second[0];
      auto B = f.second[1];
      auto C = f.second[2];
      auto D = f.second[3];
      std::array<Face,4> parf{{ {{A,C,B}}, {{A,B,D}}, {{A,D,C}}, {{B,C,D}} }};
      for (const auto& pf : parf) {
        auto a = ancestors( pf[0] );
        auto b = ancestors( pf[1] );
        auto c = ancestors( pf[2] );
        a.insert( begin(b), end(b) );
        a.insert( begin(c), end(c) );
        if (a.size() == 3) {
          std::vector< std::size_t > p( begin(a), end(a) );
          Face par{{ m_gid[p[0]], m_gid[p[1]], m_gid[p[2]] }};
          auto it = coarsefaces.find( par );
          if (it != end(coarsefaces))
            addBndFace(faces,ss,{{m_gid[pf[0]],m_gid[pf[1]],m_gid[pf[2]]}});
        }
      }
    }
  }

  //std::cout << thisIndex << " bf: " << tk::sumvalsize( m_bface ) << '\n';

  // Perform leak-test on boundary face data just updated (only in DEBUG)
  Assert( bndIntegral(), "Partial boundary integral" );
}

bool
Refiner::bndIntegral()
// *****************************************************************************
//  Compute partial boundary surface integral and sum across all chares
//! \return true so we don't trigger assert in client code
//! \details This function computes a partial surface integral over the boundary
//!   of the faces of this mesh partition then sends its contribution to perform
//!   the integral acorss the total problem boundary. After the global sum a
//!   non-zero vector result indicates a leak, e.g., a hole in the boundary
//!   which indicates an error in the boundary face data structures used to
//!   compute the partial surface integrals.
// *****************************************************************************
{
  const auto& x = m_coord[0];
  const auto& y = m_coord[1];
  const auto& z = m_coord[2];

  std::vector< tk::real > s{{ 0.0, 0.0, 0.0 }};

  for (const auto& [ setid, faceids ] : m_bface) {
    for (auto f : faceids) {
      auto A = tk::cref_find( m_lid, m_triinpoel[f*3+0] );
      auto B = tk::cref_find( m_lid, m_triinpoel[f*3+1] );
      auto C = tk::cref_find( m_lid, m_triinpoel[f*3+2] );
      // Compute geometry data for face
      auto geoface = tk::geoFaceTri( {{x[A], x[B], x[C]}},
                                     {{y[A], y[B], y[C]}},
                                     {{z[A], z[B], z[C]}} );
      // Sum up face area * face unit-normal
      s[0] += geoface(0,0,0) * geoface(0,1,0);
      s[1] += geoface(0,0,0) * geoface(0,2,0);
      s[2] += geoface(0,0,0) * geoface(0,3,0);
    }
  }

  s.push_back( -1.0 );  // negative: no call-back after reduction
  s.push_back( static_cast< tk::real >( m_meshid ) );

  // Send contribution to host summing partial surface integrals
  contribute( s, CkReduction::sum_double, m_cbr.get< tag::bndint >() );

  return true;  // don't trigger the assert in client code
}

void
Refiner::updateBndNodes(
  [[maybe_unused]] const std::unordered_set< std::size_t >& ref,
  const BndFaceData& bnd )
// *****************************************************************************
// Update boundary nodes after mesh refinement
//! \param[in] ref Unique nodes of the refined mesh using local ids
//! \param[in] bnd Boundary face data bundle
// *****************************************************************************
{
  // storage for boundary nodes associated to side-set IDs of the refined mesh
  tk::destroy( m_bnode );

  // Lambda to search the parents in the coarsest mesh of a mesh node and if
  // found, add its global id to boundary node lists associated to the side
  // set(s) of its parents. Argument 'n' is the local id of the mesh node id
  // whose parents to search.
  auto search = [&]( std::size_t n ){
    auto a = ancestors( n );  // find parents of n in coarse mesh
    if (a.size() == 1) {
      // node was part of the coarse mesh
      auto ss = keys( m_coarseBndNodes, m_gid[*a.cbegin()] );
      for (auto s : ss)
        m_bnode[ s ].push_back( m_gid[n] );
    } else if (a.size() == 2) {
      // node was added to an edge of a coarse face
      std::vector< std::size_t > p( begin(a), end(a) );
      auto ss1 = keys( m_coarseBndNodes, m_gid[p[0]] );
      auto ss2 = keys( m_coarseBndNodes, m_gid[p[1]] );
      for (auto s : ss1) {
        if (ss2.find(s) != end(ss2)) {
          m_bnode[ s ].push_back( m_gid[n] );
        }
      }
    } else if (a.size() == 3) {
      // node was added inside of a coarse face
      std::vector< std::size_t > p( begin(a), end(a) );
      auto ss1 = keys( m_coarseBndNodes, m_gid[p[0]] );
      auto ss2 = keys( m_coarseBndNodes, m_gid[p[1]] );
      auto ss3 = keys( m_coarseBndNodes, m_gid[p[2]] );
      for (auto s : ss1) {
        if (ss2.find(s) != end(ss2) && ss3.find(s) != end(ss3)) {
          m_bnode[ s ].push_back( m_gid[n] );
        }
      }
    }
  };

  const auto& pcReFaceTets = std::get< 0 >( bnd );

  // Regenerate boundary node lists for refined tets along side sets. For all
  // refined faces associated to side sets, we find the ancestors (parents of
  // nodes in the original/coarsest mesh) of the nodes comprising the nodes of
  // the tetrahedron adjacent to the refined face.
  const auto& tet_store = m_refiner.tet_store;
  for (const auto& [ face, tetid ] : pcReFaceTets) {
    // query number of children of boundary tet adjacent to boundary face
    auto nc = tet_store.data( tetid ).children.size();
    if (nc == 0) {
      // if boundary tet is not refined, add its boundary nodes to the side
      // set(s) of their parent (in coarse mesh) nodes
      auto t = face;
      for (auto& n : t) n = tk::cref_find( m_lid, n );
      addBndNodes( t, search );
    } else {        // if boundary tet is refined
      const auto& tets = tet_store.tets;
      for (decltype(nc) i=0; i<nc; ++i ) {      // for all child tets
        // get child tet id
        auto childtet = tet_store.get_child_id( tetid, i );
        auto ct = tets.find( childtet );
        Assert( ct != end(tets), "Child tet not found" );
        // ensure all nodes of child tet are in refined mesh
        Assert( std::all_of( begin(ct->second), end(ct->second),
                  [&]( std::size_t n ){ return ref.find(n) != end(ref); } ),
                "Boundary child tet node id not found in refined mesh" );
        // search each child tet of refined boundary tet and add their boundary
        // nodes to the side set(s) of their parent (in coarse mesh) nodes
        auto A = tk::cref_find( m_lref, ct->second[0] );
        auto B = tk::cref_find( m_lref, ct->second[1] );
        auto C = tk::cref_find( m_lref, ct->second[2] );
        auto D = tk::cref_find( m_lref, ct->second[3] );
        addBndNodes( Tet{{A,B,C,D}}, search );
      }
    }
  }

  const auto& pcDeFaceTets = std::get< 1 >( bnd );

  // Regenerate boundary node lists for derefined tets along side sets. For all
  // refined faces associated to side sets, we find the ancestors (parents of
  // nodes in the original/coarsest mesh) of the nodes comprising the nodes of
  // the parent tetrahedron (previously) associated to the derefined face.
  for (const auto& f : pcDeFaceTets) addBndNodes( f.second, search );

  // Make boundary node IDs unique for each physical boundary (side set)
  for (auto& s : m_bnode) tk::unique( s.second );

  //std::cout << thisIndex << " bn: " << tk::sumvalsize( m_bnode ) << '\n';
}

#include "NoWarning/refiner.def.h"

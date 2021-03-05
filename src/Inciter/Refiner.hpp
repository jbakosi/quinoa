// *****************************************************************************
/*!
  \file      src/Inciter/Refiner.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Mesh refiner for interfacing the mesh refinement library
  \details   Mesh refiner is a Charm++ chare array and is used to interface the
   mesh refinement object which does not know about parallelization and thus the
   distributed nature of the mesh it operates on, i.e., it operates on mesh
   chunks. Thus it does not do parallel communication and also does not know
   about global vs local IDs. Instead this Charm++ chare array is the one that
   does all parallel computing aspects, i.e., communcation, and using the mesh
   refiner object as a library.
*/
// *****************************************************************************
#ifndef Refiner_h
#define Refiner_h

#include <vector>
#include <unordered_map>

#include "PUPAMR.hpp"
#include "AMR/mesh_adapter.hpp"
#include "Inciter/Options/AMRInitial.hpp"
#include "TaggedTuple.hpp"
#include "Tags.hpp"
#include "Callback.hpp"
#include "UnsMesh.hpp"
#include "Base/Fields.hpp"
#include "Scheme.hpp"
#include "DiagCG.hpp"
#include "ALECG.hpp"
#include "DG.hpp"
#include "CommMap.hpp"

#include "NoWarning/transporter.decl.h"
#include "NoWarning/refiner.decl.h"

namespace inciter {

//! Mesh refiner for interfacing the mesh refinement library
class Refiner : public CBase_Refiner {

  private:
    using Edge = tk::UnsMesh::Edge;
    using Face = tk::UnsMesh::Face;
    using Tet = tk::UnsMesh::Tet;
    using EdgeSet = tk::UnsMesh::EdgeSet;
    using FaceSet = tk::UnsMesh::FaceSet;
    using TetSet = tk::UnsMesh::TetSet;
    template< std::size_t N > using Hash = tk::UnsMesh::Hash< N >;
    template< std::size_t N > using Eq = tk::UnsMesh::Eq< N >;

    //! Boundary face data bundle, see boundary()
    using BndFaceData = std::tuple<
      std::unordered_map< Face, std::size_t, Hash<3>, Eq<3> >,
      std::unordered_map< Face, Tet, Hash<3>, Eq<3> >,
      std::unordered_map< int, FaceSet >
    >;

    //! Used to associate error to edges
    using EdgeError = std::unordered_map< Edge, tk::real, Hash<2>, Eq<2> >;

  public:
    //! Mode of operation: the way Refiner is used
    enum class RefMode : std::size_t {
      T0REF = 1,        //!< Initial (t<0) refinement
      DTREF,            //!< During time stepping (t>0)
      OUTREF,           //!< Refinement for field output
      OUTDEREF,         //!< De-refinement after field output
      DTDEREF };        //!< De-refinement during time stepping (t>0)

    //! Constructor
    explicit Refiner( std::size_t meshid,
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
                      int nchare );

    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
    #endif
    //! Migrate constructor
    // cppcheck-suppress uninitMemberVar
    explicit Refiner( CkMigrateMessage* ) {}
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #endif

    //! \brief Incoming query for a list boundary edges for which this chare
    //!   compiles shared edges
    void query( int fromch, const EdgeSet& edges );
    //! Receive receipt of boundary edge lists to quer
    void recvquery();
    //! Respond to boundary edge list queries
    void response();
    //! Receive shared boundary edges for our mesh chunk
    void bnd( int fromch, const std::vector< int >& chares );
    //! Receive receipt of shared boundary edges
    void recvbnd();

    //! Query Sorter and update local mesh with the reordered one
    void reorder();

    //! Start new step of initial mesh refinement/derefinement
    void start();

    //! Continue after finishing a refinemen/derefinementt step
    void next();

    //! Start mesh refinement (during time stepping, t>0)
    void dtref( const std::map< int, std::vector< std::size_t > >& bface,
                const std::map< int, std::vector< std::size_t > >& bnode,
                const std::vector< std::size_t >& triinpoel,
                RefMode mode = RefMode::DTREF );

    //! Start mesh refinement (for field output)
    void outref( const std::map< int, std::vector< std::size_t > >& bface,
                 const std::map< int, std::vector< std::size_t > >& bnode,
                 const std::vector< std::size_t >& triinpoel,
                 CkCallback c,
                 RefMode mode = RefMode::OUTREF );

    //! Do a single step of mesh refinemen/derefinementt (only tag edges)
    void refine();

    //! Receive newly added mesh edges and locks on our chare boundary
    void addRefBndEdges( int fromch,
                         const AMR::EdgeData& ed,
                         const std::unordered_set<size_t>& intermediates );

    //! Correct refinement to arrive at conforming mesh across chare boundaries
    void correctref();

    //! Communicate refined edges after a refinement/derefinement step
    void comExtra();

    //! Perform mesh refinement and decide how to continue
    void perform();

    //! Send Refiner proxy to Discretization objects
    void sendProxy();

    //! Get refinement field data in mesh cells
    std::tuple< std::vector< std::string >,
                std::vector< std::vector< tk::real > >,
                std::vector< std::string >,
                std::vector< std::vector< tk::real > > >
    refinementFields() const;

    /** @name Charm++ pack/unpack serializer member functions */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er &p ) override {
      p | m_meshid;
      p | m_host;
      p | m_sorter;
      p | m_meshwriter;
      p | m_scheme;
      p | m_cbr;
      p | m_cbs;
      p | m_ginpoel;
      p | m_el;
      if (p.isUnpacking()) {
        m_inpoel = std::get< 0 >( m_el );
        m_gid = std::get< 1 >( m_el );
        m_lid = std::get< 2 >( m_el );
      }
      p | m_coordmap;
      p | m_coord;
      p | m_bface;
      p | m_bnode;
      p | m_triinpoel;
      p | m_nchare;
      p | m_mode;
      p | m_initref;
      p | m_refiner;
      p | m_nref;
      p | m_nbnd;
      p | m_extra;
      p | m_ch;
      p | m_edgech;
      p | m_chedge;
      p | m_localEdgeData;
      p | m_remoteEdgeData;
      p | m_remoteEdges;
      p | m_intermediates;
      p | m_nodeCommMap;
      p | m_oldTets;
      p | m_addedNodes;
      p | m_addedTets;
      p | m_oldntets;
      p | m_coarseBndFaces;
      p | m_coarseBndNodes;
      p | m_rid;
      p | m_oldrid;
      p | m_lref;
      //p | m_oldlref;
      p | m_parent;
      p | m_writeCallback;
      p | m_outref_ginpoel;
      p | m_outref_el;
      p | m_outref_coord;
      p | m_outref_addedNodes;
      p | m_outref_addedTets;
      p | m_outref_nodeCommMap;
      p | m_outref_bface;
      p | m_outref_bnode;
      p | m_outref_triinpoel;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] r Refiner object reference
    friend void operator|( PUP::er& p, Refiner& r ) { r.pup(p); }
    //@}

  private:
    //! Mesh ID
    std::size_t m_meshid;
    //! Host proxy
    CProxy_Transporter m_host;
    //! Mesh sorter proxy
    CProxy_Sorter m_sorter;
    //! Mesh writer proxy
    tk::CProxy_MeshWriter m_meshwriter;
    //! Discretization schemes (one per mesh)
    std::vector< Scheme > m_scheme;
    //! Charm++ callbacks associated to compile-time tags for refiner
    tk::RefinerCallback m_cbr;
    //! Charm++ callbacks associated to compile-time tags for sorter
    tk::SorterCallback m_cbs;
    //! Tetrtahedron element connectivity of our chunk of the mesh (global ids)
    std::vector< std::size_t > m_ginpoel;
    //! Elements of the mesh chunk we operate on
    //! \details The first vector is the element connectivity (local IDs), the
    //!   second vector is the global node IDs of owned elements, while the
    //!   third one is a map of global->local node IDs.
    tk::UnsMesh::Chunk m_el;
    //! Alias to element connectivity with local node IDs in m_el
    std::vector< std::size_t >& m_inpoel = std::get<0>( m_el );
    //! Alias to global node IDs of owned elements in m_el
    std::vector< std::size_t >& m_gid = std::get<1>( m_el );
    //! \brief Alias to local node IDs associated to the global ones of owned
    //!    elements in m_el
    std::unordered_map< std::size_t, std::size_t >& m_lid = std::get<2>( m_el );
    //! Coordinates associated to global node IDs of our mesh chunk
    tk::UnsMesh::CoordMap m_coordmap;
    //! Coordinates of mesh nodes of our chunk of the mesh
    tk::UnsMesh::Coords m_coord;
    //! List of boundary faces associated to side-set IDs
    std::map< int, std::vector< std::size_t > > m_bface;
    //! List of boundary nodes associated to side-set IDs
    std::map< int, std::vector< std::size_t > > m_bnode;
    //! Boundary face-node connectivity
    std::vector< std::size_t > m_triinpoel;
    //! Total number of refiner chares
    int m_nchare;
    //! True if initial AMR, false if during time stepping
    RefMode m_mode;
    //! Initial mesh refinement type list (in reverse order)
    std::vector< ctr::AMRInitialType > m_initref;
    //! Number of initial mesh refinement/derefinement steps
    std::size_t m_ninitref;
    //! Mesh refiner (library) object
    AMR::mesh_adapter_t m_refiner;
    //! Counter during distribution of newly added nodes to chare-boundary edges
    std::size_t m_nref;
    //! Counter for number of chares contributing to chare boundary edges
    std::size_t m_nbnd;
    //! Number of chare-boundary newly added nodes that need correction
    std::size_t m_extra;
    //! Chares we share at least a single edge with
    std::unordered_set< int > m_ch;
    //! Edge->chare map used to build shared boundary edges
    std::unordered_map< Edge, std::vector< int >, Hash<2>, Eq<2> > m_edgech;
    //! Chare->edge map used to build shared boundary edges
    std::unordered_map< int, EdgeSet > m_chedge;
    //! Refinement data associated to edges
    AMR::EdgeData m_localEdgeData;
    //! Refinement data associated to edges shared with other chares
    std::unordered_map< int, std::vector< std::tuple<
      Edge, int, AMR::Edge_Lock_Case > > > m_remoteEdgeData;
    //! Edges received from other chares
    std::unordered_map< int, std::vector< Edge > > m_remoteEdges;
    //! Intermediate nodes
    std::unordered_set< size_t> m_intermediates;
    //! \brief Global mesh node IDs bordering the mesh chunk held by fellow
    //!    worker chares associated to their chare IDs for the coarse mesh
    tk::NodeCommMap m_nodeCommMap;
    //! Tetrahedra before refinement/derefinement step
    TetSet m_oldTets;
    //! Newly added mesh nodes (local id) and their parents (local ids)
    std::unordered_map< std::size_t, Edge > m_addedNodes;
    //! Newly added mesh cells (local id) and their parent (local id)
    std::unordered_map< std::size_t, std::size_t > m_addedTets;
    //! Number of tetrahedra in the mesh before refinement/derefinement step
    std::size_t m_oldntets;
    //! A unique set of faces associated to side sets of the coarsest mesh
    std::unordered_map< int, FaceSet > m_coarseBndFaces;
    //! A unique set of nodes associated to side sets of the coarsest mesh
    std::unordered_map< int, std::unordered_set<std::size_t> > m_coarseBndNodes;
    //! Local -> refiner lib node id map
    std::vector< std::size_t > m_rid;
    //! Local -> refiner lib node id map for previous mesh
    std::vector< std::size_t > m_oldrid;
    //! Refiner lib -> local node id map
    std::unordered_map< std::size_t, std::size_t > m_lref;
    //! Refiner lib -> local node id map for previous mesh
    //std::unordered_map< std::size_t, std::size_t > m_oldlref;
    //! Child -> parent tet map
    std::unordered_map< Tet, Tet, Hash<4>, Eq<4> > m_parent;
    //! Function to continue with after writing field output
    CkCallback m_writeCallback;
    //! \brief Tetrahedron element connectivity of our chunk of the mesh
    //!   (global ids) for the field-output-refined mesh
    std::vector< std::size_t > m_outref_ginpoel;
    //! \brief Elements of the mesh chunk we operate on for the
    //!   field-output-refined mesh
    //! \details The first vector is the element connectivity (local IDs), the
    //!   second vector is the global node IDs of owned elements, while the
    //!   third one is a map of global->local node IDs.
    tk::UnsMesh::Chunk m_outref_el;
    //! \brief Coordinates of mesh nodes of our chunk of the mesh for
    //!   field-output-refined mesh
    tk::UnsMesh::Coords m_outref_coord;
    //! \brief Newly added mesh nodes (local id) and their parents (local ids)
    //!   for the field-output-refined mesh
    std::unordered_map< std::size_t, Edge > m_outref_addedNodes;
    //! \brief Newly added mesh cells (local id) and their parent (local id)
    //!   for the field-output-refined mesh
    std::unordered_map< std::size_t, std::size_t > m_outref_addedTets;
    //! \brief Global mesh node IDs bordering the mesh chunk held by fellow
    //!    worker chares associated to their chare IDs for the coarse mesh for
    //!    the field-output-refined mesh
    tk::NodeCommMap m_outref_nodeCommMap;
    //! \brief List of boundary faces associated to side-set IDs for the
    //!   field-output-refined mesh
    std::map< int, std::vector< std::size_t > > m_outref_bface;
    //! \brief List of boundary nodes associated to side-set IDs for the
    //!   field-output-refined mesh
    std::map< int, std::vector< std::size_t > > m_outref_bnode;
    //! Boundary face-node connectivity for the field-output-refinedmesh
    std::vector< std::size_t > m_outref_triinpoel;

    //! (Re-)generate local -> refiner lib node id map and its inverse
    void libmap();

    //! (Re-)generate boundary data structures for coarse mesh
    void coarseBnd();

    //! Generate flat coordinate data from coordinate map
    tk::UnsMesh::Coords flatcoord( const tk::UnsMesh::CoordMap& coordmap );

    //! Output mesh to file before a new step of mesh refinement/derefinement
    void t0ref();

    //! Generate boundary edges and send them to all chares
    void bndEdges();

    //! Finish initiel mesh refinement
    void endt0ref();

    //! Do uniform mesh refinement
    void uniformRefine();

    //! Do uniform mesh derefinement
    void uniformDeRefine();

    //! Do error-based mesh refinement
    void errorRefine(RefMode mode = RefMode::DTREF);

    //! Compute errors in edges
    EdgeError
    errorsInEdges( std::size_t npoin,
                   const std::pair< std::vector< std::size_t >,
                                    std::vector< std::size_t > >& esup,
                   const tk::Fields& u ) const;

    //! Update (or evaluate) solution on current mesh
    tk::Fields
    solution( std::size_t npoin,
              const std::pair< std::vector< std::size_t >,
                               std::vector< std::size_t > >& esup ) const;

    //! Do mesh refinement based on user explicitly tagging edges
    void edgelistRefine();

    //! Do mesh refinement based on tagging edges based on end-point coordinates
    void coordRefine();

    //! Query AMR lib and update our local store of edge data
    void updateEdgeData();

    //! Aggregate number of extra edges across all chares
    void matched();

    //! Update old mesh after refinement
    void updateMesh();

    //! Update volume mesh after mesh refinement
    void newVolMesh( const std::unordered_set< std::size_t >& old,
                     const std::unordered_set< std::size_t >& ref );

    //! Update boundary data structures after mesh refinement
    void newBndMesh( const std::unordered_set< std::size_t >& ref );

    //! \brief Generate boundary data structures used to update
    //!   refined/derefined boundary faces and nodes of side sets
    BndFaceData boundary();

    //! Regenerate boundary faces after mesh refinement/derefinement step
    void updateBndFaces( const std::unordered_set< std::size_t >& ref,
                         const BndFaceData& bnd );

    //! Regenerate boundary nodes after mesh refinement/derefinement step
    void updateBndNodes( const std::unordered_set< std::size_t >& ref,
                         const BndFaceData& bnd );

    //! Evaluate initial conditions (IC) at mesh nodes
    tk::Fields
    nodeinit( std::size_t npoin,
              const std::pair< std::vector< std::size_t >,
                               std::vector< std::size_t > >& esup ) const;

    //! Output mesh to file(s)
    void writeMesh( const std::string& basefilename,
                    uint64_t it,
                    tk::real t,
                    CkCallback c ) const;

    //! Compute partial boundary surface integral and sum across all chares
    bool bndIntegral();

    //! Find the oldest parents of a mesh node in the AMR hierarchy
    std::unordered_set< std::size_t >
    ancestors( std::size_t n );

    //! Return a set of keys among whose values a primitive is found
    //! \tparam Sets Type of map of sets we search for the primitive
    //! \tparam Primitive The primitive we search for in the sets
    //! \note Sets::mapped_type == Primitive
    //! \param[in] sets Map of sets we search in
    //! \param[in] p Primitive we search for
    //! \return A unique set of set ids in which the primitive is found or
    //!   an empty set if the primitive was not found.
    //! \details This function searches a map of sets for an item (a primitive,
    //!   e.g., a single id or a face given by 3 node ids) and returns a
    //!   unique set of keys behind whose associated sets the item was found.
    template< class Sets, class Primitive >
    std::unordered_set< int >
    keys( const Sets& sets, const Primitive& p ) {
      static_assert( std::is_same< typename Sets::mapped_type::value_type,
        Primitive >::value, "Type of primitive (face/node) in map of sets must "
        "be the same as the type of primitive (face/node) that is searched" );
      std::unordered_set< int > ss;
      for (const auto& s : sets)
        if (s.second.find(p) != end(s.second))
          ss.insert( s.first );
      return ss;
    }

    //! Call a function on each item of an array
    //! \tparam N Number of nodes in array
    //! \tparam F Function to pass each item to
    //! \param[in] array Array whose items to pass to function
    //! \param[in] f Function to pass each item of array to
    template< std::size_t N, class F >
    void addBndNodes( const std::array< std::size_t, N >& array, F f ) {
      for (auto n : array) f( n );
    }
};

} // inciter::

#endif // Refiner_h

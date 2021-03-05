#include "mesh_adapter.hpp"

#include <assert.h>                        // for assert
#include <cstddef>                         // for size_t
#include <iostream>                        // for operator<<, endl, basic_os...
#include <set>                             // for set
#include <utility>                         // for pair
#include "AMR/AMR_types.hpp"                 // for Edge_Refinement, edge_list_t
#include "AMR/Loggers.hpp"                   // for trace_out
#include "AMR/Refinement_State.hpp"          // for Refinement_Case, Refinemen...
#include "AMR/edge.hpp"                      // for operator<<, edge_t
#include "AMR/edge_store.hpp"                // for edge_store_t
#include "AMR/marked_refinements_store.hpp"  // for marked_refinements_store_t
#include "AMR/node_connectivity.hpp"         // for node_connectivity_t
#include "AMR/refinement.hpp"                // for refinement_t
#include "AMR/tet_store.hpp"                 // for tet_store_t

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunreachable-code"
  #pragma clang diagnostic ignored "-Wdocumentation"
#endif

namespace AMR {


#ifdef ENABLE_NODE_STORE
    /**
     * @brief This accepts external coord arrays and allows the node_store to
     * track the new node positions as they are added
     *
     * @param m_x X coodinates
     * @param m_y Y coodinates
     * @param m_z Z coodinates
     * @param graph_size Total number of nodes
     */
    // TODO: remove graph size and use m.size()
    // TODO: remove these pointers
    //void mesh_adapter_t::init_node_store(coord_type* m_x, coord_type* m_y, coord_type* m_z)
    //{
    //    assert( m_x->size() == m_y->size() );
    //    assert( m_x->size() == m_z->size() );

    //    node_store.set_x(*m_x);
    //    node_store.set_y(*m_y);
    //    node_store.set_z(*m_z);
    //}
#endif

    std::pair< bool, std::size_t > mesh_adapter_t::check_same_face(
      std::size_t tet_id,
      const std::unordered_set<std::size_t>& inactive_nodes)
    {
       edge_list_t edge_list = tet_store.generate_edge_keys(tet_id);

       Assert(inactive_nodes.size()==3 || inactive_nodes.size()==2,
         "Incorrectly sized inactive nodes set");

       // for a tet ABCD, the keys (edges) are ordered
       // A-B, A-C, A-D, B-C, B-D, C-D
       // 0-1, 0-2, 0-3, 1-2, 1-3, 2-3

       std::array< std::array< std::size_t, 3 >, 4 >
         edges_on_face;

       // A-B-C
       edges_on_face[0][0] =
         tk::cref_find(node_connectivity.data(),edge_list[0].get_data());
       edges_on_face[0][1] =
         tk::cref_find(node_connectivity.data(),edge_list[1].get_data());
       edges_on_face[0][2] =
         tk::cref_find(node_connectivity.data(),edge_list[3].get_data());

       // A-B-D
       edges_on_face[1][0] =
         tk::cref_find(node_connectivity.data(),edge_list[0].get_data());
       edges_on_face[1][1] =
         tk::cref_find(node_connectivity.data(),edge_list[2].get_data());
       edges_on_face[1][2] =
         tk::cref_find(node_connectivity.data(),edge_list[4].get_data());

       // B-C-D
       edges_on_face[2][0] =
         tk::cref_find(node_connectivity.data(),edge_list[3].get_data());
       edges_on_face[2][1] =
         tk::cref_find(node_connectivity.data(),edge_list[4].get_data());
       edges_on_face[2][2] =
         tk::cref_find(node_connectivity.data(),edge_list[5].get_data());

       // A-C-D
       edges_on_face[3][0] =
         tk::cref_find(node_connectivity.data(),edge_list[1].get_data());
       edges_on_face[3][1] =
         tk::cref_find(node_connectivity.data(),edge_list[2].get_data());
       edges_on_face[3][2] =
         tk::cref_find(node_connectivity.data(),edge_list[5].get_data());

       //Iterate over edges to determine if inactive_nodes are all part of a face
       bool same_face(false);
       [[maybe_unused]] bool tnode_set(false);
       std::size_t third_node = 0;
       for(const auto& face : edges_on_face)
       {
         std::size_t icount = 0;
         for (const auto& np_node : face) {
           if (inactive_nodes.count(np_node)) ++icount;
         }
         if (inactive_nodes.size() == icount) {
           same_face = true;
           // if the two inactive_nodes being checked are on the same parent
           // face, determine the third node on that face
           if (inactive_nodes.size() == 2) {
             for (auto fn:face) {
               if (inactive_nodes.count(fn) == 0) {
                 third_node = fn;
                 tnode_set = true;
                 break;
               }
             }
           }
         }
       }

       if (same_face && inactive_nodes.size() == 2)
         Assert(tnode_set, "Third node on face not set in derefine");

       return {same_face, third_node};
    }

    /** @brief Consume an existing mesh, and turn it into the AMRs
     * representations of tets and nodes
     *
     * @param tetinpoel Vector of nodes grouped together in blocks of 4 to
     * represent tets
     */
    void mesh_adapter_t::consume_tets(const std::vector< std::size_t >& tetinpoel )
    {
        for (size_t i = 0; i < tetinpoel.size(); i+=4)
        {
            tet_t t = {
                {
                    tetinpoel[i],
                    tetinpoel[i+1],
                    tetinpoel[i+2],
                    tetinpoel[i+3]
                }
            };

            trace_out << "Consume tet " << i << std::endl;
            tet_store.add(t, AMR::Refinement_Case::initial_grid);
        }
    }

    /**
     * @brief Place holder function to evaluate error estimate at
     * nodes, and therefor mark things as needing to be refined
     */
    //void mesh_adapter_t::evaluate_error_estimate() {
    //    for (auto& kv : tet_store.edge_store.edges)
    //    {
    //        // Mark them as needing refinement
    //        if (kv.second.refinement_criteria > refinement_cut_off)
    //        {
    //            kv.second.needs_refining = 1;
    //        }
    //        else
    //        {
    //            // TODO: Check this won't be overwriting valuable
    //            // information from last iteration
    //            kv.second.needs_refining = 0;
    //        }
    //    }
    //}

    /**
     * @brief Helper function to apply uniform refinement to all tets
     */
    void mesh_adapter_t::mark_uniform_refinement()
    {
        for (auto& kv : tet_store.edge_store.edges) {
           auto& local = kv.second;
           if (local.lock_case == Edge_Lock_Case::unlocked)
             local.needs_refining = 1;
        }
        mark_refinement();
    }

    /**
     * @brief Helper function to apply uniform derefinement to all tets
     */
    void mesh_adapter_t::mark_uniform_derefinement()
    {
      const auto& inp = tet_store.get_active_inpoel();
      auto& edge_store = tet_store.edge_store;
      for (std::size_t t=0; t<inp.size()/4; ++t) {
        const auto edges =
          edge_store.generate_keys(
            {inp[t*4+0], inp[t*4+1], inp[t*4+2], inp[t*4+3]});
        for (const auto& tetedge : edges) {
          auto e = edge_store.edges.find(tetedge);
          if (e != end(edge_store.edges)) {
            auto& local = e->second;
            //if (local.lock_case == Edge_Lock_Case::unlocked) {
              local.needs_derefining = 1;
            //  trace_out << "edge marked for deref: " << local.A << " - "
            //    << local.B << std::endl;
            //}
          }
        }
      }
      mark_derefinement();
    }

    /**
     * @brief For a given set of edges, set their refinement criteria for
     * refinement
     *
     * @param remote Vector of edges and edge tags
     * @param is_deref True if error-based derefinement is required; default is
     *   is false, i.e. error-based refinement
     */
    void mesh_adapter_t::mark_error_refinement(
            const std::vector< std::pair< edge_t, edge_tag > >& remote,
            bool is_deref )
    {
       for (const auto& r : remote) {
         auto& local = tet_store.edge_store.get( r.first );
         if (r.second == edge_tag::REFINE) {
           if (local.lock_case > Edge_Lock_Case::unlocked) {
             local.needs_refining = 0;
           } else {
             local.needs_refining = 1;
           }
         } else if (r.second == edge_tag::DEREFINE) {
           if (local.lock_case > Edge_Lock_Case::unlocked) {
             local.needs_derefining = 0;
           } else {
             local.needs_derefining = 1;
           }
         }
       }

       if (is_deref) {
         mark_derefinement();
       }
       else {
         mark_refinement();
       }
    }

   void mesh_adapter_t::mark_error_refinement_corr( const EdgeData& edges,
     bool is_deref )
    {
       for (const auto& r : edges)
       {
           auto& edgeref = tet_store.edge_store.get( edge_t(r.first) );
           edgeref.needs_refining = r.second.first;
           assert(edgeref.lock_case <= r.second.second);
           edgeref.lock_case = r.second.second;
       }

       if (is_deref) {
         mark_derefinement();
       }
       else {
         mark_refinement();
       }
    }

    /**
     * @brief Function to detect the compatibility class (1,
     * 2, or 3) based on the number of locked edges and the existence
     * of intermediate edges
     *
     * @param num_locked_edges The number of locked edges
     * @param num_intermediate_edges The number of intermediate edges
     * @param refinement_case The refinement case of the tet
     * @param normal TODO: Document this!
     *
     * @return The compatibili4y class of the current scenario
     */
    int mesh_adapter_t::detect_compatibility(
            int num_locked_edges,
            int num_intermediate_edges,
            AMR::Refinement_Case refinement_case,
            int normal
    )
    {
        int compatibility = 0;
        num_locked_edges += num_intermediate_edges;

        /*
        // Split this into three categories
        // 1. Normal elements without locked edges. => 1
        //if (normal) {

            // 3. Intermediate elements with at least one edge marked for refinement => 3
            if (num_intermediate_edges > 0)
            {
                compatibility = 3;
            }
            else if (num_locked_edges == 0) {
                compatibility = 1;
            }
        // 2. Normal elements with locked edges. => 2
            else {
                compatibility = 2;
            }
        //}
        */

        //else {
            //if (num_intermediate_edges > 0) { compatibility = 3; }
        //}



        // Only 1:2 and 1:4 are intermediates and eligible for class3 // NOT TRUE!
        /*
        if (num_intermediate_edges > 0)
        {
            if (!normal) {
                trace_out << " not normal 3 " << std::endl;
                compatibility = 3;
            }
            else { // Attempt to allow for "normal" 1:4 and 1:8
                compatibility = 2;
                trace_out << " normal 3 " << std::endl;
            }

        }
        else {
            if (num_locked_edges == 0) {
                trace_out << " no lock 1 " << std::endl;
                compatibility = 1;
            }
            else {
                trace_out << " lock 2 " << std::endl;
                compatibility = 2;
            }
        }
        */


        // Old implementation
        // Only 1:2 and 1:4 are intermediates and eligible for class3 // NOT TRUE!
        if (
                (refinement_case == AMR::Refinement_Case::one_to_two) or
                (refinement_case == AMR::Refinement_Case::one_to_four)
           )
        {
            if (!normal) {
                trace_out << " not normal 3 " << std::endl;
                compatibility = 3;
            }
            else { // Attempt to allow for "normal" 1:4 and 1:8
                compatibility = 2;
                trace_out << " normal 3 " << std::endl;
            }

        }
        else {
            if (num_locked_edges == 0) {
                trace_out << " no lock 1 " << std::endl;
                compatibility = 1;
            }
            else {
                trace_out << " lock 2 " << std::endl;
                compatibility = 2;
            }
        }

        assert(compatibility > 0);
        assert(compatibility < 4);
        return compatibility;
    }

    /**
     * @brief Function which implements the main refinement algorithm from
     * the paper Iterating over the cells, deciding which refinement and
     * compatibility types are appropriate etc
     */
    void mesh_adapter_t::mark_refinement() {

#ifndef AMR_MAX_ROUNDS
        // Paper says the average actual num rounds will be 5-15
#define AMR_MAX_ROUNDS 30
#endif
        const size_t max_num_rounds = AMR_MAX_ROUNDS;

        // Mark refinements
        size_t iter;
        //Iterate until convergence
        for (iter = 0; iter < max_num_rounds; iter++)
        {

            tet_store.marked_refinements.get_state_changed() = false;

            // Loop over Tets.
            for (const auto& kv : tet_store.tets)
            {
                size_t tet_id = kv.first;

                trace_out << "Process tet " << tet_id << std::endl;

                // Only apply checks to tets on the active list
                if (tet_store.is_active(tet_id)) {
                    int num_locked_edges = 0;
                    int num_intermediate_edges = 0;

                    // Loop over nodes and count the number which need refining
                    int num_to_refine = 0;

                    // This is useful for later inspection
                    edge_list_t edge_list = tet_store.generate_edge_keys(tet_id);

                    //Iterate over edges
                    for(auto & key : edge_list)
                    {

                        trace_out << "Edge " << key << std::endl;

                        //Count locked edges and edges in need of
                        // refinement
                        // Count Locked Edges
                        if(tet_store.edge_store.get(key).lock_case == AMR::Edge_Lock_Case::locked)
                        {
                            trace_out << "Found locked edge " << key << std::endl;
                            trace_out << "Locked :" << tet_store.edge_store.get(key).lock_case << std::endl;
                            num_locked_edges++;
                        }
                        else if(tet_store.edge_store.get(key).lock_case == AMR::Edge_Lock_Case::intermediate)
                        {
                            trace_out << "Found intermediate edge " << key << std::endl;
                            num_intermediate_edges++;
                        }
                        else
                        {
                            // Count edges which need refining
                            //  We check in here as we won't refine a
                            //  locked edge and will thus ignore it
                            if (tet_store.edge_store.get(key).needs_refining)
                            {
                                num_to_refine++;
                                trace_out << "key needs ref " << key << std::endl;
                            }
                        }
                    }

                    // TODO: Should this be a reference?
                    AMR::Refinement_Case refinement_case = tet_store.get_refinement_case(tet_id);
                    int normal = tet_store.is_normal(tet_id);

                    trace_out << "Checking " << tet_id <<
                        " ref case " << refinement_case <<
                        " num ref " << num_to_refine <<
                        " normal " << normal <<
                        std::endl;



                    //If we have any tets to refine
                    if (num_to_refine > 0)
                    {
                        //Determine compatibility case

                        int compatibility = detect_compatibility(num_locked_edges,
                                num_intermediate_edges, refinement_case, normal);

                        trace_out << "Compat " << compatibility << std::endl;

                        // Now check num_to_refine against situations
                        if (compatibility == 1)
                        {
                            refinement_class_one(num_to_refine, tet_id);
                        }
                        else if (compatibility == 2)
                        {
                            refinement_class_two(edge_list, tet_id);
                        }
                        else if (compatibility == 3)
                        {
                            refinement_class_three(tet_id);
                        }

                        /*
                        // Write temp mesh out
                        std::string temp_file =  "temp." +
                        std::to_string(iter) + "." +
                        std::to_string(tet_id) + ".exo";

                        std::cout << "Writing " << temp_file << std::endl;
                        Adaptive_UnsMesh outmesh(
                        get_active_inpoel(), x(), y(), z()
                        );
                        tk::ExodusIIMeshWriter( temp_file, tk::ExoWriter::CREATE ).
                        writeMesh(outmesh);
                        */

                    } // if num_to_refine
                    else {
                            // If we got here, we don't want to refine this guy
                            //tet_store.marked_refinements.add(tet_id, AMR::Refinement_Case::none);
                    }
                } // if active
                else {
                    trace_out << "Inactive" << std::endl;
                }
            } // For

            // If nothing changed during that round, break
            if (!tet_store.marked_refinements.get_state_changed())
            {
                trace_out << "Terminating loop at iter " << iter << std::endl;
                break;
            }
            trace_out << "End iter " << iter << std::endl;
        }
        trace_out << "Loop took " << iter << " rounds." << std::endl;

        //std::cout << "Print Tets" << std::endl;
        //print_tets();
    }

    /**
     * @brief Helper function to print tet information when needed
     */
    void mesh_adapter_t::print_tets() {
        tet_store.print_tets();
    }

    /**
     * @brief Function to call refinement after each tet has had it's
     * refinement case marked and calculated
     */
    void mesh_adapter_t::perform_refinement()
    {
        // Track tets which need to be deleted this iteration
        std::set<size_t> round_two;

        trace_out << "Perform ref" << std::endl;

        // Do refinements
        for (const auto& kv : tet_store.tets)
        {
            size_t tet_id = kv.first;

            trace_out << "Do refine of " << tet_id << std::endl;
            if (tet_store.has_refinement_decision(tet_id))
            {
                switch(tet_store.marked_refinements.get(tet_id))
                {
                    case AMR::Refinement_Case::one_to_two:
                        refiner.refine_one_to_two(tet_store,node_connectivity,tet_id);
                        break;
                    case AMR::Refinement_Case::one_to_four:
                        refiner.refine_one_to_four(tet_store,node_connectivity,tet_id);
                        break;
                    case AMR::Refinement_Case::one_to_eight:
                        refiner.refine_one_to_eight(tet_store,node_connectivity,tet_id);
                        break;
                    case AMR::Refinement_Case::two_to_eight:
                        round_two.insert( tet_store.get_parent_id(tet_id) );
                        //std::cout << "2->8\n";
                        break;
                    case AMR::Refinement_Case::four_to_eight:
                        round_two.insert( tet_store.get_parent_id(tet_id));
                        //std::cout << "4->8\n";
                        break;
                    case AMR::Refinement_Case::initial_grid:
                        // Do nothing
                    case AMR::Refinement_Case::none:
                        // Do nothing
                        break;
                        // No need for default as enum is explicitly covered
                }
                // Mark tet as not needing refinement
                tet_store.marked_refinements.erase(tet_id);
            }
        }

        trace_out << "round_two size " << round_two.size() << std::endl;
        for (const auto i : round_two)
        {
            trace_out << "round two i " << i << std::endl;

            // Cache children as we're about to change this data
            auto former_children = tet_store.data(i).children;

            AMR::Refinement_State& element = tet_store.data(i);

            if (element.children.size() == 2)
            {
                trace_out << "perform 2:8" << std::endl;
                refiner.derefine_two_to_one(tet_store,node_connectivity,i);
            }
            else if (element.children.size() == 4)
            {
                trace_out << "perform 4:8" << std::endl;
                refiner.derefine_four_to_one(tet_store,node_connectivity,i);
            }
            else {
                std::cout << "num children " << element.children.size() << std::endl;
                assert(0);
            }

            refiner.refine_one_to_eight(tet_store,node_connectivity,i);

            // Grab children after it has been updated
            auto current_children = tet_store.data(i).children;

            // I want to set the children stored in *my* own children, to be
            // the value of my new children....
            //refiner.overwrite_children(tet_store, former_children, current_children);

            tet_store.unset_marked_children(i); // FIXME: This will not work well in parallel
            element.refinement_case = AMR::Refinement_Case::one_to_eight;
        }

        // Clean up dead edges
        // clean_up_dead_edges(); // Nothing get's marked as "dead" atm?

        //std::cout << "Total Edges : " << tet_store.edge_store.size() << std::endl;
        //std::cout << "Total Tets : " << tet_store.size() << std::endl;
        //std::cout << "Total Nodes : " << m_x.size() << std::endl;

        trace_out << "Done ref" << std::endl;
        node_connectivity.print();
        node_connectivity.print();
        tet_store.print_node_types();
        tet_store.print_tets();
        //node_connectivity.print();

        //reset_intermediate_edges();
        remove_edge_locks(1);
        remove_normals();

        lock_intermediates();

        for (auto& kv : tet_store.edge_store.edges) {
           auto& local = kv.second;
           local.needs_refining = 0;
        }
    }

    void mesh_adapter_t::lock_intermediates()
    {
        /*
        for (auto k : tet_store.intermediate_list)
        {
            refiner.lock_edges_from_node(tet_store,k, Edge_Lock_Case::intermediate);
        }
        */
        // TODO: Passing tet_store twice probably isn't the best
        refiner.lock_intermediates(tet_store, tet_store.intermediate_list, Edge_Lock_Case::intermediate);
    }

    /**
     * @brief A method implementing "Algorithm 1" from the paper
     *
     * @param num_to_refine Number of edges to refine
     * @param tet_id The id of the given tet
     */
    void mesh_adapter_t::refinement_class_one(int num_to_refine, size_t tet_id)
    {
        trace_out << "Refinement Class One" << std::endl;

        // "If nrefine = 1
        // Accept as a 1:2 refinement"
        if (num_to_refine == 1)
        {
            tet_store.mark_one_to_two(tet_id);
        }

        // "Else if nrefine = 2 OR nrefine = 3"
        else if (num_to_refine > 1 && num_to_refine < 4)
        {

            // We need to detect if the edges which need to refine are
            // on the same face
            // and if so which face so we know how to 1:4

            face_list_t face_list = tet_store.generate_face_lists(tet_id);
            bool edges_on_same_face = false;
            size_t face_refine_id = 0;

            // Iterate over each face
            for (size_t face = 0; face < NUM_TET_FACES; face++)
            {
                int num_face_refine_edges = 0;
                face_ids_t face_ids = face_list[face];

                trace_out << "Face is " <<
                    face_ids[0] << ", " <<
                    face_ids[1] << ", " <<
                    face_ids[2] << ", " <<
                    std::endl;

                edge_list_t face_edge_list = AMR::edge_store_t::generate_keys_from_face_ids(face_ids);
                // For this face list, see which ones need refining
                for (size_t k = 0; k < NUM_FACE_NODES; k++)
                {
                    edge_t key = face_edge_list[k];
                    if (tet_store.edge_store.get(key).needs_refining == 1)
                    {
                        num_face_refine_edges++;
                    }
                }
                if (num_face_refine_edges == num_to_refine)
                {
                    edges_on_same_face = true;
                    face_refine_id = face;
                    trace_out << "Breaking with face value " << face << std::endl;
                    break;
                }
            }

            // "If active edges are on the same face
            // Activate any inactive edges of the face
            // Accept as a 1:4 // refinement"
            if (edges_on_same_face)
            {
                size_t opposite_offset = AMR::node_connectivity_t::face_list_opposite(face_list,
                        face_refine_id);

                tet_t tet = tet_store.get(tet_id);
                size_t opposite_id = tet[opposite_offset];

                trace_out << "face_refine_id " << face_refine_id << std::endl;
                trace_out << "opposite_offset " << opposite_offset << std::endl;
                trace_out << "opposite_id " << opposite_id << std::endl;

                // Activate edges on this face
                edge_list_t face_edge_list = AMR::edge_store_t::generate_keys_from_face_ids(face_list[face_refine_id]);

                for (size_t k = 0; k < NUM_FACE_NODES; k++)
                {
                    edge_t key = face_edge_list[k];
                    tet_store.edge_store.mark_for_refinement(key);
                }

                //refiner.refine_one_to_four(tet_id, face_list[face_refine_id],
                //opposite_id);
                tet_store.mark_one_to_four(tet_id);
            }
            // "Else if active edges are not on the same face
            // Activate all edges
            // Accept as a 1:8 refinement"
            else {
                //refiner.refine_one_to_eight(tet_id);
                tet_store.mark_edges_for_refinement(tet_id);
                tet_store.mark_one_to_eight(tet_id);
            }

        }

        // "Else if nrefine > 3
        // Activate any inactive edges
        // Accept as a 1:8 refinement"
        else if (num_to_refine > 3)
        {
            //refiner.refine_one_to_eight(tet_id);
            tet_store.mark_edges_for_refinement(tet_id);
            tet_store.mark_one_to_eight(tet_id);
        }
    }

    // TODO: Document this
    void mesh_adapter_t::lock_tet_edges(size_t tet_id) {
        edge_list_t edge_list = tet_store.generate_edge_keys(tet_id);
        for (size_t k = 0; k < NUM_TET_EDGES; k++)
        {
            edge_t key = edge_list[k];
            if (tet_store.edge_store.get(key).lock_case == AMR::Edge_Lock_Case::unlocked)
            {
                trace_out << "LOCKING! " << key << std::endl;
                tet_store.edge_store.get(key).lock_case = AMR::Edge_Lock_Case::locked;
            }
        }
    }

    // TODO: Document this
    // TODO: This has too similar a name to deactivate_tet
    void mesh_adapter_t::deactivate_tet_edges(size_t tet_id) {
        edge_list_t edge_list = tet_store.generate_edge_keys(tet_id);
        for (size_t k = 0; k < NUM_TET_EDGES; k++)
        {
            edge_t key = edge_list[k];

            tet_store.edge_store.unmark_for_refinement(key);
            trace_out << "Deactivating " << key << std::endl;
            tet_store.edge_store.get(key).needs_derefining = false;
        }
    }

    /**
     * @brief Unmarks edges of given tet for derefinement only
     *
     * @param tet_id The id of the given tet
     */
    void mesh_adapter_t::deactivate_deref_tet_edges(size_t tet_id) {
        edge_list_t edge_list = tet_store.generate_edge_keys(tet_id);
        for (size_t k = 0; k < NUM_TET_EDGES; k++)
        {
            edge_t key = edge_list[k];

            trace_out << "Deactivating " << key << std::endl;
            tet_store.edge_store.get(key).needs_derefining = false;
        }
    }

    /**
     * @brief An implementation of "Algorithm 2" from the paper
     *
     * @param edge_list The list of edges for the given tet
     * @param tet_id The id of the given tet
     */
    void mesh_adapter_t::refinement_class_two(edge_list_t edge_list, size_t tet_id)
    {
        trace_out << "Refinement Class Two" << std::endl;


        // "Deactivate all locked edges"

        // count number of active edges
        int num_active_edges = 0;
        for (size_t k = 0; k < NUM_TET_EDGES; k++)
        {
            edge_t key = edge_list[k];
            if (tet_store.edge_store.get(key).lock_case != AMR::Edge_Lock_Case::unlocked)
            {
                tet_store.edge_store.unmark_for_refinement(key);
            }
            // "Count number of active edges"
            if (tet_store.edge_store.get(key).needs_refining == 1) {
                num_active_edges++;
            }
        }

        // Find out of two active edges live on the same face
        bool face_refine = false;
        size_t face_refine_id = 0; // FIXME: Does this need a better default
        face_list_t face_list = tet_store.generate_face_lists(tet_id);

        // Iterate over each face
        for (size_t face = 0; face < NUM_TET_FACES; face++)
        {
            trace_out << "face " << face << std::endl;
            int num_face_refine_edges = 0;
            int num_face_locked_edges = 0;

            face_ids_t face_ids = face_list[face];
            edge_list_t face_edge_list = AMR::edge_store_t::generate_keys_from_face_ids(face_ids);
            // For this face list, see which ones need refining
            for (size_t k = 0; k < NUM_FACE_NODES; k++)
            {
                edge_t key = face_edge_list[k];
                trace_out << "Checking " << key << std::endl;
                if (tet_store.edge_store.get(key).needs_refining == 1)
                {
                    num_face_refine_edges++;
                    trace_out << "ref! " << key << std::endl;
                }

                // Check for locked edges
                // This case only cares about faces with no locks
                if (tet_store.edge_store.get(key).lock_case != AMR::Edge_Lock_Case::unlocked)
                {
                    num_face_locked_edges++;
                    trace_out << "locked! " << key << std::endl;
                }
            }


            // Decide if we want to process this face
            if (num_face_refine_edges >= 2 && num_face_locked_edges == 0)
            {
                // We can refine this face
                face_refine = true;
                face_refine_id = face;
                break;
            }
        }

        // "If nrefine = 1
        // Accept as 1:2 refinement"
        // TODO: can we hoist this higher
        if (num_active_edges == 1)
        {
            //node_pair_t nodes = find_single_refinement_nodes(edge_list);
            //refine_one_to_two( tet_id, nodes[0], nodes[1]);
            tet_store.mark_one_to_two(tet_id);
        }
        // "Else if any face has nrefine >= 2 AND no locked edges
        // Active any inactive edges of the face
        // Accept as a 1:4 refinement"
        else if (face_refine)
        {
            size_t opposite_offset = AMR::node_connectivity_t::face_list_opposite(face_list, face_refine_id);

            tet_t tet = tet_store.get(tet_id);
            size_t opposite_id = tet[opposite_offset];

            trace_out << "Tet ID " << tet_id << std::endl;
            trace_out << "Opposite offset " << opposite_offset << std::endl;
            trace_out << "Opposite id " << opposite_id << std::endl;
            trace_out << "Face refine id " << face_refine_id << std::endl;

            edge_list_t face_edge_list =
                AMR::edge_store_t::generate_keys_from_face_ids(face_list[face_refine_id]);

            for (size_t k = 0; k < NUM_FACE_NODES; k++)
            {
                edge_t key = face_edge_list[k];
                tet_store.edge_store.mark_for_refinement(key);
            }

            //refiner.refine_one_to_four(tet_id, face_list[face_refine_id], opposite_id);
            tet_store.mark_one_to_four(tet_id);
        }

        // "Else
        // Deactivate all edges
        // Mark all edges as locked"
        else {
            trace_out << "Class 2 causes some locking.." << std::endl;
            deactivate_tet_edges(tet_id);
            lock_tet_edges(tet_id);
        }

    }

    /**
     * @brief Based on a tet_id, decide if it's current state of locked
     * and marked edges maps to a valid refinement case. The logic for
     * this was derived from talking to JW and reading Chicoma.
     *
     * It basically just checks if something a 1:2 and has 3
     * intermediates and 3 makred edges, or is a 1:4 and has 5/6
     * intermediates
     *
     * @param child_id the id of the tet to check
     *
     * @return A bool saying if the tet is in a valid state to be refined
     */
    bool mesh_adapter_t::check_valid_refinement_case(size_t child_id) {

        trace_out << "check valid ref " << child_id << std::endl;
        edge_list_t edge_list = tet_store.generate_edge_keys(child_id);

        size_t num_to_refine = 0;
        size_t num_intermediate = 0;
        size_t unlocked = 0;
        size_t locked = 0;

        for (size_t k = 0; k < NUM_TET_EDGES; k++)
        {
            edge_t key = edge_list[k];
            trace_out << "Key " << key << std::endl;

            // Count intermediate edges
            if (tet_store.edge_store.get(key).lock_case == AMR::Edge_Lock_Case::intermediate)
            {
                trace_out << "found intermediate" << std::endl;
                num_intermediate++;
            }

            // Count number of marked for refinement
            if (tet_store.edge_store.get(key).needs_refining)
            {
                trace_out << "found refine" << std::endl;
                num_to_refine++;
            }

            if (tet_store.edge_store.get(key).lock_case == AMR::Edge_Lock_Case::unlocked)
            {
                trace_out << "found unlocked" << std::endl;
                unlocked++;
            }

            if (tet_store.edge_store.get(key).lock_case == AMR::Edge_Lock_Case::locked)
            {
                trace_out << "found locked" << std::endl;
                locked++;
            }

        }

        AMR::Refinement_State& element = tet_store.data(child_id);

        trace_out <<
            "Intermediates " << num_intermediate <<
            " num to refine " << num_to_refine <<
            " unlocked " << unlocked <<
            " locked " << locked <<
            " Case " << element.refinement_case <<
            std::endl;

        // check if element is 1:2
        if (element.refinement_case == AMR::Refinement_Case::one_to_two)
        {
            // If so check it has 3 intermediates and 3 which need refining
            if (num_intermediate != 3 || num_to_refine != 3) {
                return false;
            }
            else {
                trace_out << "True " <<
                    "Intermediates " << num_intermediate <<
                    " num to refine " << num_to_refine <<
                    " Case " << element.refinement_case <<
                    " 2:1 " << AMR::Refinement_Case::one_to_two <<
                    std::endl;
            }
        }

        // check if element is 1:4
        else if (element.refinement_case == AMR::Refinement_Case::one_to_four)
        {
            // TODO: Check if it's a center tet for a 1:4
            // FIXME: Is this even needed? How else would you get these
            // combinations? Can't we just combine these two checks?

            bool is_center_tet = tet_store.is_center(child_id);

            if (is_center_tet)
            {
                if (num_to_refine != 0 || num_intermediate != 6)
                {
                    trace_out << "Fail compat 1:4 center" << std::endl;
                    return false;
                }
            }
            else { // Is one of the outsides (not center)
                if (num_to_refine != 1 || num_intermediate != 5)
                {
                    trace_out << "Fail compat 1:4 non center" << std::endl;
                    return false;
                }
            }
        }

        // If it makes it here, it's compatible
        return true;

    }

    /**
     * @brief Place holder method for the implementation of "Algorithm
     * 3" from the paper
     */
    // TODO: Does this parse a childs siblings multiple times?
    void mesh_adapter_t::refinement_class_three(size_t tet_id) {

        trace_out << "Refinement Class Three" << std::endl;

        // "Identify parent element iparent"
        // TODO: WE should either always use the id to fetch, or always do the data lookup
        //size_t parent_id = master_elements.get_parent(tet_id);
        size_t parent_id = tet_store.get_parent_id(tet_id);

        trace_out << "Parent id = " << parent_id << std::endl;

        // NOTE: This implies comms when we use these ids?
        child_id_list_t children = tet_store.data(parent_id).children;

        // "Do for each child element ielement
        // Activate all non-locked edges
        // Deactivate all locked edges"
        for (size_t i = 0; i < children.size(); i++)
        {
            // TODO: Is this in element or tet ids?
            trace_out << "Checking child " << children[i] << std::endl;
            edge_list_t edge_list = tet_store.generate_edge_keys(children[i]);
            for (size_t k = 0; k < NUM_TET_EDGES; k++)
            {
                edge_t key = edge_list[k];
                trace_out << "Compat 3 " << key << std::endl;
                if (tet_store.edge_store.get(key).lock_case == AMR::Edge_Lock_Case::unlocked)
		{
                    trace_out << "Compat 3 marking edge " << key << std::endl;
                    tet_store.edge_store.mark_for_refinement(key);
                }
                else {
                    tet_store.edge_store.unmark_for_refinement(key);
                }
            }
        }

        // "Set compatible = TRUE
        bool compatible = true;

        // Do for each child element ielement
        // If ielement is not a valid refinement case
        // compatible = FALSE"
        for (size_t i = 0; i < children.size(); i++)
        {
            size_t child = children[i];
            if ( !check_valid_refinement_case(child) )
            {
                trace_out << "Compat 3 Marking compatible false because of invalid refinement case" << std::endl;

                compatible = false;
            }
            else {
                trace_out << "Is compatible" << std::endl;
            }
        }

        // "If compatible = FALSE
        // Do for each child element ielement
        // Deactive all edges of ielement
        // Mark all edges of ielement as locked
        // Mark ielement as normal"
        if (compatible == false)
        {
            for (size_t i = 0; i < children.size(); i++)
            {
                size_t child = children[i];
                deactivate_tet_edges(child);
                lock_tet_edges(child);
                trace_out << "Compat 3 locking edges of " << child << std::endl;
                // Here we interpret normal to mean "don't treat it like it has intermediates"
                tet_store.mark_normal(child);
                trace_out << "Compat 3 " << child << std::endl;
            }
        }
        else {
            trace_out << "TIME TO 2:8 " << tet_id << std::endl;
            // Accept as 2:8 or 4:8
            AMR::Refinement_State& element = tet_store.data(tet_id);
            if (element.refinement_case == AMR::Refinement_Case::one_to_two)
            {
                tet_store.mark_two_to_eight(tet_id);
            }
            else if (element.refinement_case == AMR::Refinement_Case::one_to_four)
            {
                tet_store.mark_four_to_eight(tet_id);
            }
            else {
                trace_out << " I don't know what to do with this..it looks like you're trying to 2/4:8 an 8... " << std::endl;
            }

        }
    }

    void mesh_adapter_t::remove_normals()
    {
        for (const auto& kv : tet_store.tets)
        {
            size_t tet_id = kv.first;
            tet_store.set_normal(tet_id, 0);
        }
    }

    void mesh_adapter_t::remove_edge_locks(int intermediate)
    {
        for (const auto& kv : tet_store.tets)
        {
            size_t tet_id = kv.first;

            trace_out << "Process tet removelock " << tet_id << std::endl;

            // Only apply checks to tets on the active list
            if (tet_store.is_active(tet_id)) {
                // change it from intermediate to locked
                update_tet_edges_lock_type(tet_id, AMR::Edge_Lock_Case::locked, AMR::Edge_Lock_Case::unlocked);
                if (intermediate) {
                    update_tet_edges_lock_type(tet_id, AMR::Edge_Lock_Case::intermediate, AMR::Edge_Lock_Case::unlocked);
                }
            }
        }

    }
    //void mesh_adapter_t::reset_intermediate_edges()
    //{
    //    for (const auto& kv : tet_store.tets)
    //    {
    //        size_t tet_id = kv.first;

    //        trace_out << "Process tet reset " << tet_id << std::endl;

    //        // Only apply checks to tets on the active list
    //        if (tet_store.is_active(tet_id)) {
    //            // change it from intermediate to locked
    //            update_tet_edges_lock_type(tet_id, AMR::Edge_Lock_Case::intermediate, AMR::Edge_Lock_Case::locked);
    //        }
    //    }
    //}

    void mesh_adapter_t::update_tet_edges_lock_type(size_t tet_id, AMR::Edge_Lock_Case check, AMR::Edge_Lock_Case new_case) {
        edge_list_t edge_list = tet_store.generate_edge_keys(tet_id);
        for (size_t k = 0; k < NUM_TET_EDGES; k++)
        {
            edge_t key = edge_list[k];
            if (tet_store.edge_store.get(key).lock_case == check)
            {
                tet_store.edge_store.get(key).lock_case = new_case;
            }
        }
    }

    void mesh_adapter_t::mark_derefinement()
    {
        const size_t max_num_rounds = AMR_MAX_ROUNDS;

        // Mark refinements
        size_t iter;
        //Iterate until convergence
        for (iter = 0; iter < max_num_rounds; iter++)
        {
            tet_store.marked_derefinements.get_state_changed() = false;

            // Loop over tets
            for (const auto& kv : tet_store.tets)
            {
                size_t tet_id = kv.first;

                // Skip tets which have no children or which are grand-parents
                child_id_list_t children = tet_store.data(tet_id).children;
                bool grandparent = false;
                for (const auto& ch : children) {
                  if (tet_store.data(ch).children.size()) grandparent = true;
                }
                if (children.size() <= 0) { continue; }
                else if (grandparent) {
                  //TODO: this needs to be done only at the first iteration
                  trace_out << "grandparent: " << tet_id
                    << " unmarking children from deref" << std::endl;
                  // go over grandparent's children and lock its edges, only if
                  // that edge does not belong to a grandchild. The following
                  // does this in a simple but 'hacky' way
                  std::map< edge_t, std::size_t > edge_derefmark;

                  for (auto child_id : children) {
                    auto grandchildren = tet_store.data(child_id).children;
                    // get child_id's nodes
                    auto child_nodes = tet_store.get(child_id);
                    std::unordered_set<size_t> child_node_set{
                      std::begin(child_nodes), std::end(child_nodes)};
                    trace_out << "looking at child: " << child_id <<
                      " which has " << grandchildren.size() << " children" << std::endl;
                  // 1. store the grandchildren's edge-markings
                    for (auto gchild_id : grandchildren) {
                      auto edges = tet_store.generate_edge_keys(gchild_id);
                      trace_out << "looking at grandchild: " << gchild_id << std::endl;
                      for (const auto& edge_key : edges) {
                        // retaining this edge's deref marking only if one of
                        // the nodes of this edge does not belong to the parent
                        // of gchild_id (which is child_id), i.e. nonparent node.
                        // basically, keep deref-decisions only for the edges
                        // that have been split
                        if (child_node_set.count(edge_key.first())==0 ||
                          child_node_set.count(edge_key.second())==0) {
                          edge_derefmark[edge_key] =
                            tet_store.edge_store.get(edge_key).needs_derefining;
                        }
                      }
                    }

                  // 2. go thru the children, and unmark their edges for deref
                    deactivate_deref_tet_edges(child_id);
                  }

                  // 3. go thru the grandparents again and reinstate their edge
                  // derefinement markings
                  for (const auto& e_key : edge_derefmark) {
                    tet_store.edge_store.get(e_key.first).needs_derefining =
                      e_key.second;
                  }

                  continue;
                }


                // This is useful for later inspection
                //edge_list_t edge_list = tet_store.generate_edge_keys(tet_id);
                std::size_t num_to_derefine = 0; // Nodes

                AMR::Refinement_Case refinement_case = tet_store.get_refinement_case(tet_id);

                auto derefine_node_set = refiner.find_derefine_node_set(tet_store, tet_id);
                // Find the set of nodes which are not in the parent
                std::unordered_set<size_t> non_parent_nodes =
                  refiner.child_exclusive_nodes(tet_store, tet_id);

                //for (auto drnode: derefine_node_set)
                //  trace_out << "derefine node: " << drnode << std::endl;

                num_to_derefine = derefine_node_set.size();

                if (num_to_derefine > 0) {
                    trace_out << "num_to_derefine " << num_to_derefine << std::endl;
                    trace_out << "ref_case " << refinement_case << std::endl;
                    trace_out << "num children " << children.size() << std::endl;
                }

                //num_to_derefine = convert_derefine_edges_to_points(tet_store, tet_id, num_edges_to_derefine, refinement_case);

                // "If nderefine = 1
                if (num_to_derefine == 1)
                {
                    // If icase = 1:2

                    //if (refinement_case == AMR::Refinement_Case::one_to_two)
                    if (children.size() == 2)
                    {
                        // Accept as 2:1 derefine"
                        trace_out << "Accept as 2:1" << std::endl;
                        //refiner.derefine_two_to_one(tet_store, node_connectivity, tet_id);
                        tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::two_to_one);
                    }
                    // "Else
                    else {
                        // Deactivate all points"
                        for (auto child_id : children) {
                          deactivate_deref_tet_edges(child_id);
                        }
                        tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::skip);
                        trace_out << "giving up on deref decision. deactivate near 2:1 ntd = 1" << std::endl;
                    }
                }


                // "If nderefine = 2
                else if (num_to_derefine == 2)
                {
                    // If icase = 1:4
                    //if (refinement_case == AMR::Refinement_Case::one_to_four)
                    if (children.size() == 4)
                    {
                        // Accept as 4:2 derefine"
                        trace_out << "Accept as 4:2" << std::endl;
                        //refiner.derefine_four_to_two(tet_store,  node_connectivity, tet_id);
                        tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::four_to_two);
                    }
                    // "Else
                    else {
                        // Deactivate all points"
                        for (auto child_id : children) {
                          deactivate_deref_tet_edges(child_id);
                        }
                        tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::skip);
                        trace_out << "giving up on deref decision. deactivate near 4:2 ntd = 2" << std::endl;
                    }
                }

                // "If nderefine = 3
                else if (num_to_derefine == 3)
                {
                    // If icase = 1:4
                    //if (refinement_case == AMR::Refinement_Case::one_to_four)
                    if (children.size() == 4)
                    {
                        // Accept as 4:2 derefine"
                        trace_out << "Accept as 4:1" << std::endl;
                        //refiner.derefine_four_to_two(tet_store,  node_connectivity, tet_id);
                        tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::four_to_one);
                    }
                    // "Else if icase = 1:8
                    //else if (refinement_case == AMR::Refinement_Case::one_to_eight)
                    else if (children.size() == 8)
                    {
                        // we have a list of (non-parent) nodes that is marked
                        // for derefinement. First, determine the nodes that are
                        // unmarked for derefinement (or inactive_nodes). Then,
                        // determine if these are on a single face.
                        std::unordered_set<size_t> inactive_node_set;
                        for (auto npn : non_parent_nodes) {
                          if (derefine_node_set.count(npn) == 0)
                            inactive_node_set.insert(npn);
                        }
                        Assert(inactive_node_set.size() == 3, "Incorrectly "
                          "sized inactive-node set");
                        auto same_face = check_same_face(tet_id, inactive_node_set);
                        // If inactive points lie on same face
                        if (same_face.first == true)
                        {
                            // Accept as 8:4 derefinement
                            trace_out << "Accept as 8:4" << std::endl;

                            // create a vector of node-array-pairs to mark edges
                            // for refinement 1:4
                            std::vector< std::array< std::size_t, 2 > > ref_edges;
                            for (auto n:inactive_node_set) {
                              ref_edges.push_back(node_connectivity.get(n));
                            }

                            tet_store.edge_store.mark_edges_for_refinement(ref_edges);
                            //refiner.derefine_eight_to_four(tet_store,  node_connectivity, tet_id);
                            tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::eight_to_four);
                        }
                        // "Else
                        else {
                            // Deactivate all points"
                            for (auto child_id : children) {
                              deactivate_deref_tet_edges(child_id);
                            }
                            tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::skip);
                            trace_out << "giving up on deref decision. deactivate near 8:4 ntd = 3" << std::endl;
                        }

                    }
                }

                // "If nderefine = 4
                else if (num_to_derefine == 4)
                //else if (children.size() == 4)
                {
                    // we have a list of (non-parent) nodes that is marked
                    // for derefinement. First, determine the nodes that are
                    // unmarked for derefinement (or inactive_nodes). Then,
                    // determine if these are on a single face.
                    std::unordered_set<size_t> inactive_node_set;
                    for (auto npn : non_parent_nodes) {
                      if (derefine_node_set.count(npn) == 0)
                        inactive_node_set.insert(npn);
                    }
                    Assert(inactive_node_set.size() == 2, "Incorrectly "
                      "sized inactive-node set");
                    // Check if the inactive point belong to the same parent
                    // face and deactivate the third point on that face
                    auto same_face = check_same_face(tet_id, inactive_node_set);

                    if (same_face.first == true)
                    {
                        // deactivate the edges associated with same_face.second
                        for (size_t i = 0; i < children.size(); i++)
                        {
                          edge_list_t edge_list = tet_store.generate_edge_keys(children[i]);
                          for (size_t k = 0; k < NUM_TET_EDGES; k++)
                          {
                            edge_t edge = edge_list[k];
                            size_t A = edge.first();
                            size_t B = edge.second();
                            if (A == same_face.second || B == same_face.second)
                              tet_store.edge_store.get(edge).needs_derefining = false;
                          }
                        }

                        // create a vector of node-array-pairs to mark edges
                        // for refinement 1:4
                        inactive_node_set.insert(same_face.second);
                        std::vector< std::array< std::size_t, 2 > > ref_edges;
                        for (auto n:inactive_node_set) {
                          ref_edges.push_back(node_connectivity.get(n));
                        }

                        tet_store.edge_store.mark_edges_for_refinement(ref_edges);

                        // Accept as 8:4 derefinement
                        trace_out << "Accept as 8:4" << std::endl;
                        //refiner.derefine_eight_to_four(tet_store,  node_connectivity, tet_id);
                        tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::eight_to_four);
                    }
                    // "Else
                    else {
                        // Deactivate all points"
                        for (auto child_id : children) {
                          deactivate_deref_tet_edges(child_id);
                        }
                        tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::skip);
                        trace_out << "giving up on deref decision. deactivate near 8:4 ntd = 4" << std::endl;
                    }
                }

                // "If nderefine = 5
                else if (num_to_derefine == 5)
                {
                    // Accept as 8:2 derefine"
                    trace_out << "Accept as 8:2 " << std::endl;
                    //refiner.derefine_eight_to_two(tet_store,  node_connectivity, tet_id);
                    tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::eight_to_two);
                }

                // "If nderefine = 6
                else if (num_to_derefine == 6)
                {
                    // Accept as 8:1 derefine"
                    trace_out << "Accept as 8:1" << std::endl;
                    //refiner.derefine_eight_to_one(tet_store,  node_connectivity, tet_id);
                    tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::eight_to_one);
                }

                // "If nderefine = 0
                else {
                    tet_store.mark_derefinement_decision(tet_id, AMR::Derefinement_Case::skip);
                    // Deactivate all points"
                    for (auto child_id : children) {
                      deactivate_deref_tet_edges(child_id);
                    }
                    trace_out << "giving up with no deref decision because nderefine = 0" << std::endl;
                }
            }

            // If nothing changed during that round, break
            if (!tet_store.marked_derefinements.get_state_changed())
            {
                trace_out << "Terminating loop at iter " << iter << std::endl;
                break;
            }
            trace_out << "End iter " << iter << std::endl;
        }
        trace_out << "Deref Loop took " << iter << " rounds." << std::endl;
    }

    // TODO: document
    void mesh_adapter_t::perform_derefinement()
    {
        trace_out << "Perform deref" << std::endl;

        // Do derefinements
        for (const auto& kv : tet_store.tets)
        {
            size_t tet_id = kv.first;
            //size_t parent_id = 0;

            // TODO: Do I really want to loop all tets?

            // TODO: is this doing a double lookup?
            if (tet_store.has_derefinement_decision(tet_id))
            {
                trace_out << "Do derefine of " << tet_id << std::endl;
                //size_t parent_id = tet_store.get_parent_id(tet_id);
                //trace_out << "Parent = " << parent_id << std::endl;
                switch(tet_store.marked_derefinements.get(tet_id))
                {
                    case AMR::Derefinement_Case::two_to_one:
                        refiner.derefine_two_to_one(tet_store,node_connectivity,tet_id);
                        trace_out << "Completed derefine 2:1 of " << tet_id << std::endl;
                        break;
                    case AMR::Derefinement_Case::four_to_one:
                        refiner.derefine_four_to_one(tet_store,node_connectivity,tet_id);
                        trace_out << "Completed derefine 4:1 of " << tet_id << std::endl;
                        break;
                    case AMR::Derefinement_Case::four_to_two:
                        refiner.derefine_four_to_two(tet_store,node_connectivity,tet_id);
                        trace_out << "Completed derefine 4:2 of " << tet_id << std::endl;
                        break;
                    case AMR::Derefinement_Case::eight_to_one:
                        refiner.derefine_eight_to_one(tet_store,node_connectivity,tet_id);
                        trace_out << "Completed derefine 8:1 of " << tet_id << std::endl;
                        break;
                    case AMR::Derefinement_Case::eight_to_two:
                        refiner.derefine_eight_to_two(tet_store,node_connectivity,tet_id);
                        trace_out << "Completed derefine 8:2 of " << tet_id << std::endl;
                        break;
                    case AMR::Derefinement_Case::eight_to_four:
                        refiner.derefine_eight_to_four(tet_store,node_connectivity,tet_id);
                        trace_out << "Completed derefine 8:4 of " << tet_id << std::endl;
                        break;
                    case AMR::Derefinement_Case::skip:
                        // What do we do with skip?
                        break;
                }
                // Mark tet as not needing derefinement
                tet_store.marked_derefinements.erase(tet_id);
            }
        }

        node_connectivity.print();
        refiner.delete_intermediates_of_children(tet_store);
        tet_store.process_delete_list();
        tet_store.print_node_types();

        for (auto& kv : tet_store.edge_store.edges) {
           auto& local = kv.second;
           local.needs_derefining = 0;
        }
    }

}

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

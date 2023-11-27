#include <algorithms/refine.h>
#include <typed-geometry/tg.hh>
#include <types/result_fit_planes.h>
#include <types/point_cloud.h>
#include <clean-core/vector.hh>

#include <functions/fit_plane_thorugh_points.h>
#include <functions/progress_bar.h>

#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>
#include <polymesh/pm.hh>
#include <polymesh/algorithms/triangulate.hh>
#include <functions/crop_plane_with_aabb.h>
#include <clean-core/format.hh>


static void ShowBox(tg::aabb3 &box){
        pm::Mesh bm;
        auto bm_pos = pm::vertex_attribute<tg::pos3>(bm);
        pm::objects::add_cube(bm, [&](pm::vertex_handle v, float x, float y, float z) {

            x = (x < 0.5) ? box.min.x :box.max.x;
            y = (y < 0.5) ? box.min.y :box.max.y;
            z = (z < 0.5) ? box.min.z :box.max.z;

            bm_pos[v] = tg::pos3(x, y, z);
        });
        pm::triangulate_naive(bm);

        auto vertecies_filter_1 = [&](pm::vertex_handle h){return bm_pos[h];};
        auto faces_filter = [&](pm::face_handle h){
            auto vx = h.vertices().to_vector();
            std::array<int, 3> indexis{int(vx[0]),int(vx[1]),int(vx[2])};
            return indexis;
            };

        auto vertecies = bm.vertices().map(vertecies_filter_1).to_vector();
        auto faces = bm.faces().map(faces_filter).to_vector();

        auto ps_box = polyscope::registerSurfaceMesh("Box", vertecies, faces);
        ps_box->setTransparency(0.5);
}
static cc::vector<std::string> ShowPlanes(std::vector<linkml::Plane> planes, tg::aabb3 box){
    auto names = cc::vector<std::string>();

    auto faces_filter = [&](pm::face_handle h){
        auto vx = h.vertices().to_vector();
        std::array<int, 3> indexis{int(vx[0]),int(vx[1]),int(vx[2])};
        return indexis;
        };

    for (int i = 0; i<(int)planes.size(); i++){

        pm::Mesh m;
        auto pos = pm::vertex_attribute<tg::pos3>(m);

        auto vertecies_filter = [&](pm::vertex_handle h){return pos[h];};

        auto valid = crop_plane_with_aabb(m, pos, box, planes[i]);
        if (!valid) continue;
        auto name = cc::format("Plane {}", i);
        auto vertecies = m.vertices().map(vertecies_filter).to_vector();
        auto faces = m.faces().map(faces_filter).to_vector();

        polyscope::registerSurfaceMesh(name.c_str(), vertecies, faces);

        names.push_back(name.c_str());
    }
    
    return names;
}
static tg::aabb3 get_aabb(const std::vector<tg::pos3>& points){

    assert(points.size() >= 1 );

    auto p_min = points[0];
    auto p_max = points[0];

    for (int i = 1; i<points.size(); i++){
        if (points[i].x < p_min.x) p_min.x = points[i].x;
        if (points[i].y < p_min.y) p_min.y = points[i].y;
        if (points[i].z < p_min.z) p_min.z = points[i].z;

        if (points[i].x > p_max.x) p_max.x = points[i].x;
        if (points[i].y > p_max.y) p_max.y = points[i].y;
        if (points[i].z > p_max.z) p_max.z = points[i].z;

    }

    return tg::aabb3(p_min, p_max);


}
namespace linkml{
    std::vector<Plane> refine(point_cloud cloud, result_fit_planes & rs,  refinement_parameters const & param){


        polyscope::init();
        polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::ShadowOnly;
        polyscope::view::setUpDir(polyscope::UpDir::ZUp);

        auto ps_cloud = polyscope::registerPointCloud("Cloud", cloud.pts);
        ps_cloud->setPointRenderMode(polyscope::PointRenderMode::Sphere);
        ps_cloud->setPointRadius(0.0030);


        auto box = get_aabb(cloud.pts);

        // ShowBox(box);

        auto names = ShowPlanes(rs.planes, box);
        polyscope::show();
        for (auto& name : names)
            polyscope::removeStructure(name);


        std::vector<Plane> planes;
        std::copy(rs.planes.begin(), rs.planes.end(), std::back_inserter(planes));

        std::vector<std::vector<int>> indecies;
        std::copy(rs.indecies.begin(), rs.indecies.end(), std::back_inserter(indecies));

        auto pbar = util::progress_bar(planes.size(), "Plane Refinement");


        for (int i = 0; i < (int)planes.size(); i++){

            std::vector<int> sel;

            // Check angle
            for (auto j = i+1; j < (int)planes.size(); j++){

                auto dot = tg::dot(planes[i].normal, planes[j].normal);
                if (dot < tg::cos(param.angle_threashhold)){
                    sel.push_back(j);
                }
            }


            auto to_be_deleted = std::vector<int>();
            // Check overlap and merge
            for (auto & j: sel){

                auto A = planes[i];
                auto B = planes[j];


                auto A_idx =indecies[i];
                auto B_idx =indecies[j];

                auto n_point_of_B_in_A = (long)tg::sum(B_idx, [&](int idx){ return tg::distance(A,cloud.pts[idx]) < param.distance_threshhold; });
                auto n_point_of_A_in_B = (long)tg::sum(A_idx, [&](int idx){ return tg::distance(B,cloud.pts[idx]) < param.distance_threshhold; });


                auto Nt = (long)tg::min( A_idx.size(),B_idx.size())/5;

                if (n_point_of_A_in_B > Nt and n_point_of_B_in_A > Nt){
                    to_be_deleted.push_back(j);
                    // Or mark them as empty
                    std::vector<int> merged;
                    std::copy(A_idx.begin(), A_idx.end(), std::back_inserter(merged));
                    std::copy(B_idx.begin(), B_idx.end(), std::back_inserter(merged));
                    planes[i] = fit_plane_thorugh_points(cloud, merged);
                    indecies[i] = merged;
                }

            }

            // Erase merged itmes
            for (auto it = to_be_deleted.rbegin(); it != to_be_deleted.rend(); ++it){
                size_t idx = *it;
                auto p_it = std::next(planes.begin(), idx );
                auto i_it = std::next(indecies.begin(), idx ); 
                planes.erase(p_it);
                indecies.erase(i_it);
            }

            pbar.update(1+to_be_deleted.size());

        }


        ShowPlanes(planes, box);

        polyscope::show();

        return planes;

    }

}


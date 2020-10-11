#pragma once
#include "UniEngine.h"
#include "TreeManager.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/IO/facets_in_complex_2_to_triangle_mesh.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/property_map.h>
#include <CGAL/IO/read_xyz_points.h>
#include <CGAL/compute_average_spacing.h>
#include <CGAL/Polygon_mesh_processing/distance.h>
#include <boost/iterator/transform_iterator.hpp>
#include <vector>
#include <fstream>
// Types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::FT FT;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef std::pair<Point, Vector> Point_with_normal;
typedef CGAL::First_of_pair_property_map<Point_with_normal> Point_map;
typedef CGAL::Second_of_pair_property_map<Point_with_normal> Normal_map;
typedef Kernel::Sphere_3 Sphere;
typedef std::vector<Point_with_normal> PointList;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef CGAL::Poisson_reconstruction_function<Kernel> Poisson_reconstruction_function;
typedef CGAL::Surface_mesh_default_triangulation_3 STr;
typedef CGAL::Surface_mesh_complex_2_in_triangulation_3<STr> C2t3;
typedef CGAL::Implicit_surface_3<Kernel, Poisson_reconstruction_function> Surface_3;


#include <CGAL/Simple_cartesian.h>
#include <CGAL/Advancing_front_surface_reconstruction.h>

typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3  Point_3A;
typedef std::array<std::size_t, 3> Facet;


#include <CGAL/Scale_space_surface_reconstruction_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/IO/read_off_points.h>
#include <CGAL/Timer.h>
typedef CGAL::Exact_predicates_inexact_constructions_kernel     Kernel;
typedef CGAL::Scale_space_surface_reconstruction_3<Kernel>    Reconstruction;
typedef Reconstruction::Facet_const_iterator                   Facet_iterator;


struct Perimeter {
    double bound;
    Perimeter(double bound)
        : bound(bound)
    {}
    template <typename AdvancingFront, typename Cell_handle>
    double operator() (const AdvancingFront& adv, Cell_handle& c,
        const int& index) const
    {
        // bound == 0 is better than bound < infinity
        // as it avoids the distance computations
        if (bound == 0) {
            return adv.smallest_radius_delaunay_sphere(c, index);
        }
        // If perimeter > bound, return infinity so that facet is not used
        double d = 0;
        d = sqrt(squared_distance(c->vertex((index + 1) % 4)->point(),
            c->vertex((index + 2) % 4)->point()));
        if (d > bound) return adv.infinity();
        d += sqrt(squared_distance(c->vertex((index + 2) % 4)->point(),
            c->vertex((index + 3) % 4)->point()));
        if (d > bound) return adv.infinity();
        d += sqrt(squared_distance(c->vertex((index + 1) % 4)->point(),
            c->vertex((index + 3) % 4)->point()));
        if (d > bound) return adv.infinity();
        // Otherwise, return usual priority value: smallest radius of
        // delaunay sphere
        return adv.smallest_radius_delaunay_sphere(c, index);
    }
};


namespace TreeUtilities {
	class CrownSurfaceRecon {
	public:
		// Poisson options
		FT sm_angle = 10.0; // Min triangle angle in degrees.
		FT sm_radius = 0.7; // Max triangle size w.r.t. point set average spacing.
		FT sm_distance = 0.1; // Surface Approximation error w.r.t. point set average spacing.
		// Reads the point set file in points[].
		// Note: read_xyz_points_and_normals() requires an iterator over points
		// + property maps to access each point's position and normal.
		
		void PoissonConstruct(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::shared_ptr<Mesh> mesh);
		void AdvancingFrontConstruct(std::vector<glm::vec3>& positions, std::shared_ptr<Mesh> mesh);
        void ScaleSpaceConstruct(std::vector<glm::vec3>& positions, std::shared_ptr<Mesh> mesh);
	};
}
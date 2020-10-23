#include "CrownSurfaceRecon.h"
/*
void TreeUtilities::CrownSurfaceRecon::PoissonConstruct(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::shared_ptr<Mesh> mesh)
{
	PointList points;
	for(int i = 0; i < positions.size(); i++)
	{
		points.emplace_back(std::make_pair(
			Point(positions[i].x, positions[i].y, positions[i].z),
			Vector(normals[i].x, normals[i].y, normals[i].z)
		));
	}
	// Creates implicit function from the read points using the default solver.
	// Note: this method requires an iterator over points
	// + property maps to access each point's position and normal.
	Poisson_reconstruction_function function(points.begin(), points.end(), Point_map(), Normal_map());
	// Computes the Poisson indicator function f()
	// at each vertex of the triangulation.
	if (!function.compute_implicit_function())
		return;
	// Computes average spacing
	FT average_spacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>
		(points, 6  knn = 1 ring
			CGAL::parameters::point_map(Point_map()));
	// Gets one point inside the implicit surface
	// and computes implicit function bounding sphere radius.
	Point inner_point = function.get_inner_point();
	Sphere bsphere = function.bounding_sphere();
	FT radius = std::sqrt(bsphere.squared_radius());
	// Defines the implicit surface: requires defining a
	// conservative bounding sphere centered at inner point.
	FT sm_sphere_radius = 5.0 * radius;
	FT sm_dichotomy_error = sm_distance * average_spacing / 1000.0; // Dichotomy error must be << sm_distance
	Surface_3 surface(function,
		Sphere(inner_point, sm_sphere_radius * sm_sphere_radius),
		sm_dichotomy_error / sm_sphere_radius);
	// Defines surface mesh generation criteria
	CGAL::Surface_mesh_default_criteria_3<STr> criteria(sm_angle,  // Min triangle angle (degrees)
		sm_radius * average_spacing,  // Max triangle size
		sm_distance * average_spacing); // Approximation error
// Generates surface mesh with manifold option
	STr tr; // 3D Delaunay triangulation for surface mesh generation
	C2t3 c2t3(tr); // 2D complex in 3D Delaunay triangulation
	CGAL::make_surface_mesh(c2t3,                                 // reconstructed mesh
		surface,                              // implicit surface
		criteria,                             // meshing criteria
		CGAL::Manifold_with_boundary_tag());  // require manifold mesh
	if (tr.number_of_vertices() == 0)
		return;
	// saves reconstructed surface mesh
	Polyhedron output_mesh;
	
	CGAL::facets_in_complex_2_to_triangle_mesh(c2t3, output_mesh);

	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	for (auto i = output_mesh.facets_begin(); i != output_mesh.facets_end(); ++i)
	{
		auto j = i->facet_begin();
		do
		{
			indices.push_back(std::distance(output_mesh.vertices_begin(), j->vertex()));
		} while (++j != i->facet_begin());
	}
	Vertex archetype;
	for(auto i = output_mesh.vertices_begin(); i != output_mesh.vertices_end(); ++i)
	{
		archetype.Position = glm::vec3(i->point().x(), i->point().y(), i->point().z());
		archetype.TexCoords0 = glm::vec2(0);
		vertices.push_back(archetype);
	}
	mesh->SetVertices(17, vertices, indices, true);
}
*/
void TreeUtilities::CrownSurfaceRecon::AdvancingFrontConstruct(std::vector<glm::vec3>& positions,
	std::shared_ptr<Mesh> mesh)
{
	double per = 0;
	double radius_ratio_bound = 5.0;
	std::vector<Point_3A> points;
	std::vector<Facet> facets;

	for(auto& i : positions)
	{
		points.emplace_back(i.x, i.y, i.z);
	}
	
	Perimeter perimeter(per);
	CGAL::advancing_front_surface_reconstruction(points.begin(),
		points.end(),
		std::back_inserter(facets),
		perimeter,
		radius_ratio_bound);

	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	for (auto& i : facets)
	{
		indices.push_back(i[0]);
		indices.push_back(i[1]);
		indices.push_back(i[2]);
	}
	Vertex archetype;
	for (auto& i : points)
	{
		archetype.Position = glm::vec3(i.x(), i.y(), i.z());
		archetype.TexCoords0 = glm::vec2(0);
		vertices.push_back(archetype);
	}
	mesh->SetVertices(17, vertices, indices, true);
}

void TreeUtilities::CrownSurfaceRecon::ScaleSpaceConstruct(std::vector<glm::vec3>& positions,
	std::shared_ptr<Mesh> mesh)
{
	std::vector<Point> points;
	for (auto& i : positions)
	{
		points.emplace_back(i.x, i.y, i.z);
	}
	Reconstruction reconstruct(points.begin(), points.end());
	reconstruct.increase_scale(4);
	reconstruct.reconstruct_surface();
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	for (auto i = reconstruct.facets_begin(); i != reconstruct.facets_end(); ++i)
	{
		indices.push_back(i->at(0));
		indices.push_back(i->at(1));
		indices.push_back(i->at(2));
	}
	Vertex archetype;
	for (auto& i : points)
	{
		archetype.Position = glm::vec3(i.x(), i.y(), i.z());
		archetype.TexCoords0 = glm::vec2(0);
		vertices.push_back(archetype);
	}
	mesh->SetVertices(17, vertices, indices, true);
}

#include <iostream>
#include <fstream>
#include <cassert>

using std::cout,
	std::ifstream;

int main(int argc, char * argv[])
{
	ifstream fin{"../maps/roads.graph"};
	assert(fin.is_open());

	// read header
	char format;
	int node_param_count,
		curve_param_count,
		area_param_count,
		curve_extrem_param_count,
		curve_verts_param_count,
		area_curve_param_count,
		area_subgraph_param_count;

	fin >> format
		>> node_param_count
		>> curve_param_count
		>> area_param_count
		>> curve_extrem_param_count
		>> curve_verts_param_count
		>> area_curve_param_count
		>> area_subgraph_param_count;

	cout << "node parameters: " << node_param_count << "\n"
		<< "curve parameters: " << curve_param_count << "\n"
		<< "area parameters: " << area_param_count << "\n"
		<< "curve extremities parameters: " << curve_extrem_param_count << "\n"
		<< "curve vertices parameters: " << curve_verts_param_count << "\n"
		<< "area's curve parameters: " << area_curve_param_count << "\n"
		<< "area's subgraphs parameters: " << area_subgraph_param_count << "\n";

	// read nodes
	int node_count;
	fin >> node_count;

	// just consume node section
	for (int i = 0; i < node_count; ++i)
	{
		float x, y;
		int connected_curves_count;

		fin >> x >> y >> connected_curves_count;

		cout << "  node #" << i + 1 << ": (" << x << ", " << y << ")\n";

		int curve_id;
		for (int j = 0; j < connected_curves_count; ++j)
			fin >> curve_id;
	}

	cout << "node count: " << node_count << "\n";

	// read curves
	int curve_count;
	fin >> curve_count;

	cout << "curve count: " << curve_count << "\n";

	for (int i = 0; i < curve_count; ++i)
	{
		cout << "  curve #" << i+1 << ": ";

		int vertex_count,
			type,
			start_node;
		float width;

		fin >> vertex_count
			>> width
			>> type
			>> start_node;

		cout << "width=" << width << ", "
			<< "n0=" << start_node << ", "
			<< "vertices=" << vertex_count << ", ";

		assert(vertex_count > 1);

		for (int j = 0; j < vertex_count-2; ++j)
		{
			float x, y;
			int control;

			fin >> x >> y >> control;

			cout << "(" << x << ", " << y << ")";
			if (control)
				cout << "c";

			cout << ", ";
		}

		int end_node,
			left_area_id,
			right_area_id,
			ancestor_curve_id;

		fin >> end_node
			>> left_area_id
			>> right_area_id
			>> ancestor_curve_id;

		cout << "n1=" << end_node << "\n";
	}

	// read areas
	int area_count;
	fin >> area_count;

	cout << "area count: " << area_count << "\n";

	for (int i = 0; i < area_count; ++i)
	{
		int curves_in_area_count,
			user_data,
			has_subgraphs;

		fin >> curves_in_area_count
			>> user_data
			>> has_subgraphs;

		for (int j = 0; j < curves_in_area_count; ++j)
		{
			int curve_id,
				curve_orientation,
				ancestor_area;

			fin >> curve_id
				>> curve_orientation
				>> ancestor_area;
		}
	}

	cout << "done!.\n";
	return 0;
}

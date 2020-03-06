#include <algorithm>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/io/dsv/write.hpp>
#include "geometry/glm_geometry.hpp"
#include "geometry/box2.hpp"

namespace bg = boost::geometry;

using std::max;
using std::vector;
using std::cout;
using glm::vec2,
	glm::vec3;
using bg::envelope,
	bg::return_envelope,
	bg::append,
	bg::dsv;
using geom::box2;

using linestring = boost::geometry::model::linestring<vec2>;

// lat, lon locations
vector<vec2> road = {
	{50.1067295f, 14.4498393f},
	{50.1059568f, 14.4498711f},
	{50.1054599f, 14.449893f},
	{50.1050815f, 14.449901f}
};

void dump_road(vector<vec2> const & r, vec3 const & tile_offset);

int main(int argc, char * argv[])
{
	linestring ls;
	for (vec2 const & n : road)
		append(ls, n);

	box2 bb = return_envelope<box2>(ls);
	cout << "road bounding box is " << dsv(bb) << "\n";

	cout << "road area width=" << geom::width(bb) << ", height="
		<< geom::height(bb) << "\n";

	float tile_width = max(geom::width(bb), geom::height(bb));

	cout << "road:\n";
	vec3 tile_offset = vec3{bb.min_corner().x, bb.min_corner().y, 1/tile_width};
	dump_road(road, tile_offset);

	cout << "done!\n";
	return 0;
}

void dump_road(vector<vec2> const & r, vec3 const & tile_offset)
{
	cout << "nodes=" << size(r) << "\n";
	for (vec2 const & n : r)
	{
		vec2 n_ = (n - vec2{tile_offset.x, tile_offset.y}) * tile_offset.z;
		cout << dsv(n_) << ", ";
	}
	cout << "\n";
}

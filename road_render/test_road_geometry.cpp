// shows how to use envelope algorithm witha a string of points
#include <algorithm>
#include <vector>
#include <iostream>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/io/dsv/write.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <glm/glm.hpp>
#include "geometry/glm_geometry.hpp"

namespace bg = boost::geometry;

using std::vector,
	std::size,
	std::max,
	std::cout;
using bg::return_envelope,
	bg::num_points,
	bg::append,
	bg::dsv,
	bg::read_wkt;
using glm::vec2,
	glm::vec3,
	glm::length;

using linestring = bg::model::linestring<vec2>;
using box = bg::model::box<vec2>;

void dump_road_geometry(linestring const & r, float road_width,
	vec3 const & tile_offset = vec3{0,0,1});

int main(int argc, char * argv[])
{
	linestring road;
	read_wkt("LINESTRING(14.4555004 50.1055493, 14.4554394 50.1054522, 14.4554295 50.1053666,"
		"14.4554618 50.105246, 14.4555489 50.1051538, 14.4556708 50.1051275,"
		"14.4557598 50.1051296)", road);

	box bb = return_envelope<box>(road);
	cout << "road bounding box is " << dsv(bb) << "\n"
		<< "there are " << num_points(road) << " nodes there\n";

	vec2 bb_size = bb.max_corner() - bb.min_corner();
	float area_size = max(bb_size.x, bb_size.y);
	float screen_width = 400;
	vec3 tile_offset = vec3{bb.min_corner(), 1/area_size * screen_width/2};

	dump_road_geometry(road, 0.00001f, tile_offset);

	cout << "done!\n";

	return 0;
}

void dump_road_geometry(linestring const & r, float road_width, vec3 const & tile_offset)
{
	size_t node_count = size(r);

	float w = road_width/2;
	for (size_t i = 0; i < node_count; ++i)
	{
		vec2 const & cur = r[i];
		vec2 ds;

		if (i == 0)  // first node
		{
			vec2 const & next = r[i+1];
			ds = glm::normalize(next - cur);
		}
		else if (i == node_count-1)  // last node
		{
			vec2 const & prev = r[i-1];
			ds = glm::normalize(cur - prev);
		}
		else // middle nodes
		{
			vec2 const & prev = r[i-1],
			next = r[i+1];

			ds = glm::normalize(cur - prev);
			ds += glm::normalize(next - cur);
			float l = (ds.x*ds.x + ds.y*ds.y) * 0.5f;
			ds /= l;
		}

		ds *= w;

		vec2 left = vec2{cur.x - ds.y, cur.y + ds.x},
			right = vec2{cur.x + ds.y, cur.y - ds.x};

		left = (left - vec2{tile_offset.x, tile_offset.y}) * tile_offset.z;
		right = (right - vec2{tile_offset.x, tile_offset.y}) * tile_offset.z;

		cout << dsv(cur) << " -> " << dsv(left) << ", " << dsv(right)
			<< ", length=" << length(right - left) << "\n";
	}
}

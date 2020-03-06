#pragma once
#include <vector>
#include <string>
#include <boost/geometry.hpp>
#include <glm/vec2.hpp>
#include "geometry/glm_geometry.hpp"

using vec2 = glm::vec2;
using box = boost::geometry::model::box<vec2>;

enum class road_type  //!< see https://wiki.openstreetmap.org/wiki/Key:highway for more details
{
	motorway,
	trunk,
	primary,
	secondary,
	tertiary,
	unclassified,
	residential,
	service,
	living_street,
	pedestrian,
	track,
	bus_guideway,
	escape,
	raceway,
	road,
	footway,
	bridleway,
	steps,
	path,
	cycleway,
	construction,
	count  //!< number of road types
};

struct road
{
	road_type type;
	std::vector<vec2> geometry;
};

//! read roads from OSM file
void read_roads(std::string const & osm_file, std::vector<road> & roads,
	box & bounding_box);

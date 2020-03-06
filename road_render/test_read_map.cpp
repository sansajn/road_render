// show how te read OSM map
#include <iostream>
#include "road.hpp"

using std::vector,
	std::cout,
	std::endl;
using boost::geometry::wkt,
	boost::geometry::dsv;

int main(int argc, char * argv[])
{
	vector<road> roads;
	box bounding_box;
	read_roads("../maps/highways.osm", roads, bounding_box);

	cout << "there are " << roads.size() << " roads\n"
		<< "in a " << dsv(bounding_box) << " area\n";

	// wkt can be also used there
	cout << "area=" << wkt(bounding_box) << "\n";

	cout << "done" << endl;

	return 0;
}

#include <map>
#include <list>
#include <utility>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/io/any_input.hpp>
#include "road.hpp"

using std::string,
	std::vector,
	std::map,
	std::list,
	std::pair;
using boost::geometry::expand,
	boost::geometry::make_inverse;


class road_reader : public osmium::handler::Handler
{
public:
	using way_list = list<pair<vector<size_t>, road_type>>;
	using node_map = map<size_t, osmium::Location>;

	road_reader(vector<road> & roads)
		: _bbox{make_inverse<box>()}
		, _roads{roads}
	{}

	void read(string const & osm_file);
	box const & bounding_box() const {return _bbox;}

	// handler API
	void way(osmium::Way & w);
	void node(osmium::Node & n);

private:
	way_list _ways;
	node_map _nodes;
	box _bbox;
	vector<road> & _roads;
};


void read_roads(std::string const & osm_file, std::vector<road> & roads,
	box & bounding_box)
{
	road_reader rr{roads};
	rr.read(osm_file);
	bounding_box = rr.bounding_box();
}


void road_reader::read(string const & osm_file)
{
	_roads.clear();
	_roads.shrink_to_fit();

	osmium::io::Reader osm{osm_file.c_str()};
	osmium::apply(osm, *this);

	_bbox = make_inverse<box>();
	_roads.reserve(_ways.size());

	while (!_ways.empty())
	{
		road r;

		r.type = _ways.front().second;

		// geometry
		for (size_t nid : _ways.front().first)
		{
			osmium::Location & loc = _nodes[nid];
			if (loc.valid())
			{
				vec2 p{loc.lon(), loc.lat()};
				expand(_bbox, p);
				r.geometry.push_back(p);
			}
		}

		if (!r.geometry.empty())
			_roads.push_back(r);

		_ways.pop_front();
	}

	_nodes.clear();
}

road_type to_road_type(char const * value)
{
	// TODO: slow implementation
	if (strcmp(value, "motorway") == 0)
		return road_type::motorway;
	else if (strcmp(value, "trunk") == 0)
		return road_type::trunk;
	else if (strcmp(value, "primary") == 0)
		return road_type::primary;
	else if (strcmp(value, "secondary") == 0)
		return road_type::secondary;
	else if (strcmp(value, "tertiary") == 0)
		return road_type::tertiary;
	else if (strcmp(value, "unclassified") == 0)
		return road_type::unclassified;
	else if (strcmp(value, "residential") == 0)
		return road_type::residential;
	else if (strcmp(value, "service") == 0)
		return road_type::service;
	else if (strcmp(value, "living_street") == 0)
		return road_type::living_street;
	else if (strcmp(value, "pedestrian") == 0)
		return road_type::pedestrian;
	else if (strcmp(value, "track") == 0)
		return road_type::track;
	else if (strcmp(value, "bus_guideway") == 0)
		return road_type::bus_guideway;
	else if (strcmp(value, "escape") == 0)
		return road_type::escape;
	else if (strcmp(value, "raceway") == 0)
		return road_type::raceway;
	else if (strcmp(value, "road") == 0)
		return road_type::road;
	else if (strcmp(value, "footway") == 0)
		return road_type::footway;
	else if (strcmp(value, "bridleway") == 0)
		return road_type::bridleway;
	else if (strcmp(value, "steps") == 0)
		return road_type::steps;
	else if (strcmp(value, "path") == 0)
		return road_type::path;
	else if (strcmp(value, "cycleway") == 0)
		return road_type::cycleway;
	else if (strcmp(value, "construction") == 0)
		return road_type::construction;
	else
		return road_type::road;  // generic road type
}

void road_reader::way(osmium::Way & w)
{
	// props
	char const * value = w.tags()["highway"];
	if (!value)
		return;   // not a road

	// nodes
	vector<size_t> tmp;
	for (osmium::NodeRef const & n : w.nodes())
		tmp.push_back(n.ref());

	pair<vector<size_t>, road_type> p;
	p.first = move(tmp);
	p.second = to_road_type(value);
	_ways.push_back(move(p));
}

void road_reader::node(osmium::Node & n)
{
	_nodes[n.id()] = n.location();
}


#ifndef LAYER_H
#define LAYER_H

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <boost/noncopyable.hpp>

#include "coord.hpp"
#include "surface.hpp"
#include "mill.hpp"

class Layer : boost::noncopyable
{
public:
	Layer( const string& name, boost::shared_ptr<Surface> surface, boost::shared_ptr<RoutingMill> manufacturer, bool backside, bool mirror_absolute );
	
	vector< boost::shared_ptr<icoords> > get_toolpaths();
	boost::shared_ptr<RoutingMill> get_manufacturer();
	string get_name() { return name; };
	void add_mask( boost::shared_ptr<Layer>);

private:
	string name;
	bool mirrored;
	bool mirror_absolute;
	boost::shared_ptr<Surface> surface;
	boost::shared_ptr<RoutingMill>    manufacturer;

	friend class Board;
};

#endif // LAYER_H

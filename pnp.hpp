#ifndef PNP_H
#define PNP_H

#endif //PNP_H

#include <string>
#include <stdexcept>


#include <iostream>
#include <fstream>
#include "coord.hpp"
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>


using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::fstream;
using std::vector;
using std::pair;


struct component {
	string Ref;
	string Val;
	string Package;
	icoordpair Position;
	int Rotation;
	bool Top;
};

class PnpData
{
public:
	void loadfile( string posfile);
	vector<component> components;
	

};

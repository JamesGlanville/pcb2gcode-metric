#include "pnp.hpp"

void
PnpData::loadfile( string posfile )
{
		std::ifstream stream;
	try
	{
		try {
			stream.open(posfile.c_str());
		} catch( std::exception& e ) {
			cerr << "Error parsing configuration file \"" << posfile << "\": "
			     << e.what() << endl;
		}
	}
	catch( std::exception& e ) 
	{
		cerr << "Error reading configuration file \"" << posfile << "\": " << e.what() << endl;
	}
	
	string line;
	vector<string> strings;
	vector<string> tokens;
	component currentcomponent;
	
	while(stream.good() && (getline(stream, line)))
	{
		if (line.find("#")==string::npos)
			{
			boost::split(strings, line,boost::is_any_of("\t "));
			tokens.clear();
			BOOST_FOREACH(string st, strings)
			{
				if (st!=""){
				tokens.push_back(st);}
			}
		}

		if (tokens.size() == 7)
		{
			currentcomponent.Ref=tokens[0];
			currentcomponent.Val=tokens[1];
			currentcomponent.Package=tokens[2];
			currentcomponent.Position=pair<ivalue_t,ivalue_t>(boost::lexical_cast<double>(tokens[3]),boost::lexical_cast<double>(tokens[4]));
			currentcomponent.Rotation=boost::lexical_cast<double>(tokens[5]);
			if (tokens[6]=="F.Cu") {currentcomponent.Top=true;}

			components.push_back(currentcomponent);
		}
	}	
	stream.close();
	


}

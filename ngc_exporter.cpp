/*
 * This file is part of pcb2gcode.
 * 
 * Copyright (C) 2009, 2010 Patrick Birnzain <pbirnzain@users.sourceforge.net>
 * 
 * pcb2gcode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * pcb2gcode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with pcb2gcode.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ngc_exporter.hpp"

#include <boost/foreach.hpp>

#include <iostream>
#include <iomanip>

#define PI 3.141593

using namespace std;

NGC_Exporter::NGC_Exporter( boost::shared_ptr<Board> board ) : Exporter(board)
{
	this->board = board;
	bDoSVG = false;
}


void
NGC_Exporter::set_svg_exporter( boost::shared_ptr<SVG_Exporter> svgexpo )
{
	this->svgexpo = svgexpo;
	bDoSVG = true;
}


void
NGC_Exporter::add_header( string header )
{
        this->header.push_back(header);
}

void
NGC_Exporter::export_all(boost::program_options::variables_map& options)
{
	BOOST_FOREACH( string layername, board->list_layers() ) {
		std::stringstream option_name;
		option_name << layername << "-output";
		string of_name = options[option_name.str()].as<string>();
		cerr << "Current Layer: " << layername << ", exporting to " << of_name << "." << endl;
		export_layer( board->get_layer(layername), of_name);
	}
}

double
NGC_Exporter::get_tolerance( void )
{
	// set maximum deviation to 5 pixels to ensure smooth movement
	return 5.0/this->board->get_dpi();
}

void
NGC_Exporter::export_layer( boost::shared_ptr<Layer> layer, string of_name )
{
	string layername = layer->get_name();
	boost::shared_ptr<RoutingMill> mill = layer->get_manufacturer();

	bool bSvgOnce = TRUE;
	
	// open output file
	std::ofstream of; of.open( of_name.c_str() );

	// write header to .ngc file
        BOOST_FOREACH( string s, header )
        {
                of << "( " << s << " )" << endl;
        }
        of << endl;

        of.setf( ios_base::fixed );
        of.precision(5);
	of << setw(7);

	// preamble
	of << ""
       #ifndef METRIC_OUTPUT
	   << "G20     ( Units == INCHES.             )\n"
       #else
       << "G21     ( Units == MM.                 )\n"
       #endif
	   << "G90     ( Absolute coordinates.        )\n"
	   << "S" << left << mill->speed << "  ( RPM spindle speed.           )\n"
	   << "M3      ( Spindle on clockwise.        )\n"
	   << endl;

	of << "G64 P" << get_tolerance() << " ( set maximum deviation from commanded toolpath )\n"
	   << endl;

	
	//SVG EXPORTER
	if (bDoSVG) {
		//choose a color
		svgexpo->set_rand_color();
	}
	
	
    cout << "exporting_layer";

	if (layername == "paste") 
	{
		boost::shared_ptr<Paster> paster = boost::dynamic_pointer_cast<Paster>( mill );

		double pasteextruded = 0;
		of << "G92 E0\n";
		of << "M82\n"; //Use absolute distances for extrusion.
		of << "G1 F" << CONVERT_UNITS(paster->pastespeed) << " E" << CONVERT_UNITS(paster->initialslack) << endl;
		of << "G92 E0" << endl;
	//	cout << "placeholder, paste" <<endl;
		BOOST_FOREACH( boost::shared_ptr<icoords> path, layer->get_toolpaths() )
		{

			of << "G04 P0 ( dwell for no time -- G64 should not smooth over this point )\n";
			//of << "G00 Z" << CONVERT_UNITS(mill->zsafe) << " ( retract )\n" << endl;
			of << "G1 Z" << CONVERT_UNITS(mill->zwork + paster->pastethickness) <<  endl;
			//of << "G00 X" << CONVERT_UNITS(path->begin()->first) << " Y" << CONVERT_UNITS(path->begin()->second) << " ( rapid move to begin. )\n";

			icoords::iterator iter = path->begin();
			icoords::iterator last = path->end(); // initializing to quick & dirty sentinel value
			icoords::iterator peek;

			icoords paste_corners;
			icoordpair center;
			ivalue_t area;
				
			//SVG EXPORTER
			if (bDoSVG) {						
				svgexpo->move_to(path->begin()->first, path->begin()->second);
				bSvgOnce = TRUE;
			}			

			while( iter != path->end() ) {
				peek = iter + 1;
				if( /* it's necessary to write the coordinates if... */
						last == path->end() || /* it's the beginning */
						peek == path->end() || /* it's the end */
						!( /* or if neither of the axis align */
							( last->first == iter->first && iter->first == peek->first ) || /* x axis aligns */
							( last->second == iter->second && iter->second == peek->second ) /* y axis aligns */
						)
						/* no need to check for "they are on one axis but iter is outside of last and peek" becaus that's impossible from how they are generated */
				  ) {
					  paste_corners.push_back(pair<ivalue_t,ivalue_t>(CONVERT_UNITS(iter->first),CONVERT_UNITS(iter->second)));
//					of << "G01 X" << CONVERT_UNITS(iter->first) << " Y" << CONVERT_UNITS(iter->second) << " F" << CONVERT_UNITS(mill->feed) << endl;
					
					//SVG EXPORTER
					if (bDoSVG) if (bSvgOnce) {svgexpo->line_to(iter->first, iter->second);}
					}
				last = iter;
				++iter;
			}	
				
//			cout << "ASSUMING RECTANGULAR PAD!!" <<endl;
			area = abs(paste_corners.at(0).first - paste_corners.at(1).first + paste_corners.at(0).second - paste_corners.at(1).second) * 
					abs(paste_corners.at(3).second - paste_corners.at(0).second + paste_corners.at(3).first - paste_corners.at(0).first);
			center = pair<ivalue_t,ivalue_t>((paste_corners.at(0).first + paste_corners.at(2).first)/2,(paste_corners.at(0).second + paste_corners.at(1).second)/2);
//			cout << "Blob of paste of area = " <<area << " at center X= " <<center.first << " Y= " <<center.second<<endl;
			pasteextruded+=(area*CONVERT_UNITS(paster->pastethickness)/(M_PI*(pow(CONVERT_UNITS(paster->pastewidth)/2,2))));
//			cout << "pasteextruded" << pasteextruded <<"pastethickness"<<CONVERT_UNITS(paster->pastethickness)<<endl;

			//RETRACT:
			of << "G1 F" << CONVERT_UNITS(paster->pastespeed) << " E" << pasteextruded - CONVERT_UNITS(paster->retraction_distance)<< endl;
			
			of << "G1 F" << CONVERT_UNITS(paster->feed) << " X" << center.first << " Y" <<center.second << endl;
			of << "G1 F" << CONVERT_UNITS(paster->pastespeed) << " E" << pasteextruded << endl;

//			cout << "retraction_distance" << CONVERT_UNITS(paster->retraction_distance) <<endl;

			//SVG EXPORTER
			if (bDoSVG) {
				svgexpo->close_path();
				bSvgOnce = FALSE;
			}
		}
	}
	
	else
	{
		// contours
		BOOST_FOREACH( boost::shared_ptr<icoords> path, layer->get_toolpaths() )
			{
			// retract, move to the starting point of the next contour
			of << "G04 P0 ( dwell for no time -- G64 should not smooth over this point )\n";
			of << "G00 Z" << CONVERT_UNITS(mill->zsafe) << " ( retract )\n" << endl;
					of << "G00 X" << CONVERT_UNITS(path->begin()->first) << " Y" << CONVERT_UNITS(path->begin()->second) << " ( rapid move to begin. )\n";
			
				
			//SVG EXPORTER
			if (bDoSVG) {						
				svgexpo->move_to(path->begin()->first, path->begin()->second);
				bSvgOnce = TRUE;
			}
				
			/** if we're cutting, perhaps do it in multiple steps, but do isolations just once.
			 *  i know this is partially repetitive, but this way it's easier to read
			 */
			boost::shared_ptr<Cutter> cutter = boost::dynamic_pointer_cast<Cutter>( mill );
			if( cutter && cutter->do_steps ) {
				// cutting
				double z_step = cutter->stepsize;
				double z = mill->zwork + z_step * abs( int( mill->zwork / z_step ) );

				while( z >= mill->zwork ) {
					of << "G01 Z" << CONVERT_UNITS(z) << " F" << CONVERT_UNITS(mill->feed) << " ( plunge. )\n";
					of << "G04 P0 ( dwell for no time -- G64 should not smooth over this point )\n";

					icoords::iterator iter = path->begin();
					icoords::iterator last = path->end(); // initializing to quick & dirty sentinel value
					icoords::iterator peek;
					while( iter != path->end() ) {
						peek = iter + 1;
						if( /* it's necessary to write the coordinates if... */
								last == path->end() || /* it's the beginning */
								peek == path->end() || /* it's the end */
								!( /* or if neither of the axis align */
									( last->first == iter->first && iter->first == peek->first ) || /* x axis aligns */
									( last->second == iter->second && iter->second == peek->second ) /* y axis aligns */
								)
								/* no need to check for "they are on one axis but iter is outside of last and peek" becaus that's impossible from how they are generated */
						  ) {
							of << "G01 X" << CONVERT_UNITS(iter->first) << " Y" << CONVERT_UNITS(iter->second) << " F" << CONVERT_UNITS(mill->feed) << endl;
							
							//SVG EXPORTER
							if (bDoSVG) {
								if (bSvgOnce) svgexpo->line_to(iter->first, iter->second);
							}
						}
						last = iter;
						++iter;
					}
					//SVG EXPORTER
					if (bDoSVG) {
						svgexpo->close_path();
						bSvgOnce = FALSE;
					}
				
					z -= z_step;
				}
			} else {
				// isolating
				of << "G01 Z" << CONVERT_UNITS(mill->zwork) << " F" << CONVERT_UNITS(mill->feed) << " ( plunge. )\n";
				of << "G04 P0 ( dwell for no time -- G64 should not smooth over this point )\n";

				icoords::iterator iter = path->begin();
				icoords::iterator last = path->end(); // initializing to quick & dirty sentinel value
				icoords::iterator peek;
				while( iter != path->end() ) {
					peek = iter + 1;
					if( /* it's necessary to write the coordinates if... */
							last == path->end() || /* it's the beginning */
							peek == path->end() || /* it's the end */
							!( /* or if neither of the axis align */
								( last->first == iter->first && iter->first == peek->first ) || /* x axis aligns */
								( last->second == iter->second && iter->second == peek->second ) /* y axis aligns */
							)
							/* no need to check for "they are on one axis but iter is outside of last and peek" becaus that's impossible from how they are generated */
					  ) {
						of << "G01 X" << CONVERT_UNITS(iter->first) << " Y" << CONVERT_UNITS(iter->second) << " F" << CONVERT_UNITS(mill->feed) << endl;
						
						//SVG EXPORTER
						if (bDoSVG) if (bSvgOnce) svgexpo->line_to(iter->first, iter->second);

					}
					last = iter;
					++iter;
				}
				//SVG EXPORTER
				if (bDoSVG) {
					svgexpo->close_path();
					bSvgOnce = FALSE;
				}
			}			
			}
		}
        of << endl;

	// retract, end
	of << "G04 P0 ( dwell for no time -- G64 should not smooth over this point )\n";
	of << "G00 Z" << CONVERT_UNITS(mill->zchange) << " ( retract )\n" << endl;

	of << "M9 ( Coolant off. )\n";
	of << "M2 ( Program end. )\n\n";

	of.close();
	
	//SVG EXPORTER
	if (bDoSVG) {
		svgexpo->stroke();
	}
}

void NGC_Exporter::set_preamble(string _preamble)
{
	preamble=_preamble;
}

void NGC_Exporter::set_postamble(string _postamble)
{
	postamble=_postamble;
}

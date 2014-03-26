
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

#include <iostream>
using std::cout;
using std::endl;

#include <fstream>
#include <iomanip>
using namespace std;

#include "paste.hpp"

#include <cstring>
#include <boost/scoped_array.hpp>

#include <boost/foreach.hpp>

using std::pair;

PasteProcessor::PasteProcessor( string pastefile, const ivalue_t board_width ) : board_width(board_width)
{
	bDoSVG = false;
	project = gerbv_create_project();

	const char* cfilename = pastefile.c_str();
	boost::scoped_array<char> filename( new char[strlen(cfilename) + 1] );
	strcpy(filename.get(), cfilename);

	gerbv_open_layer_from_filename(project, filename.get());
	if( project->file[0] == NULL) throw paste_exception();

	preamble = string("G94     ( Inches per minute feed rate. )\n");
    #ifdef METRIC_OUTPUT
        preamble +=  "G21     ( Units == MM.             )\n";
    #else
        preamble += "G20     ( Units == INCHES.             )\n";
    #endif
    preamble += "G90     ( Absolute coordinates.        )\n";
	postamble = string("M9 ( Coolant off. )\n") +
		"M2 ( Program end. )\n\n";
}


void
PasteProcessor::set_svg_exporter(  boost::shared_ptr<SVG_Exporter> svgexpo )
{
	this->svgexpo = svgexpo;
	bDoSVG = true;
}


void
PasteProcessor::add_header( string header )
{
        this->header.push_back(header);
}

void
PasteProcessor::export_ngc( const string of_name,  boost::shared_ptr<Paster> paster, bool mirrored, bool mirror_absolute )
{
	cout << "GOT HERE OK" << endl;
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;
	ivalue_t double_mirror_axis = mirror_absolute ? 0 : board_width;
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;

	//SVG EXPORTER
	int rad = 1.;
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;
	
	// open output file
	std::ofstream of; of.open( of_name.c_str() );
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;

	 boost::shared_ptr<const map<int,pasteneedle> > needles = get_needles();
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;
	 boost::shared_ptr<const map<int,icoords> > blobs = get_blobs();	
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;

	// write header to .ngc file
        BOOST_FOREACH( string s, header )
        {
                of << "( " << s << " )" << endl;
        }
        of << endl;

	of << "( This file uses " << needles->size() << " paste needle sizes. )\n\n";

        of.setf( ios_base::fixed );
        of.precision(5);
	of << setw(7);

	// preamble
	of << preamble
	   << "S" << left << paster->speed << "  ( RPM spindle speed.           )\n"
	   << endl;

	for( map<int,pasteneedle>::const_iterator it = needles->begin(); it != needles->end(); it++ ) {
		of << "G00 Z" << CONVERT_UNITS(paster->zchange) << " ( Retract )\n"
		   << "T" << it->first << endl
		   << "M5      ( Spindle stop.                )\n"
		   << "M6      ( Tool change.                 )\n"
		   << "(MSG, CHANGE TOOL BIT: to drill size " << it->second.diameter << " " << it->second.unit << ")\n"
		   << "M0      ( Temporary machine stop.      )\n"
		   << "M3      ( Spindle on clockwise.        )\n"
		   << endl;

		const icoords paste_coords = blobs->at(it->first);
		icoords::const_iterator coord_iter = paste_coords.begin();

		
		//SVG EXPORTER
		if (bDoSVG) {
			//set a random color
			svgexpo->set_rand_color();
			//draw first circle
			svgexpo->circle( (double_mirror_axis - coord_iter->first), coord_iter->second, rad);		
			svgexpo->stroke();
		}
		
		
		while( coord_iter != paste_coords.end() ) {
            of << "G0 X" << CONVERT_UNITS((mirrored?double_mirror_axis - coord_iter->first:coord_iter->first)) << " Y" << CONVERT_UNITS(coord_iter->second) <<endl;
            of << "G1 Z" << CONVERT_UNITS(paster->zwork) << " F" << CONVERT_UNITS(paster->feed) << endl;
            of << "G0 Z" << CONVERT_UNITS(paster->zsafe) << endl;
			
			//SVG EXPORTER
			if (bDoSVG) {
				//make a whole
				svgexpo->circle( (double_mirror_axis - coord_iter->first), coord_iter->second, rad);
				svgexpo->stroke();
			}
			
			++coord_iter;
		}

		of << "\n\n";
	}

	// retract, end
	of << "G00 Z" << CONVERT_UNITS(paster->zchange) << " ( All done -- retract )\n" << endl;

	of << "M9 ( Coolant off. )\n";
	of << "M2 ( Program end. )\n\n";

	of.close();
}
/*
void ExcellonProcessor::millhole(std::ofstream &of,float x, float y,   boost::shared_ptr<Cutter> cutter,float holediameter)
{
	g_assert(cutter);
	double cutdiameter=cutter->tool_diameter;

	if(cutdiameter*1.001>=holediameter)
	{
		of<<"G0 X"<< x<<" Y" << y<< endl;
		//of<<"G1 Z"<<cutter->zwork<<endl;
		//of<<"G0 Z"<<cutter->zsafe<<endl<<endl;
		of<<"G1 Z#50"<<endl;
		of<<"G0 Z#51"<<endl<<endl;
	}
	else
	{
		float millr=(holediameter-cutdiameter)/2.;
		of<<"G0 X"<< x+millr<<" Y" << y<< endl;
		
		double z_step = cutter->stepsize;
		//z_step=0.01;
		double z = cutter->zwork + z_step * abs( int( cutter->zwork / z_step ) );
		if( !cutter->do_steps ) {
			z=cutter->zwork;
			z_step=1; //dummy to exit the loop
		}
		int stepcount=abs( int( cutter->zwork / z_step )) ;
		while( z >= cutter->zwork ) 
		{
			//of<<"G1 Z"<<z<<endl;
			of<<"G1 Z[#50+"<<stepcount<<"*#52]"<<endl;
			of<<"G2 I"<<-millr<<" J0"<<endl;
			z -= z_step;
			stepcount--;
		}
		of<<"G0 Z"<<cutter->zsafe<<endl<<endl;
	}
}*/

//mill larger holes by using a smaller mill-head
/*void 
ExcellonProcessor::export_ngc( const string outputname,   boost::shared_ptr<Cutter> target, bool mirrored, bool mirror_absolute )
{

	g_assert( mirrored == true );
	g_assert( mirror_absolute == false );
	cerr << "Currently Pasting "<< endl;

	// open output file
	std::ofstream of; of.open( outputname.c_str() );

	boost::shared_ptr<const map<int,pasteneedle> > needles = get_needles();
	boost::shared_ptr<const map<int,icoords> > blobs = get_blobs();	

	// write header to .ngc file
        BOOST_FOREACH( string s, header )
        {
                of << "( " << s << " )" << endl;
        }
        of << endl;

	//of << "( This file uses " << bits->size() << " drill bit sizes. )\n\n";
	of << "( This file uses a paste needle of "<<target->tool_diameter<<" to extrude the "<<needles->size() <<"needle sizes. )\n\n";

        of.setf( ios_base::fixed );
        of.precision(5);
	of << setw(7);

	// preamble
	of << preamble
	   << "S" << left << target->speed << "  ( RPM spindle speed.           )\n"
	   << endl;
	of<<"F"<<target->feed<<endl;
	
	of<<"#50="<<target->zwork<<" ; zwork"<<endl;
	of<<"#51="<<target->zsafe<<" ; zsafe"<<endl;
	of<<"#52="<<target->stepsize<<" ; stepsize"<<endl;
	
	
	for( map<int,pasteneedle>::const_iterator it = needles->begin(); it != needles->end(); it++ ) {
		
		float diameter=it->second.diameter;
		//cerr<<"bit:"<<diameter<<endl;
		const icoords paste_coords = blobs->at(it->first);
		icoords::const_iterator coord_iter = paste_coords.begin();
		
		//millhole(of,  board_width - coord_iter->first, coord_iter->second, target,diameter); //JJJ
		++coord_iter;

		while( coord_iter != paste_coords.end() ) {

		//JJJ	millhole(of,  board_width - coord_iter->first, coord_iter->second, target,diameter);
			++coord_iter;
		}
	
	}

	// retract, end
	of << "G00 Z" << target->zchange << " ( All done -- retract )\n" << endl;

	of << postamble;

	of.close();
}*/

void
PasteProcessor::parse_needles()
{
		cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;

	needles =  boost::shared_ptr< map<int,pasteneedle> >( new map<int,pasteneedle>() );
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;

	//for( gerbv_drill_list_t* currentPaste = project->file[0]->image->drill_stats->drill_list; currentPaste;

	//for( int* currentPaste = project->file[0]->image->paste_stats->paste_list; currentPaste;
	 //    currentPaste = currentPaste->next ) {
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;
/*		pasteneedle curNeedle; //JJJ
		curNeedle.diameter = currentPaste->drill_size;
		curNeedle.unit = string( currentPaste->drill_unit );
		curNeedle.paste_count = currentPaste->drill_count;

		needles->insert( pair<int,pasteneedle>(currentPaste->drill_num, curNeedle) );
	}*/
}

void
PasteProcessor::parse_blobs()
{
	if(!needles)
		parse_needles();

	blobs =  boost::shared_ptr< map<int,icoords> >( new map<int,icoords>() );

	for (gerbv_net_t* currentNet = project->file[0]->image->netlist; currentNet; currentNet = currentNet->next) {
		if(currentNet->aperture != 0)
			(*blobs)[currentNet->aperture].push_back( icoordpair(currentNet->start_x, currentNet->start_y) );
	}	
}

 boost::shared_ptr<const map<int,pasteneedle> >
PasteProcessor::get_needles()
{
		cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;

	if(!needles)
{	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;
		parse_needles();
	cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << endl;
}
	return needles;
}

 boost::shared_ptr<const map<int,icoords> >
PasteProcessor::get_blobs()
{
	if(!blobs)
		parse_blobs();

	return blobs;
}

PasteProcessor::~PasteProcessor()
{
	gerbv_destroy_project(project);
}

void PasteProcessor::set_preamble(string _preamble)
{
	preamble=_preamble;
}
void PasteProcessor::set_postamble(string _postamble)
{
	postamble=_postamble;
}

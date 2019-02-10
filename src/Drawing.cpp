
#include "loguru.h"

#include "VegaXmlPlotter.h"
#include "ChainLoader.h"
#include "XmlHistogram.h"
#include "Utils.h"

#include "TLatex.h"
#include "THStack.h"
#include "TKey.h"
#include "TList.h"
#include "TStyle.h"
#include "TColor.h"
#include "TTree.h"
#include "TString.h"
#include "TEllipse.h"

// #include "TBufferJSON.h"

#include <thread>


void VegaXmlPlotter::exec_TCanvas( string _path ){
	DSCOPE();
	int width = -1, height = -1;
	
	width = config.get<int>( _path + ":width", width );
	height = config.get<int>( _path + ":height", height );

	width = config.get<int>( _path + ":w", width );
	height = config.get<int>( _path + ":h", height );

	TCanvas * c = nullptr; 
	if ( width != -1 && height > -1 ){
		c = new TCanvas( "c", "c", width, height );
		LOG_F( INFO, "TCanvas( width=%d, height=%d )", width, height );
	}
	else {
		c = new TCanvas( "c" );
		LOG_F( INFO, "TCanvas()" );
	}

	c->SetFillColor(0); 
	c->SetFillStyle(4000);

	c->cd();
	c->Modified();
	c->Draw(); 
	c->Show();
	gPad = c;
	//return c;
} // exec_TCanvas

void VegaXmlPlotter::exec_Data( string _path ){
	DSCOPE();
	if ( config.exists( _path + ":name" ) && !config.exists( _path + ":treeName" ) ){
		loadDataFile( _path );
	} else if ( config.exists( _path + ":name" ) && config.exists( _path + ":treeName" )){
		loadChain( _path );
	}
} // exec_Data

void VegaXmlPlotter::exec_TFile( string _path ){
	DSCOPE();
	string url = config.getString( _path + ":url" );
	LOG_F( INFO, "Opening %s in RECREATE mode", url.c_str() );
	dataOut = new TFile( url.c_str(), "RECREATE" );
}

void VegaXmlPlotter::exec_Script( string _path ){
	DSCOPE();
	vector<string> scripts = config.getStringVector( _path );
	for ( string s : scripts ){
		LOG_F( INFO, "gROOT->ProcessLine( \".L %s\" )", s.c_str() );
		gROOT->ProcessLine( (".L " + s).c_str() );
	}
 	
}

void VegaXmlPlotter::exec_children( string _path ){
	DSCOPE();
	vector<string> paths = config.childrenOf( _path, 1 );
	for ( string p : paths ){
		exec_node( p );
	}
} //exec_children

void VegaXmlPlotter::exec_children( string _path, string tag_type ){
	DSCOPE();
	vector<string> paths = config.childrenOf( _path, 1 );
	DLOG( "exec children of %s, where tag = %s", _path.c_str(), tag_type.c_str() );
	for ( string p : paths ){
		string tag = config.tagName( p );

		if ( tag_type == tag )
			exec_node( p );
	}
} //exec_children

void VegaXmlPlotter::exec_Loop( string _path ){
	DSCOPE();
	vector<string> states;
	string var = "state";
	string v = "%s";



	if ( config.exists( _path + ":states" ) )
		states = config.getStringVector( _path +":states" );

	if ( config.exists( _path + ":var" ) )
		var = config.getString( _path + ":var" );

	// support arange
	if ( states.size() == 0 && config.exists( _path + ":arange" ) ){
		vector<float> pars = config.getFloatVector( _path + ":arange" );

		if ( pars.size() >= 2 ){
			float step = 1.0;
			if ( pars.size() > 2  )
				step = pars[2];
			LOG_SCOPE_F( INFO, "Loop arange (%f, %f, %f)", pars[0], pars[1], step );
			for ( float fv = pars[0]; fv < pars[1] - step/2.; fv+=step ){
				states.push_back( ts( fv ) );
			}
		}
	}

	// support linspace
	if ( states.size() == 0 && config.exists( _path + ":linspace" ) ){
		vector<float> pars = config.getFloatVector( _path + ":linspace" );
		LOG_SCOPE_F( INFO, "Loop linspace (%f, %f, %f)", pars[0], pars[1], pars[2] );
		if ( pars.size() > 2  ){
			for ( int i = 0; i < (int)pars[2]+1; i++ ){
				float fv = pars[0] + ((pars[1] - pars[0]) / ((int)pars[2])) * i;
				states.push_back( ts( fv ) );
			}
		}
	}

	// suport a range 
	if ( config.tagName( _path ) == "RangeLoop" ){

		if ( !config.exists( _path + ":vmin" ) ) { LOG_F( WARNING, "<RangeLoop vmin vmax min max width/>"  ); }
		if ( !config.exists( _path + ":vmax" ) ) { LOG_F( WARNING, "<RangeLoop vmin vmax min max width/>"  ); }
		if ( !config.exists( _path + ":min" ) ) { LOG_F( WARNING, "<RangeLoop vmin vmax min max width/>"  ); }
		if ( !config.exists( _path + ":max" ) ) { LOG_F( WARNING, "<RangeLoop vmin vmax min max width/>"  ); }



		string vmin = config.get<string>( _path + ":vmin" );
		string vmax = config.get<string>( _path + ":vmax" );
		string indexName = config.get<string>( _path + ":index", vmin+"_i" );

		HistoBins bx;
		bx.load( config, _path );
		if ( 0 == bx.nBins() ){
			LOG_F( WARNING, "Cannot make x bins" );
		}

		LOG_F( INFO, "<RangLoop vmin=%s vmax=%s", vmin.c_str(), vmax.c_str() );
		LOG_F( INFO, "%s", bx.toString().c_str() );
		LOG_F( INFO, "</RangLoop>" );

		for ( int i = 0; i < bx.bins.size()-1; i++ ){
			float va = bx.bins[i];
			float vb = bx.bins[i+1];
			// DLOG( "Executing range loop %s = %s", var.c_str(), state.c_str() );
			config.set( vmin, ts(va) );
			config.set( vmax, ts(vb) );

			DLOG( "Executing Range Loop [%s = %d] (%s=%f, %s=%f)", indexName.c_str(), i, vmin.c_str(), va, vmax.c_str(), vb );

			config.set( indexName, ts(i) );

			exec_children( _path );
		}
		return;
	}
	
	


	// Used for Transforms or organization
	if ( states.size() == 0 ){
		LOG_SCOPE_F( INFO, "Scope at %s", _path.c_str() );
		// LOG_F( INFO, "Executing Scope" );
		exec_children( _path );
	} else {
		LOG_SCOPE_F( INFO, "Loop at %s", _path.c_str() );
		int i = 0;
		string indexName = config.get<string>( _path + ":index", var + "_i" );
		for ( string state : states ){
			DLOG( "Executing loop %s[%s = %d] = %s", var.c_str(), indexName.c_str(), i, state.c_str() );
			string value = state;
			config.set( var, value );
			config.set( indexName, ts(i) );
			// vector<string> paths = config.childrenOf( _path, 1 );
			exec_children( _path );
			i++;
		} // loop on states
	}

	

} // exec_Loop

void VegaXmlPlotter::exec_Palette( string _path ) {
	gStyle->SetPalette( config.getInt( _path ) );
	if ( true == config.get<bool>( _path +":invert", false ) ){
		TColor::InvertPalette();
	}
}

void VegaXmlPlotter::exec_Plot( string _path ) {
	DSCOPE();
	DLOG( "Makign Plot[%s] @ %s", config.getString( _path+":name", "" ).c_str(), _path.c_str() );

	histos.clear();
	graphs.clear();
	funcs.clear();

	exec_node( _path + ".Axes" );

	// if ( config.exists( _path + ".Palette" ) ){
	// 	gStyle->SetPalette( config.getInt( _path + ".Palette" ) );
	// }

	vector<string> tlp = { "Margins", "StatBox", "Scope", "Loop", "Histo", "Graph", "TF1", "TLine", "TLatex", "Rect", "Ellipse", "Assign", "Format", "Palette" };
	vector<string> paths = config.childrenOf( _path, 1 );
	for ( string p : paths ){
		string tag = config.tagName( p );
		if ( std::find( tlp.begin(), tlp.end(), tag ) != tlp.end() )
			exec_node( p );
	}

	// Make LAtex on global scope for ease
	exec_children( "", "TLatex" );



	// makeLegend( _path, histos, graphs, funcs );
	
	exec_children( _path, "Legend" );
	exec_children( _path, "Export" );
} // exec_Plot

void VegaXmlPlotter::exec_Axes( string _path ){
	DSCOPE();
	
	if ( !config.exists( _path ) ) return;

	RooPlotLib rpl;
	HistoBins x( config, _path );
	HistoBins y( config, _path );

	// also check for linspaces on x, y
	LOG_F( 0, "Checking @%s:x :: %s ", _path.c_str(), config.oneOf( _path + ":x", _path + ":lsx" ).c_str() );
	x.linspace( config, config.oneOf( _path + ":x", _path + ":lsx" ) );
	x.arange( config, config.oneOf( _path + ":xrange", _path + ":xr" ) );

	LOG_F( 0, "Checking @%s:y :: %s ", _path.c_str(), config.oneOf( _path + ":y", _path + ":lsy" ).c_str() );
	y.linspace( config, config.oneOf( _path + ":y", _path + ":lsy" ) );
	y.arange( config, config.oneOf( _path + ":yrange", _path + ":yr" ) );

	// INFOC( "X : " << x.toString() << "\n\n\n" );
	// INFOC( "Y : " << y.toString() );

	if ( x.nBins() <= 0 ) {
		LOG_F( ERROR, "X bins invalid, bailing out" );
		// ERRORC( "Cannot make Axes, invalid x bins" );
		return;
	}

	if ( y.nBins() <= 0 ) {
		LOG_F( ERROR, "Y bins invalid, bailing out" );
		// ERRORC( "Cannot make Axes, invalid y bins" );
		return;
	}

	config.set( "x_min", dts(x.minimum()) );
	config.set( "x_max", dts(x.maximum()) );

	config.set( "y_min", dts(y.minimum()) );
	config.set( "y_max", dts(y.maximum()) );


	TH1 * frame = new TH1C( TString::Format( "frame_%s", random_string( 4 ).c_str()), "", x.nBins(), x.bins.data() );
	frame->SetDirectory( 0 );

	rpl.style( frame ).set( "yr", y.minimum(), y.maximum() );
	rpl.set( config, _path ).set( config, _path + ":style" ).set( config, _path + ".style" );

	frame->SetMarkerStyle(8);
	frame->SetMarkerSize(0);
	frame->DrawClone("p");

	current_frame = (TH1C*)frame;
} // exec_Axes

void VegaXmlPlotter::exec_Export( string _path ){
	DSCOPE();
	TPad * _pad = (TPad*)gPad;
	
	if ( !config.exists( _path + ":url" ) ) return;

	string url = config.getXString( _path + ":url" );
	if ( url.find( ".json" ) != string::npos ){
		// TBufferJSON::ExportToFile( url.c_str(), _pad );
		// LOG_F( INFO, "%s",  );
		// ofstream fout( url.c_str() );
		// fout << TBufferJSON::ConvertToJSON( _pad ).Data();
		// fout.close();
	} else {
		_pad->Print( url.c_str() );
	}
} // exec_Export


void VegaXmlPlotter::exec_StatBox( string _path ){
	DSCOPE();
	int value = config.get<int>( _path + ":v", config.get<int>( _path + ":value", 111 ) );
	int fit   = config.get<int>( _path + ":f", config.get<int>( _path + ":fit", 111 ) );
	float w = config.get<float>( _path + ":w", 0.3 );
	float h = config.get<float>( _path + ":h", 0.3 );
	vector<float> pos = config.getFloatVector( _path + ":pos" );
	if ( pos.size() < 2 ){
		// it will def be length 2
		pos.push_back( -1 ); pos.push_back( -1 );
	}

	gStyle->SetOptStat( value );
	if ( config.exists( _path + ":f" ) || config.exists( _path + ":fit" ) ){
		gStyle->SetOptFit( fit );
		LOG_F(  INFO, "OptFit=%d", fit );
	}
	if ( pos[0] > -1 )
		gStyle->SetStatX( pos[0] );
	if ( pos[1] > -1 )
		gStyle->SetStatY( pos[1] );

	gStyle->SetStatW( w );
	gStyle->SetStatH( h );

	LOG_F( INFO, "StatBox( %d ) @ (x=%f, y=%f) with (w=%f, h=%f)", value, pos[0], pos[1], w, h );

}

void VegaXmlPlotter::exec_Histo( string _path ){
	DSCOPE();
	RooPlotLib rpl;

	string name = config.getXString( _path + ":name" );
	string data = config.getXString( _path + ":data" );
	string fqn = fullyQualifiedName( data, name );

	TH1* h = findHistogram( _path, 0 );
	
	if ( nullptr == h ) {
		LOG_F( WARNING, "Could not find Histogram @ %s (fqn=\"%s\")", _path.c_str(), fqn.c_str() );
		return;
	}

	config.set( "ClassName", h->ClassName() );
	DLOG( "ClassName %s", config[ "ClassName" ].c_str() );

	
	LOG_F( INFO, "Found Histogram @%s = %p (fqn=%s)", _path.c_str(), h, fqn.c_str() );

	float nI = h->Integral();
	string normp = config.oneOf( _path + ".Norm", _path + ":norm" );
	if ( "" != normp && nI > 0 && config.getBool( normp, true ) == true){
		float normC = config.get<float>( normp, 0 );
		if ( normC <= 0 )
			normC = 1.0;
		LOG_F( INFO, "Normalize: %s to %f / %f", h->GetName(), normC, nI );
		h->Scale( normC / nI );
	}

	string styleRef = config.getXString( _path + ":style" );
	
	if ( config.exists( styleRef ) ){
		rpl.style( h ).set( config, styleRef );
	}

	rpl.style( h ).set( config, _path ).set( config, _path + ".style" ).draw();

	if ( config.exists( _path +":after_draw" ) ){
		string cmd = ".x " + config[_path+":after_draw"] + "( " + h->GetName() + " )";
		LOG_F( INFO, "Executing: %s", cmd.c_str()  );
		gROOT->ProcessLine( cmd.c_str() );
	}

	TPaveStats *st = (TPaveStats*)h->GetListOfFunctions()->FindObject("stats");
	// if ( nullptr != st  ){
	// 	LOG_F( INFO, "Exec positionOptStats( %s, %p )", _path.c_str(), st );
	// 	positionOptStats( _path, st );
	// 	// st->SetX1NDC( 0.7 ); st->SetX2NDC( 0.975 );
	// } else {
	// 	LOG_F( INFO, "OptStat is NULL" );
	// }

	LOG_F( INFO, "Indexing histo[%s] and histo[%s]", quote(nameOnly(fqn)).c_str(), quote(fqn).c_str() );
	histos[ nameOnly(fqn) ] = h;
	histos[ fqn ] 			= h;

	// return h;
} // exec_Histo

void VegaXmlPlotter::exec_Graph( string _path ){
	DSCOPE();
	RooPlotLib rpl;

	TObject * obj = findObject( _path );
	LOG_F( INFO, "Object = %p", obj );
	TGraph * g = dynamic_cast<TGraph*>( obj );

	if ( nullptr == g ) {
		LOG_F( ERROR, "Cannot get Graph @ %s", _path.c_str() );
		return;
	}
	LOG_F( INFO, "Found Graph at %s", _path.c_str() );

	// set meta info
	config.set( "ClassName", g->ClassName() );

	string name = config.getXString( _path + ":name" );
	string data = config.getXString( _path + ":data" );
	
	string fqn = fullyQualifiedName( data, name );


	rpl.style( g ).set( config, _path ).set( config, _path + ":style" ).set( config, _path + ".style" ).draw();
	graphs[ name ] = g;
	graphs[ fqn ] = g;

} // exec_Graph

void VegaXmlPlotter::exec_TF1( string _path ){
	DSCOPE();
	RooPlotLib rpl;

	XmlFunction xf;
	xf.set( config, _path );

	TF1 * f = xf.getTF1().get();
	if ( nullptr == f ) {
		LOG_F( ERROR, "Cannot make TF1 @ %s", _path.c_str() );
		return;
	}
	f = (TF1*)f->Clone( (f->GetTitle() + string("_clone")).c_str() );

	
	LOG_F( INFO, "%s", f->GetTitle() );
	LOG_F( INFO, "Eval f(0)=%f", f->Eval( 1.0 ) );

	// set meta info
	config.set( "ClassName", xf.getTF1()->ClassName() );

	string name = config.getXString( _path + ":name" );
	string data = config.getXString( _path + ":data" );
	
	string fqn = fullyQualifiedName( data, name );

	rpl.style( f ).set( config, _path ).set( config, _path + ":style" ).set( config, _path + ".style" ).draw( "same" );

	funcs[ nameOnly(fqn) ]  = f;
	funcs[ fqn ] 			= f;
} // exec_TF1

void VegaXmlPlotter::exec_TLine( string _path){
	DSCOPE();

	vector<double> x(2);
	vector<double> y(2);

	if ( config.exists( _path + ":x" ) ){
		x = config.getDoubleVector( _path + ":x" );
	}
	if ( config.exists( _path + ":y" ) ){
		y = config.getDoubleVector( _path + ":y" );
	}
	if ( config.exists( _path + ":p1" ) ){
		vector<double> p = config.getDoubleVector( _path + ":p1" );
		if ( p.size() >= 2 ){
			x[0] = p[0];
			y[0] = p[1];
		}
	}
	if ( config.exists( _path + ":p2" ) ){
		vector<double> p = config.getDoubleVector( _path + ":p2" );
		if ( p.size() >= 2 ){
			x[1] = p[0];
			y[1] = p[1];
		}
	}

	x[0] = config.getDouble( _path + ":x1", x[0] );
	x[1] = config.getDouble( _path + ":x2", x[1] );
	y[0] = config.getDouble( _path + ":y1", y[0] );
	y[1] = config.getDouble( _path + ":y2", y[1] );

	LOG_F( INFO, "Line (%0.2f, %0.2f)->(%0.2f, %0.2f)", x[0], y[0], x[1], y[1] );
	TLine * line = new TLine( 
		x[0], y[0],
		x[1], y[1] );

	line->SetLineColor( color( config.getString( _path + ":color" ) ) );
	line->SetLineWidth( config.getInt( _path + ":width", 1 ) );
	line->SetLineStyle( config.getInt( _path + ":style", 2 ) );

	line->Draw("same");
} // exec_TLine

void VegaXmlPlotter::exec_Rect( string _path ){
	DSCOPE();
	vector<float> c;
	c = config.getFloatVector( _path + ":pos" );
	LOG_F( INFO, "Rect( %0.2f, %0.2f, %0.2f, %0.2f )", c[0], c[1], c[2], c[3] );
	TBox * rect = new TBox( c[0], c[1], c[2], c[3] );
	RooPlotLib rpl;
	rpl.style( rect ).set( config, _path );
	rect->Draw();
} // exec_Rect

void VegaXmlPlotter::exec_Ellipse( string _path ){
	DSCOPE();
	vector<float> c;
	c = config.getFloatVector( _path + ":pos" );
	double x = c[0];
	double y = c[1];
	c = config.getFloatVector( _path + ":r" );
	double rx = c[0];
	double ry = c[1];

	LOG_F( INFO, "Ellipse( %0.2f, %0.2f, %0.2f, %0.2f )", x, y, rx, ry );
	TEllipse * rect = new TEllipse( x, y, rx, ry );
	RooPlotLib rpl;
	rpl.style( rect ).set( config, _path );
	rect->Draw();
} // exec_Rect

void VegaXmlPlotter::exec_TLatex( string _path ){
	DSCOPE();

	TLatex latex;
	if ( !config.exists( _path + ":text" ) ) return;

	string text = config.getXString( _path + ":text" );
	
	latex.SetTextSize( config.getFloat( _path + ":size", 0.05 ) );
	if ( config.exists( _path + ":point" ) )
		latex.SetTextSize( config.getFloat( _path + ":point", 18 ) / 360.0 );
	


	if ( config.exists( _path +":color" ) ){
		string cs = config.getXString( _path + ":color" );
		latex.SetTextColor( color( cs ) );
	} else {
		latex.SetTextColor( kBlack );
	}
	
	TLatex * TL = nullptr;
	if ( config.exists( _path + ":x" ) && config.exists( _path + ":y" ) ){
		TL = latex.DrawLatexNDC( config.getFloat( _path + ":x" ), 
						config.getFloat( _path + ":y" ), 
						config.getXString( _path + ":text" ).c_str() );
	} else if ( config.exists( _path + ":ux" ) && config.exists( _path + ":uy" ) ){
		TL = latex.DrawLatex( config.getFloat( _path + ":ux" ), 
						config.getFloat( _path + ":uy" ), 
						config.getXString( _path + ":text" ).c_str() );
	}
	if ( nullptr == TL ){
		LOG_F(ERROR, "Cannot draw latex, missing attribute. Must have pad corrds (:x,:y) or user corrds( :ux, :uy)" ); 
		return;
	}		 

	if ( config.exists( _path + ":font" ) ){
		TL->SetTextFont( config.getInt( _path + ":font" ) );
	}
	if ( config.exists( _path + ":angle" ) ){
		TL->SetTextAngle( config.getFloat( _path + ":angle" ) );
	}

	if ( config.exists( _path + ":align" ) ){
		string strAlign = config.getString( _path + ":align" );
		if ( "top" == strAlign || "t" == strAlign ){
			TL->SetTextAlign( 13 );
		} else if ( "center" == strAlign || "centered" == strAlign || "c" == strAlign ){
			TL->SetTextAlign( 12 );
		} else if ( "bottom" == strAlign || "b" == strAlign ){
			TL->SetTextAlign( 11 );
		} else if ( "special" == strAlign || "s" == strAlign ){
			TL->SetTextAlign( 10 );
		} else {
			TL->SetTextAlign( config.getInt( _path + ":align" ) );
		}
	}
} // exec_TLatex

void VegaXmlPlotter::exec_TLegend( string _path ){
	DSCOPE();
	
	RooPlotLib rpl;
	float x1, y1, x2, y2;
	
	x1 = config.getDouble( _path + ".Position:x1", 0.1 );
	y1 = config.getDouble( _path + ".Position:y1", 0.7 );
	x2 = config.getDouble( _path + ".Position:x2", 0.5 );
	y2 = config.getDouble( _path + ".Position:y2", 0.9 );

	string spos = config.getXString( _path + ".Position:pos" );

	float w = config.getFloat( _path + ".Position:w", 0.4 );
	float h = config.getFloat( _path + ".Position:h", 0.2 );
	vector<float> padding = config.getFloatVector( _path + ".Position:padding" );
	if ( padding.size() < 4 ){
		padding.clear();
		padding.push_back( 0 ); padding.push_back( 0 );
		padding.push_back( 0 ); padding.push_back( 0 );
	}

	DLOG( "Legend padding : %s", vts( padding ).c_str() );


	if ( spos.find("top") != string::npos ){
		DLOG( "LEGEND: TOP" );
		y2 = 0.9 - padding[0];
		if ( nullptr != gPad) y2 = 1.0 - gPad->GetTopMargin() - padding[0];
		y1 = y2 - h;
	}
	if ( spos.find("right") != string::npos ){
		DLOG( "LEGEND: RIGHT" );
		x2 = 0.9 - padding[1];
		if ( nullptr != gPad) x2 = 1.0 - gPad->GetRightMargin() - padding[1];
		x1 = x2 - w;
	}
	if ( spos.find("bottom") != string::npos ){
		DLOG( "LEGEND: BOTTOM" );
		y1 = 0.1 + padding[2];
		if ( nullptr != gPad) y1 = gPad->GetBottomMargin() + padding[2];
		y2 = y1 + h;
	}
	if ( spos.find("left") != string::npos ){
		DLOG( "LEGEND: LEFT" );
		x1 = 0.1 + padding[3];
		if ( nullptr != gPad) x1 = gPad->GetLeftMargin() + padding[3];
		x2 = x1 + w;
	}
	if ( spos.find("vcenter") != string::npos ){
		DLOG( "LEGEND: VERTICAL CENTER" );
		y1 = 0.5 - h/2 + padding[2];
		y2 = 0.5 + h/2 + padding[2] - padding[0];
	}
	if ( spos.find("hcenter") != string::npos ){
		DLOG( "LEGEND: HORIZONTAL CENTER" );
		x1 = 0.5 - w/2 + padding[3];
		x2 = 0.5 + w/2 + padding[3] - padding[1];
	}
	DLOG( "Legend position: (%f, %f) -> (%f, %f)", x1, y1, x2, y2 );
	TLegend * leg = new TLegend( x1, y1, x2, y2 );

	if ( config.exists(  _path + ":title" ) ) 
		leg->SetHeader( config.getXString( _path + ":title" ).c_str() );
	leg->SetNColumns( config.getInt( _path + ":columns", 1 ) );

	vector<string> entries = config.childrenOf( _path + "", "Entry" );

	// HISTOGRAMS
	for ( string entryPath : entries ){
		//INFO( classname(), "Entry @" << entryPath );
		if ( config.exists( entryPath + ":name" ) != true ) continue;
		string name = config.getXString( entryPath + ":name" );
		
		if ( histos.count( name ) <= 0 || histos[ name ] == nullptr ) {
			LOG_F( INFO, "Could not add Legend entry for name=%s", quote( name ).c_str() );
			continue;
		}
		TH1 * h = (TH1*)histos[ name ]->Clone( ("hist_legend_" + name).c_str() );
		
		string t = config.getXString(  entryPath + ":title", name );
		string opt = config.getXString(  entryPath + ":opt", "l" );
		rpl.style( h ).set( config, entryPath );
		


		leg->AddEntry( h, t.c_str(), opt.c_str() );
	}

	// GRAPHS
	for ( string entryPath : entries ){
		if ( config.exists( entryPath + ":name" ) != true ) continue;
		string name = config.getXString( entryPath + ":name" );
		if ( graphs.count( name ) <= 0 || graphs[ name ] == nullptr )
			continue;
		TGraph * g = (TGraph*)graphs[ name ]->Clone( ("graph_legend_" + name).c_str() );

		string t = config.getXString(  entryPath + ":title", name );
		string opt = config.getXString(  entryPath + ":opt", "l" );
		rpl.style( g ).set( config, entryPath );
		leg->AddEntry( g, t.c_str(), opt.c_str() );
	}

	// FUNCTIONS
	for ( string entryPath : entries ){
		//INFO( classname(), "Entry @" << entryPath );
		if ( config.exists( entryPath + ":name" ) != true ) continue;
		string name = config.getXString( entryPath + ":name" );
		
		if ( funcs.count( name ) <= 0 || funcs[ name ] == nullptr ) {
			continue;
		}
		TF1 * f = (TF1*)funcs[ name ]->Clone( ("func_legend_" + name).c_str() );

		string t   = config.getXString(  entryPath + ":title", name );
		string opt = config.getXString(  entryPath + ":opt", "l" );

		rpl.style( f ).set( config, entryPath );
		leg->AddEntry( f, t.c_str(), opt.c_str() );
	}

	if ( entries.size() <= 0 ){
		// Add an entry for all histograms by default if no <Entry> nodes found
		for ( auto kv : histos ){
			string t = config.getString(  _path + "." + kv.first+ ":title", kv.first) ;
			string opt = config.getString(  _path + "." + kv.first+ ":opt", "l") ;
			leg->AddEntry( kv.second, t.c_str(), opt.c_str() );
		}
	}

	vector< string > attr = config.attributesOf( _path );
	for ( unsigned int i = 0; i < attr.size(); i++ ){
		// attr is full paths to attribute
		string nattr = RooPlotLib::normalizeAttribute( config.attributeName(attr[i]) );
		if ( "textsize" == nattr )
			leg->SetTextSize( config.getDouble( attr[i] ) );
		if ( "textpoint" == nattr )
			leg->SetTextSize( config.getDouble( attr[i] ) / 360.0 );
		if ( "bordersize" == nattr )
			leg->SetBorderSize( config.getDouble( attr[i] ) );
		if ( "fillcolor" == nattr )
			leg->SetFillColor( color( config.getString( attr[i] ) ) );
		if ( "fillstyle" == nattr )
			leg->SetFillStyle( config.getInt( attr[i] ) );
	}

	leg->Draw( );
} // exec_TLegend

void VegaXmlPlotter::exec_Canvas( string _path ){
	DSCOPE();

	xcanvas = new XmlCanvas( config, _path );

	exec_children( _path, "Loop" );
	exec_children( _path, "Pad" );
	
	xcanvas->getCanvas()->Modified();
	xcanvas->getCanvas()->Update();

	gPad = (TPad*) xcanvas->getCanvas();
	gPad->SetFillColor(0); 
	gPad->SetFillStyle(0);



	exec_children( _path, "Export" );
} // exec_Canvas

void VegaXmlPlotter::exec_Pad( string _path ){
	DSCOPE();
	if ( nullptr == xcanvas ){
		LOG_F( ERROR, "xcanvas is NULL, cannot make Pad @ %s", _path.c_str() );
		return;
	}
	string n = config.getString( _path + ":name" );
	if ( "" == n ){
		LOG_F( WARNING, "Pad must have valid name, skipping pad @ %s", _path.c_str() );
		return;
	}

	XmlPad * xpad = xcanvas->activatePad( n );

	TPad * p = xpad->getRootPad();
	p->SetFillColor(0);
	p->SetFillStyle(0);
	p->SetFrameFillColor(0);
	p->SetFrameFillStyle(0);

	// move pad to origin
	xpad->moveToOrigin();
	xpad->getRootPad()->Update();
	
	exec_Plot( _path );

	xpad->reposition();
	xpad->getRootPad()->Update();
}

void VegaXmlPlotter::exec_Margins( string _path ){
	DSCOPE();
	
	float tm=-1, rm=-1, bm=-1, lm=-1;

	vector<float> parts = config.getFloatVector( _path );
	if ( parts.size() >= 4 ){
		tm=parts[0];rm=parts[1];bm=parts[2];lm=parts[3];
	}

	tm = config.getFloat( _path + ":top"    , tm );
	rm = config.getFloat( _path + ":right"  , rm );
	bm = config.getFloat( _path + ":bottom" , bm );
	lm = config.getFloat( _path + ":left"   , lm );

	LOG_F( INFO, "Margins( %f, %f, %f, %f )", tm, rm, bm, lm );

	if ( nullptr == gPad ){
		LOG_F( WARNING, "Null gPad, cannot set margins. Add a <TCanvas> before the <Margin>" );
		return;
	}
	if ( tm>=0 ) gPad->SetTopMargin( tm );
	if ( rm>=0 ) gPad->SetRightMargin( rm );
	if ( bm>=0 ) gPad->SetBottomMargin( bm );
	if ( lm>=0 ) gPad->SetLeftMargin( lm );
} // exec_Margins

void VegaXmlPlotter::exec_ExportConfig( string _path ){
	DSCOPE();

	string fname = config[_path+":url"];
	LOG_F( INFO, "Exporting config to %s", fname.c_str() );
	config.toXmlFile( fname );
	if ( config.get<bool>( _path + ":yaml", false ) ){
		string yaml_url=fname.substr(0,fname.find_last_of('.')) + ".yaml";
		LOG_F( INFO, "Dumping config to %s", yaml_url.c_str() );
		config.dumpToFile( yaml_url );
	}
	config.deleteNode( _path );
} // ExportConfig
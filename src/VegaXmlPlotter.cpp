#include "VegaXmlPlotter.h"
#include "ChainLoader.h"
#include "XmlCanvas.h"

#include "TLatex.h"
#include "THStack.h"
#include "TKey.h"
#include "TList.h"
#include "TStyle.h"
#include "TColor.h"

#define LOGURU_IMPLEMENTATION 1
#include "loguru.h"

// #define VEGADEBUG 1


#if VEGADEBUG > 0
	#define DLOG( ... ) LOG_F(INFO, __VA_ARGS__)
	#define DSCOPE() LOG_SCOPE_FUNCTION(INFO)
#else
	#define DLOG( ... ) do {} while(0)
	#define DSCOPE() do {} while(0)
#endif

void VegaXmlPlotter::init(){
	DSCOPE();
	string logfile = config[ "Logger:url" ];
	if ( config.exists( "Logger:url" ) ){
		loguru::add_file(logfile.c_str(), loguru::Truncate, loguru::Verbosity_MAX);
	}

	setDefaultPalette();
}

void VegaXmlPlotter::make() {
	DSCOPE();

	TCanvas *c = new TCanvas( "rp" );
	c->Print( "rp.pdf[" );

	loadData();
	makeOutputFile();

	// string ll = config.getString( "Logger:globalLogLevel", "debug" );
	//INFOC( "Setting ll to " << quote(ll) );
	// Logger::setGlobalLogLevel( "none" );
	makeTransforms();
	makePlots();
	makePlotTemplates();
	makeCanvases();

	if ( dataOut ){
		dataOut->Write();
		dataOut->Close();
	}

	c->Print( "rp.pdf]" );

	config.toXmlFile( "compiled_config.xml" );
	config.dumpToFile( "config_dump.txt" );
}

void VegaXmlPlotter::loadDataFile( string _path ){
	DSCOPE();
	if ( config.exists( _path + ":name" ) ){
		string name = config.getXString( _path+":name" );
		string url = config.getXString( _path+":url" );
		LOG_S( INFO ) <<  "Data name=" << name << " @ " << url ;
		TFile * f = new TFile( url.c_str() );
		dataFiles[ name ] = f;
	}
} // loadDataFiles

int VegaXmlPlotter::numberOfData() {
	DSCOPE();
	return dataFiles.size() + dataChains.size();
}


void VegaXmlPlotter::loadChain( string _path ){
	DSCOPE();
	string name     = config.getXString( _path + ":name" );
	string treeName = config.getXString( _path + ":treeName" );
	string url      = config.getXString( _path + ":url" );
	int maxFiles    = config.getInt( _path + ":maxFiles", -1 );

	dataChains[ name ] = new TChain( treeName.c_str() );
	
	if ( url.find( ".lis" ) != std::string::npos )
		ChainLoader::loadList( dataChains[ name ], url, maxFiles );
	else 
		ChainLoader::load( dataChains[ name ], url, maxFiles );
	LOG_S(INFO) << "Loaded TTree [name=" << quote(name) << "] from url: " << url;
	int nFiles = 0;
	if ( dataChains[name]->GetListOfFiles() )
		nFiles = dataChains[name]->GetListOfFiles()->GetEntries();
	LOG_S(INFO) << "Chain has " << nFiles << plural( nFiles, " file", " files" );
}

void VegaXmlPlotter::loadData(){
	DSCOPE();
	vector<string> data_nodes = config.childrenOf( nodePath, "Data" );
	
	LOG_F( INFO, "Found %lu data nodes", data_nodes.size() );
	for ( string path : data_nodes ){
		if ( config.exists( path + ":name" ) && !config.exists( path + ":treeName" ) ){
			loadDataFile( path );
		} else if ( config.exists( path + ":name" ) && config.exists( path + ":treeName" )){
			loadChain( path );
		}
	}

	if ( data_nodes.size() <= 0 ){
		LOG_F(WARNING, "No data nodes found!" );
	}

} // loadData


void VegaXmlPlotter::makeOutputFile(){
	DSCOPE();
	if ( config.exists( "TFile:url" ) ){
		dataOut = new TFile( config.getXString( "TFile:url" ).c_str(), "RECREATE" );
	} else {
		//INFO( classname(), "No output data file requested" );
	}
}

void VegaXmlPlotter::makePlotTemplates(){
	DSCOPE();
	vector<string> plott_paths = config.childrenOf( nodePath, "PlotTemplate" );
	DLOG_F( INFO, "Found %lu PlotTemplate", plott_paths.size() );
	for ( string path : plott_paths ){
		vector<string> names = config.getStringVector( path + ":names" );
		if ( names.size() <= 1 ){
			names = glob( config.getString( path+":names" ) );
		}

		int iPlot = 0;
		for ( string name : names ){

			// set the global vars for this template
			config.set( "name", name );
			config.set( "uname", underscape(name) );
			config.set( "iPlot", ts(iPlot) );

			makeMargins( path );
			makePlot( path );
			iPlot++;
		}
	}
}

void VegaXmlPlotter::makePlot( string _path, TPad * _pad ){
	DSCOPE();
	DLOG_F( INFO, "Makign Plot[%s] @ %s", config.getString( _path+":name", "" ).c_str(), _path.c_str() );

	if ( config.exists( _path + ".Palette" ) ){
		gStyle->SetPalette( config.getInt( _path + ".Palette" ) );
	}

	makeAxes( _path + ".Axes" );
	map<string, TH1*> histos = makeHistograms( _path );
	// makeHistoStack( histos );
	makeLatex(""); // global first
	makeLatex(_path);
	makeLine( _path );
	makeLegend( _path, histos );

	makeExports( _path, _pad );
	// INFOC( "Finished making Plot" );
}

void VegaXmlPlotter::makePlots(){
	DSCOPE();
	vector<string> plot_paths = config.childrenOf( nodePath, "Plot" );
	
	DLOG_S(INFO) << "Found " << plot_paths.size() << plural( plot_paths.size(), " Plot", " Plots" );

	for ( string path : plot_paths ){
		TCanvas * c = makeCanvas( path ); 
		c->cd();
		makeMargins( path );
		makePlot( path );
	}
}

void VegaXmlPlotter::makeCanvases(){
	DSCOPE();
	vector<string> c_paths = config.childrenOf( nodePath, "Canvas" );
	//INFO( classname(), "Found " << c_paths.size() << plural( c_paths.size(), " Canvas", " Canvases" ) );
	for ( string path : c_paths ){
		drawCanvas( path );
	}
}

// void VegaXmlPlotter::makeHistoStack( map<string, TH1*> histos ){
// DSCOPE();
// 	THStack *hs = new THStack( "hs", "" );
// 	for ( auto kv : histos ){
// 		hs->Add( kv.second );
// 	}
// 	RooPlotLib rpl;
// 	rpl.style( hs )
// }

void VegaXmlPlotter::makeMargins( string _path ){
	DSCOPE();
	
	float tm=-1, rm=-1, bm=-1, lm=-1;

	vector<float> parts = config.getFloatVector( "Margins" );
	if ( parts.size() >= 4 ){
		tm=parts[0];rm=parts[1];bm=parts[2];lm=parts[3];
	}

	parts = config.getFloatVector( _path + ".Margins" );
	if ( parts.size() >= 4 ){
		tm=parts[0];rm=parts[1];bm=parts[2];lm=parts[3];
	}

	// Global first
	
	tm = config.getFloat( "Margins:top"    , tm );
	rm = config.getFloat( "Margins:right"  , rm );
	bm = config.getFloat( "Margins:bottom" , bm );
	lm = config.getFloat( "Margins:left"   , lm );

	tm = config.getFloat( _path + ".Margins:top"    , tm );
	rm = config.getFloat( _path + ".Margins:right"  , rm );
	bm = config.getFloat( _path + ".Margins:bottom" , bm );
	lm = config.getFloat( _path + ".Margins:left"   , lm );

	LOG_F( INFO, "Margins( %f, %f, %f, %f )", tm, rm, bm, lm );
	if ( tm>=0 ) gPad->SetTopMargin( tm );
	if ( rm>=0 ) gPad->SetRightMargin( rm );
	if ( bm>=0 ) gPad->SetBottomMargin( bm );
	if ( lm>=0 ) gPad->SetLeftMargin( lm );
}


TH1 *VegaXmlPlotter::findHistogram( string _data, string _name ){
	DSCOPE();

	// TODO: add support for splitting name inot data and name by "/"
	
	if ( "" == _data && _name.find( "/" ) != string::npos ){
		_data = dataOnly( _name );
		_name = nameOnly( _name );
	}

	string tdata = _data;
	if ( "" == _data && dataFiles.size() >= 1 ) tdata = dataFiles.begin()->first;
	if ( dataFiles.count( tdata ) > 0 && dataFiles[ tdata ] ){
		if ( nullptr == dataFiles[ tdata ]->Get( _name.c_str() ) ) return nullptr;
		TH1 * h = (TH1*)dataFiles[ tdata ]->Get( _name.c_str() );
		//INFO( classname(), "Found Histogram [" << _name << "]= " << h << " in data file " << tdata );
		return h;
	}

	// finally look for histos we made and named in the ttree drawing
	if ( globalHistos.count( _name ) > 0 && globalHistos[ _name ] ){
		//INFOC( "Found histogram in mem pool" );
		return globalHistos[ _name ];
	}

	return nullptr;
}

TH1* VegaXmlPlotter::findHistogram( string _path, int iHist, string _mod ){
	DSCOPE();

	string data = config.getXString( _path + ":data" + _mod, config.getXString( _path + ":data" ) );
	string name = config.getXString( _path + ":name" + _mod, config.getXString( _path + ":name" + _mod ) );

	if ( "" == data && name.find( "/" ) != string::npos ){
		data = dataOnly( name );
		//INFOC( "data from name " << quote( data ) );
		name = nameOnly( name );
	}

	//INFO( classname(), "Looking for [" << name << "] in " << data << "=(" << dataFiles[data] <<")" );
	// first check for a normal histogram from a root file
	if ( dataFiles.count( data ) > 0 && dataFiles[ data ] ){
		if ( nullptr == dataFiles[ data ]->Get( name.c_str() ) ) return nullptr;
		TH1 * h = (TH1*)dataFiles[ data ]->Get( name.c_str() );
		h = (TH1*)h->Clone( (string("hist_") + h->GetName() ).c_str() );
		//INFO( classname(), "Found Histogram [" << name << "]= " << h << " in data file " << data );
		return h;
	}

	// look for a dataChain with the name
	if ( dataChains.count( data ) > 0 && dataChains[ data ] ){
		TH1 * h = makeHistoFromDataTree( _path, iHist );
		return h;
	}

	// finally look for histos we made and named in the ttree drawing
	if ( globalHistos.count( name ) > 0 && globalHistos[ name ] ){
		//INFOC( "Found histogram in mem pool" );
		return globalHistos[ name ];
	}


	return nullptr;
}


TH1* VegaXmlPlotter::makeAxes( string _path ){
	DSCOPE();
	
	if ( !config.exists( _path ) ) return nullptr;

	RooPlotLib rpl;
	HistoBins x( config, _path );
	HistoBins y( config, _path );

	// also check for linspaces on x, y
	//INFOC( "Checking @" << _path + ":x" << " :: " << config.oneOf( _path + ":x", _path + ":lsx" ) );
	x.linspace( config, config.oneOf( _path + ":x", _path + ":lsx" ) );
	x.arange( config, config.oneOf( _path + ":xrange", _path + ":xr" ) );

	LOG_F( 0, "Checking @%s:y :: %s ", _path.c_str(), config.oneOf( _path + ":y", _path + ":lsy" ).c_str() );
	y.linspace( config, config.oneOf( _path + ":y", _path + ":lsy" ) );
	y.arange( config, config.oneOf( _path + ":yrange", _path + ":yr" ) );

	// INFOC( "X : " << x.toString() << "\n\n\n" );
	// INFOC( "Y : " << y.toString() );

	if ( x.nBins() <= 0 ) {
		// ERRORC( "Cannot make Axes, invalid x bins" );
		return nullptr;
	}

	if ( y.nBins() <= 0 ) {
		// ERRORC( "Cannot make Axes, invalid y bins" );
		return nullptr;
	}

	TH1 * frame = new TH1C( "frame", "", x.nBins(), x.bins.data() );

	rpl.style( frame ).set( "yr", y.minimum(), y.maximum() );
	rpl.set( config, _path ).set( config, _path + ":style" ).set( config, _path + ".style" );

	frame->Draw();

	return frame;
}


map<string, TH1*> VegaXmlPlotter::makeHistograms( string _path ){
	DSCOPE();
	map<string, TH1*> histos;
	vector<string> hist_paths = config.childrenOf( _path, "Histo" );

	for ( string hpath : hist_paths ){

		if ( config.exists( hpath + ":names" ) ){
			//INFOC( "HISTOGRAM TEMPLATE at " << hpath );

			vector<string> names = config.getStringVector( hpath + ":names" );
			if ( names.size() <= 1 ){
				names = glob( config.getString( hpath + ":names" ) );
			}

			int i = 0;
			for ( string name : names ){

				// set the global vars for this template
				config.set( hpath + ":name", name );
				config.set( "i", ts(i) );

				//INFOC( "Histogram from template name = " << config.getXString( hpath + ":name" ) );

				string fqn = "";
				TH1 * h = makeHistogram( hpath, fqn );
				histos[ nameOnly(fqn) ] = h;
				histos[ fqn ] = h;
				i++;
			}
			continue;
		}

		string fqn = "";
		TH1 * h = makeHistogram( hpath, fqn );
		histos[ nameOnly(fqn) ] = h;
		histos[ fqn ] = h;
		//INFOC( "[" << nameOnly(fqn) << "] = " << h );
		//INFOC( "[" << fqn << "] = " << h );
	}

	return histos;
}


TH1* VegaXmlPlotter::makeHistogram( string _path, string &fqn ){
	DSCOPE();
	RooPlotLib rpl;

	TH1* h = findHistogram( _path, 0 );
	//INFOC( "Got " << h );
	

	for (auto kv : globalHistos ){
		//INFOC( "mem pool : " << kv.first );
	}

	if ( nullptr == h ) return nullptr;

	config.set( "ClassName", h->ClassName() );
	DLOG( "ClassName %s", config[ "ClassName" ].c_str() );
	
	string name = config.getXString( _path + ":name" );
	string data = config.getXString( _path + ":data" );
	
	fqn = fullyQualifiedName( data, name );

	//INFOC( "[" << fqn << "] = " << h  );
	
	// if ( config.exists( _path + ".Norm" ) && config.getBool( _path + ".Norm", true ) ){
	// 	if ( nullptr != h && h->Integral() > 0 )
	// 		h->Scale( 1.0 / h->Integral() );
	// }

	string styleRef = config.getXString( _path + ":style" );
	//INFOC( "Style Ref : " << styleRef );
	if ( config.exists( styleRef ) ){
		rpl.style( h ).set( config, styleRef );
	}

	rpl.style( h ).set( config, _path + ".style" ).draw();

	if ( config.exists( _path +":after_draw" ) ){
		string cmd = ".x " + config[_path+":after_draw"] + "( " + h->GetName() + " )";
		LOG_F( INFO, "Executing: %s", cmd.c_str()  );
		gROOT->ProcessLine( cmd.c_str() );
	}

	TPaveStats *st = (TPaveStats*)h->FindObject("stats");
	if ( nullptr != st  ){
		positionOptStats( _path, st );
		// st->SetX1NDC( 0.7 ); st->SetX2NDC( 0.975 );
	}
	return h;
}


void VegaXmlPlotter::makeLegend( string _path, map<string, TH1*> &histos ){
	DSCOPE();
	if ( config.exists( _path + ".Legend" ) ){
		
		RooPlotLib rpl;
		float x1, y1, x2, y2;
		x1 = config.getDouble( _path + ".Legend.Position:x1", 0.1 );
		y1 = config.getDouble( _path + ".Legend.Position:y1", 0.7 );
		x2 = config.getDouble( _path + ".Legend.Position:x2", 0.5 );
		y2 = config.getDouble( _path + ".Legend.Position:y1", 0.9 );

		string spos = config.getXString( _path + ".Legend.Position:pos" );

		float w = config.getFloat( _path + ".Legend.Position:w", 0.4 );
		float h = config.getFloat( _path + ".Legend.Position:h", 0.2 );

		if ( spos.find("top") != string::npos ){
			LOG_F( INFO, "LEGEND: TOP" );
			y2 = 0.9;
			if ( nullptr != gPad) y2 = 1.0 - gPad->GetTopMargin();
			y1 = y2 - h;
		}
		if ( spos.find("right") != string::npos ){
			LOG_F( INFO, "LEGEND: RIGHT" );
			x2 = 0.9;
			if ( nullptr != gPad) x2 = 1.0 - gPad->GetRightMargin();
			x1 = x2 - w;
		}
		if ( spos.find("bottom") != string::npos ){
			LOG_F( INFO, "LEGEND: BOTTOM" );
			y1 = 0.1;
			if ( nullptr != gPad) y1 = gPad->GetBottomMargin();
			y2 = y1 + h;
		}
		if ( spos.find("left") != string::npos ){
			LOG_F( INFO, "LEGEND: LEFT" );
			x1 = 0.1;
			if ( nullptr != gPad) x1 = gPad->GetLeftMargin();
			x2 = x1 + w;
		}
		if ( spos.find("vcenter") != string::npos ){
			LOG_F( INFO, "LEGEND: VERTICAL CENTER" );
			y1 = 0.5 - h/2;
			y2 = 0.5 + h/2;
		}
		if ( spos.find("hcenter") != string::npos ){
			LOG_F( INFO, "LEGEND: HORIZONTAL CENTER" );
			x1 = 0.5 - w/2;
			x2 = 0.5 + w/2;
		}

		TLegend * leg = new TLegend( x1, y1, x2, y2 );

		if ( config.exists(  _path + ".Legend:title" ) ) {
			leg->SetHeader( config.getXString( _path + ".Legend:title" ).c_str() );
		}
		leg->SetNColumns( config.getInt( _path + ".Legend:columns", 1 ) );

		vector<string> entries = config.childrenOf( _path + ".Legend", "Entry" );

		for ( string entryPath : entries ){
			//INFO( classname(), "Entry @" << entryPath );
			if ( config.exists( entryPath + ":name" ) != true ) continue;
			string name = config.getXString( entryPath + ":name" );
			
			if ( histos.count( name ) <= 0 || histos[ name ] == nullptr ) {
				continue;
			}
			TH1 * h = histos[ name ];
			//INFO( classname(), "Entry histo=" << h );
			string t = config.getXString(  entryPath + ":title", name );
			string opt = config.getXString(  entryPath + ":opt", "l" );


			if ( config.exists( entryPath + ":markersize" ) ){
				rpl.style( h ).set( config, entryPath );
			}


			leg->AddEntry( h, t.c_str(), opt.c_str() );
		}

		if ( entries.size() <= 0 ){
			// Add an entry for all histograms by default if no <Entry> nodes found
			for ( auto kv : histos ){
				string t = config.getString(  _path + ".Legend." + kv.first+ ":title", kv.first) ;
				string opt = config.getString(  _path + ".Legend." + kv.first+ ":opt", "l") ;
				leg->AddEntry( kv.second, t.c_str(), opt.c_str() );
			}
		}

		if ( config.exists( _path + ".Legend:border_size" ) ){
			leg->SetBorderSize( config.getDouble( _path + ".Legend:border_size" ) );
		}
		if ( config.exists( _path + ".Legend:fill_color" ) ){
			leg->SetFillColor( color( config.getString( _path + ".Legend:fill_color" ) ) );
		}
		if ( config.exists( _path + ".Legend:fill_style" ) ){
			//INFOC( "FillStyle=" << config.getInt( _path + ".Legend:fill_style" ) );
			leg->SetFillStyle( config.getInt( _path + ".Legend:fill_style" ) );
		}

		leg->Draw( );

	}
}

void VegaXmlPlotter::makeLatex( string _path ){
	DSCOPE();
	//INFO( classname(), "_path=" << quote(_path) );
	
	TLatex latex;
	vector<string> latex_paths = config.childrenOf( _path, "TLatex", 1 );
	for ( string ltpath : latex_paths ){
		if ( !config.exists( ltpath + ":text" ) ) continue;

		string text = config.getXString( ltpath + ":text" );
		
		//INFO( classname(), "Latex @ " << ltpath );
		latex.SetTextSize( config.getFloat( ltpath + ":size", 0.05 ) );

		if ( config.exists( ltpath +":color" ) ){
			string cs = config.getXString( ltpath + ":color" );
			latex.SetTextColor( color( cs ) );
		} else {
			latex.SetTextColor( kBlack );
		}
		

		TLatex * TL = latex.DrawLatexNDC( config.getFloat( ltpath + ":x" ), 
							config.getFloat( ltpath + ":y" ), 
							config.getXString( ltpath + ":text" ).c_str() );

		if ( config.exists( ltpath + ":font" ) ){
			TL->SetTextFont( config.getInt( ltpath + ":font" ) );
		}

	}
}

void VegaXmlPlotter::makeLine( string _path ){
	DSCOPE();
	//INFO( classname(), "_path=" << quote(_path) );

	vector<string> line_paths = config.childrenOf( _path, "TLine", 1 );
	for ( string ltpath : line_paths ){
		
		//INFO( classname(), "Line (" << config.getDouble( ltpath + ":x1", 0.0 ) << ", " << config.getDouble( ltpath + ":y1", 0.0 ) << ") -> (" << config.getDouble( ltpath + ":x2", 0.0 ) << ", " << config.getDouble( ltpath + ":y2", 0.0 ) << ")" )
		TLine * line = new TLine( 
			config.getDouble( ltpath + ":x1", 0.0 ),
			config.getDouble( ltpath + ":y1", 0.0 ),
			config.getDouble( ltpath + ":x2", 0.0 ),
			config.getDouble( ltpath + ":y2", 0.0 ) );

		//INFO( classname(), "TLine @ " << ltpath );
		
		// TODO: add string color support here
		line->SetLineColor( color( config.getString( ltpath + ":color" ) ) );
		line->SetLineWidth( config.getInt( ltpath + ":width", 1 ) );
		line->SetLineStyle( config.getInt( ltpath + ":style", 2 ) );

		line->Draw("same");
	}

}

void VegaXmlPlotter::makeExports( string _path, TPad * _pad ){
	DSCOPE();
	vector<string> exp_paths = config.childrenOf( _path, "Export", 1 );
	if ( nullptr == _pad ) {
		_pad = (TPad*)gPad;
	}
	if ( nullptr == _pad ) return; 
	for ( string epath : exp_paths ){
		if ( !config.exists( epath + ":url" ) ) continue;
		string url = config.getXString( epath + ":url" );
		_pad->Print( url.c_str() );
	}
}

TH1* VegaXmlPlotter::makeHistoFromDataTree( string _path, int iHist ){
	DSCOPE();

	string data = config.getXString( _path + ":data" );
	string hName = config.getXString( _path + ":name" );

	if ( "" == data && hName.find( "/" ) != string::npos ){
		data = dataOnly( hName );
		hName = nameOnly( hName );
	}

	
	TChain * chain = dataChains[ data ];
	string title = config.getXString( _path + ":title" );

	string drawCmd = config.getXString( _path + ":draw" ) + " >> " + hName;
	string selectCmd = config.getXString( _path + ":select" );
	
	// if bins are given lets assume we need to make the histo first
	if ( config.exists( _path + ":bins_x" ) ){
		HistoBins bx( config, config.getXString( _path + ":bins_x" ) );
		HistoBins by( config, config.getXString( _path + ":bins_y" ) );
		HistoBins bz( config, config.getXString( _path + ":bins_z" ) );
		HistoBook::make( "D", hName, title, bx, by, bz );
	}

	LOG_S(INFO) << "TTree->Draw( \"" << drawCmd << "\", \"" << selectCmd << "\" )";
	chain->Draw( drawCmd.c_str(), selectCmd.c_str() );


	TH1 *h = (TH1*)gPad->GetPrimitive( hName.c_str() );
	globalHistos[ hName ] = h;

	return h;
}

void VegaXmlPlotter::positionOptStats( string _path, TPaveStats * st ){
	DSCOPE();
	if ( nullptr == st ) return;

	vector<double> pos = config.getDoubleVector( "OptStats" );

	// global first
	if ( config.exists( "OptStats:x1" ) )
		st->SetX1NDC( config.getFloat( "OptStats:x1" ) );
	if ( config.exists( "OptStats:x2" ) )
		st->SetX2NDC( config.getFloat( "OptStats:x2" ) );
	if ( config.exists( "OptStats:y1" ) )
		st->SetY1NDC( config.getFloat( "OptStats:y1" ) );
	if ( config.exists( "OptStats:y2" ) )
		st->SetY2NDC( config.getFloat( "OptStats:y2" ) ); 
	
	// local 
	if ( config.exists( _path + ".OptStats:x1" ) )
		st->SetX1NDC( config.getFloat( _path + ".OptStats:x1" ) );
	if ( config.exists( _path + ".OptStats:x2" ) )
		st->SetX2NDC( config.getFloat( _path + ".OptStats:x2" ) );
	if ( config.exists( _path + ".OptStats:y1" ) )
		st->SetY1NDC( config.getFloat( _path + ".OptStats:y1" ) );
	if ( config.exists( _path + ".OptStats:y2" ) )
		st->SetY2NDC( config.getFloat( _path + ".OptStats:y2" ) ); 
}

TCanvas* VegaXmlPlotter::makeCanvas( string _path ){
	DSCOPE();
	int width = -1, height = -1;
	// global first
	width = config.getInt( "TCanvas:width", -1 );
	height = config.getInt( "TCanvas:height", -1 );

	width = config.getInt( _path + ".TCanvas:width", width );
	height = config.getInt( _path + ".TCanvas:height", height );

	TCanvas * c = nullptr; 
	if ( width != -1 && height > -1 )
		c = new TCanvas( "c", "c", width, height );
	else 
		c = new TCanvas( "c" );

	c->SetFillColor(0); 
	c->SetFillStyle(4000);

	return c;
}


void VegaXmlPlotter::makeTransforms(  ){
	DSCOPE();
	vector<string> tform_paths = config.childrenOf( nodePath, "Transform" );
	LOG_F( INFO, "Found %lu Transforms", tform_paths.size() );
	//INFOC( "Found " << tform_paths.size() << plural( tform_paths.size(), " Transform set", " Transform sets" ) );
	
	for ( string path : tform_paths ){
		vector<string> states = config.getStringVector( path + ":states" );
		int i = 0;
		if( states.size() == 0 ) states.push_back( "" );

		for ( string state : states ){
			// set the global vars for this template
			config.set( "state", state );
			config.set( "i", ts(i) );
			makeTransform( path );
			i++;
		}
	}
}

void VegaXmlPlotter::makeTransform( string tpath ){
	DSCOPE();
	vector<string> tform_paths = config.childrenOf( tpath );
	LOG_F( INFO, "Found %lu Transform paths", tform_paths.size() );
	//INFOC( "Found " << tform_paths.size() << plural( tform_paths.size(), " Transform", " Transforms" ) );


	// TODO function map from string to transform??
	for ( string tform : tform_paths ){
		string tn = config.tagName( tform );
		//:( tn );

		if ( "ProjectionX" == tn )
			makeProjectionX( tform );
		if ( "ProjectionY" == tn )
			makeProjectionY( tform );
		if ( "Add" == tn )
			makeAdd( tform );
		if ( "Divide" == tn )
			makeDivide( tform );
		if ( "Rebin" == tn )
			makeRebin( tform );
		if ( "Scale" == tn )
			makeScale( tform );
		if ( "Clone" == tn )
			makeClone( tform );
		if ( "Draw" == tn )
			makeDraw( tform );
		

	}
}


void VegaXmlPlotter::makeProjection( string _path ){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	string nn = config.getXString( _path + ":save_as" );
	string axis = config.getXString( _path + ":axis", "x" );

	TH1 * h = findHistogram( _path, 0 );
	TH2 *h2 = dynamic_cast<TH2*>(h);
	TH3 *h3 = dynamic_cast<TH3*>(h);
	
	if ( nullptr == h ) {
		// ERRORC( "Could not find histogram " << quote( nn ) );
		return;
	}

	if ( nullptr != h3 ){

		if ( "x" == axis || "X" == axis ){
			int by1 = getProjectionBin( _path, h, "y", "1",  0 );
			int by2 = getProjectionBin( _path, h, "y", "2", -1 );

			int bz1 = getProjectionBin( _path, h, "z", "1",  0 );
			int bz2 = getProjectionBin( _path, h, "z", "2", -1 );

			TH1 * hNew = h3->ProjectionX( nn.c_str(), by1, by2, bz1, bz2 );
			globalHistos[ nn ] = hNew;
		} else if ( "y" == axis || "Y" == axis ){
			int bx1 = getProjectionBin( _path, h, "x", "1",  0 );
			int bx2 = getProjectionBin( _path, h, "x", "2", -1 );

			int bz1 = getProjectionBin( _path, h, "z", "1",  0 );
			int bz2 = getProjectionBin( _path, h, "z", "2", -1 );

			TH1 * hNew = h3->ProjectionY( nn.c_str(), bx1, bx2, bz1, bz2 );
			globalHistos[ nn ] = hNew;
		} else if ( "z" == axis || "Z" == axis ){
			int bx1 = getProjectionBin( _path, h, "x", "1",  0 );
			int bx2 = getProjectionBin( _path, h, "x", "2", -1 );

			int by1 = getProjectionBin( _path, h, "y", "1",  0 );
			int by2 = getProjectionBin( _path, h, "y", "2", -1 );

			TH1 * hNew = h3->ProjectionZ( nn.c_str(), bx1, bx2, by1, by2 );
			globalHistos[ nn ] = hNew;
		}

	} else if ( nullptr != h2 ){
		if ( "x" == axis || "X" == axis )
			return makeProjectionX( _path );
		else 
			return makeProjectionY( _path );
	}

}


void VegaXmlPlotter::makeProjectionX( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string nn = config.getXString( _path + ":save_as" );
	
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		// ERRORC( "Could not find histogram " << quote( d + "/" + n ) );
		return;
	}

	double y1 = -999;
	int b1 = config.getInt( _path + ":b1", 0 );
	if ( config.exists( _path + ":y1" ) ){
		y1 = config.getDouble( _path + ":y1", -1 );
		//INFOC( "ProjectionX y1=" << y1 );
		b1 = ((TH2*)h)->GetYaxis()->FindBin( y1 );
	}
	
	double y2 = -999;
	int b2 = config.getInt( _path + ":b2", -1 );
	if ( config.exists( _path + ":y2" ) ){
		y2 = config.getDouble( _path + ":y2", -1 );
		//INFOC( "ProjectionX y2=" << y2 );
		b2 = ((TH2*)h)->GetYaxis()->FindBin( y2 );
	}
	//INFOC( "ProjectionX [" << nn << "] b1=" << b1 << ", b2="<<b2 );

	double step = config.getDouble( _path + ":step", -1.0 );
	if ( step > 0.0 && y1 != -999 && y2 != -999 ){
		for (double y = y1; y <= y2; y += step ){
			config.set( _path + ":step", "" ); // prevent inf recursion
			config.set( _path +":save_as", nn + "_" + dtes( y, "p" ) + "_" + dtes( y + step, "p" ) );
			config.set( _path + ":y1", dts( y ) );
			config.set( _path + ":y2", dts( y + step ) );

			makeProjectionX( _path );
		}
		return;
	} 

	TH1 * hOther = ((TH2*)h)->ProjectionX( nn.c_str(), b1, b2 );
	h = hOther;

	globalHistos[ nn ] = h;
}

void VegaXmlPlotter::makeProjectionY( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		// ERRORC( "Could not find histogram " << quote( d + "/" + n ) );
		return;
	}
	string nn = config.getXString( _path + ":save_as" );

	int b1 = config.getInt( _path + ":b1", 0 );
	if ( config.exists( _path + ":x1" ) ){
		double x1 = config.getDouble( _path + ":x1", -1 );
		// INFOC( "ProjectionY x1=" << x1 );
		b1 = ((TH2*)h)->GetYaxis()->FindBin( x1 );
	}
	
	int b2 = config.getInt( _path + ":b2", -1 );
	if ( config.exists( _path + ":x2" ) ){
		double x2 = config.getDouble( _path + ":x2", -1 );
		// INFOC( "ProjectionY x2=" << x2 );
		b2 = ((TH2*)h)->GetYaxis()->FindBin( x2 );
	}
	//INFOC( "ProjectionX [" << nn << "] b1=" << b1 << ", b2="<<b2 );

	TH1 * hOther = ((TH2*)h)->ProjectionY( nn.c_str(), b1, b2 );
	h = hOther;

	globalHistos[ nn ] = h;

}

void VegaXmlPlotter::makeMultiAdd( string _path ){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}


	string d = config.getXString( _path + ":data" );
	vector<string> n = config.getStringVector( _path + ":name" );
	if ( n.size() < 1 ) {
		// ERRORC( "No names given" );
	}

	string nn = config.getXString( _path + ":save_as" );

	// TODO allow modifier for each
	// double mod = config.getDouble( _path + ":mod", 1.0 );

	TH1 * hFirst = findHistogram( d, n[0] );
	if ( nullptr == hFirst ){
		// ERRORC( "Hit Null Histo " << n[0] );
		return;
	}
	TH1 * hSum = (TH1*)hFirst->Clone( nn.c_str() );

	for ( int i = 1; i < n.size(); i++ ){
		TH1 * h = findHistogram( d, n[i] );
		if ( nullptr == h ) {
			// WARNC( "Hit Null Histo : " << n[i] << " SKIPPING" );
			continue;
		}
		hSum ->Add( h );
	}

	globalHistos[nn] = hSum;
}

void VegaXmlPlotter::makeAdd( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	if ( config.exists( _path+":name" ) && config.getStringVector( _path+":name" ).size() > 1 ){
		makeMultiAdd( _path );
		return;
	}
	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string nn = config.getXString( _path + ":save_as" );
	double mod = config.getDouble( _path + ":mod", 1.0 );

	TH1 * hA = findHistogram( _path, 0, "A" );
	TH1 * hB = findHistogram( _path, 0, "B" );
	if ( nullptr == hA || nullptr == hB ) {
		// ERRORC( "Could not find histogram" );
		return;
	}

	TH1 * hSum = (TH1*) hA->Clone( nn.c_str() );
	hSum->Add( hB, mod );
	globalHistos[nn] = hSum;
}
void VegaXmlPlotter::makeDivide( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string nn = config.getXString( _path + ":save_as" );

	TH1 * hNum = findHistogram( _path, 0, "A" );
	TH1 * hDen = findHistogram( _path, 0, "B" );
	if ( nullptr == hNum || nullptr == hDen ) {
		// ERRORC( "Could not find histogram" );
		return;
	}

	TH1 * hOther = (TH1*) hNum->Clone( nn.c_str() );
	hOther->Divide( hDen );
	globalHistos[nn] = hOther;
}

void VegaXmlPlotter::makeRebin( string _path ){
	DSCOPE();
	// rebins to array of bin edges
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		// ERRORC( "Could not find histogram " << quote( d + "/" + n ) );
		return;
	}
	string nn = config.getXString( _path + ":save_as" );
	TH1 * hOther = nullptr;((TH1*)h)->Clone( nn.c_str() );

	HistoBins bx, by;
	if ( config.exists( _path +":bins_x" )  ){
		if ( config.exists( config.getXString( _path+":bins_x" ) ) ){
			bx.load( config, config.getXString( _path+":bins_x" ) );
		} else {
			bx.load( config, _path + ":bins_x" );
		}
	}
	if ( config.exists( _path +":bins_y" )  ){
		if ( config.exists( config.getXString( _path+":bins_y" ) ) ){
			by.load( config, config.getXString( _path+":bins_y" ) );
		} else {
			by.load( config, _path + ":bins_y" );
		}
	}

	int nDim = 0;
	if ( bx.nBins() > 0 ) nDim++;
	if ( by.nBins() > 0 ) nDim++;


	if ( nDim == 1 && bx.nBins() > 0 ){
		hOther = h->Rebin( bx.nBins(), nn.c_str(), bx.bins.data() );
		globalHistos[nn] = hOther;
	} else if ( nDim == 1 ){
		// ERRORC( "Cannot Rebin, check error message for x bins" );
	}

	if ( nDim == 2 && bx.nBins() > 0 && by.nBins() > 0 && nullptr != dynamic_cast<TH2*>( h ) ){
		hOther = new TH2D( nn.c_str(), h->GetTitle(), bx.nBins(), bx.bins.data(), by.nBins(), by.bins.data() );
		HistoBins::rebin2D( dynamic_cast<TH2*>(h), dynamic_cast<TH2*>(hOther) );
		globalHistos[nn] = hOther;
	} else if ( nDim == 2 ){
		if ( nullptr == dynamic_cast<TH2*>( h ) ){
			// ERRORC( "Input histogram is not 2D" );
		} else {
			// ERRORC( "Cannot Rebin, check error for binsx and y" );
		}
	}


	// TODO add 2D;
	// HistoBins by;
	// if ( config.exists( _path +":bins_y" )  ){
	// 	if ( config.exists( config.getXString( _path+":bins_y" ) ) ){
	// 		by.load( config, config.getXString( _path+":bins_y" ) );
	// 	} else {
	// 		by.load( confing, _path + ":bins_y" );
	// 	}
	// }
	// if ( by.nBins() > 0 ){
	// 	hOther = h->RebinY( by.nBins(), nn.c_str(), by.bins.data() );
	// 	h = hOther;
	// }
}

void VegaXmlPlotter::makeScale( string _path ){
	DSCOPE();

	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		// ERRORC( "Could not find histogram " << quote( d + "/" + n ) );
		return;
	}
	string nn = config.getXString( _path + ":save_as" );
	TH1 * hOther = (TH1*)h->Clone( nn.c_str() );

	double factor = config.getDouble( _path + ":factor", 1.0 );
	string opt    = config.getXString( _path +":opt", "" );

	hOther->Scale( factor, opt.c_str() );
	globalHistos[nn] = hOther;
}

void VegaXmlPlotter::makeDraw( string _path ){
	DSCOPE();

	string d = config.getXString( _path + ":data" );
	string nn = nameOnly( config.getXString( _path + ":name" ) );
	TH1 * h = makeHistoFromDataTree( _path, 0 ); //findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "Could not make %s", nn.c_str() );
		return;
	}

	// string nn = config.getXString( _path + ":save_as" );
	globalHistos[ nn ] = h;
}




void VegaXmlPlotter::makeClone( string _path ){

	if ( !config.exists( _path + ":save_as" ) ){
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		return;
	}

	string nn = config.getXString( _path + ":save_as" );
	TH1 * hOther = (TH1*)h->Clone( nn.c_str() );

	// if bin ranges or axis ranges exist then clear and clone subrange
	if ( config.exists( _path+":b1" ) || config.exists( _path+":x1" ) ){
		hOther->Reset();
		int b1 = config.getInt( _path + ":b1", 0 );
		if ( config.exists( _path + ":x1" ) ){
			double x1 = config.getDouble( _path + ":x1", -1 );
			b1 = h->GetXaxis()->FindBin( x1 );
		}
		
		int b2 = config.getInt( _path + ":b2", -1 );
		if ( config.exists( _path + ":x2" ) ){
			double x2 = config.getDouble( _path + ":x2", -1 );
			b2 = h->GetXaxis()->FindBin( x2 );
		}

		// now clone the subrange
		HistoBook::cloneBinRange( h, hOther, b1, b2 );
	}

	globalHistos[nn] = hOther;
}



void VegaXmlPlotter::drawCanvas( string _path ){
	DSCOPE();

	XmlCanvas xcanvas( config, _path );
	
	vector<string> ppads = config.childrenOf( _path, "Pad" );
	vector<string> pplots = config.childrenOf( _path, "Plots" );
	vector<string> paths = ppads;
	
	if ( ppads.size() <= 0 ) paths = pplots;

	for ( string p : paths ){
		string n = config.getXString( p + ":name", "" );
		//INFOC( "Painting PAD " << n );
		if ( "" == n ){
			// WARNC( "Skipping pad without name" );
			continue;
		}

		XmlPad * xpad = xcanvas.activatePad( n );

		// move pad to origin
		xpad->moveToOrigin();
		xpad->getRootPad()->Update();
		
		makePlot( p );

		xpad->reposition();
		xpad->getRootPad()->Update();
		// pad->Update();
		// pad->Print( ("pad_" + n + ".pdf").c_str() );
		// makeExports( p );

	} // loop over pads
	xcanvas.getCanvas()->Update();

	makeExports( _path, xcanvas.getCanvas() );
}


map<string, TObject*> VegaXmlPlotter::dirMap( TDirectory *dir, string prefix, bool dive ) {
	DSCOPE();

	map<string, TObject*> mp;
	
	if ( nullptr == dir ) return mp;
	
	TList* list = dir->GetListOfKeys() ;
	if ( !list ) return mp;

	TIter next(list) ;
	TKey* key ;
	TObject* obj;
	while ( (key = (TKey*)next()) ) {
		obj = key->ReadObj() ;
		string key = prefix + obj->GetName();
		mp[ key ] = obj;
		if ( dive && 0 == strcmp( "TDirectoryFile", obj->ClassName() ) ){
			auto m = dirMap( (TDirectory*)obj, key + "/" );
			mp.insert( m.begin(), m.end() );
		}
	}
	return mp;
}

bool VegaXmlPlotter::typeMatch( TObject *obj, string type ){
	if ( nullptr == obj ) return false;
	if ( "" == type ) return true;
	string objType = obj->ClassName();
	if ( objType.substr( 0, type.length() ) == type ) return true;
	return false;
}

vector<string> VegaXmlPlotter::glob( string query ){
	DSCOPE();
	vector<string> names;
	
	// allow prefix of type
	//example "TH1:test*" will glob all names like test* that are TH1 (TH1D, TH1F etc)
	size_t typePos = query.find( ":" );
	string type = "";
	if ( typePos != string::npos ){
		type = query.substr( 0, typePos);
		query = query.substr( typePos+1 );
		DLOG_F( INFO, "Query: %s, type: %s", query.c_str(), type.c_str() );
	}


	// TODO add suffix support
	size_t pos = query.find( "*" );

	if ( pos != string::npos ){
		DLOG( "WILDCARD FOUND at %lu ", pos );
		string qc = query.substr( 0, pos );
		DLOG( "query compare : %s ", qc.c_str()  );
		// TODO add support for others in data file... 
		for ( auto kv : globalHistos ){
			string oType = kv.second->ClassName();
			DLOG( "[%s] = %s", kv.first.c_str(), oType.c_str() );
			if ( kv.first.substr( 0, pos ) == qc ){
				names.push_back( kv.first );
				DLOG( "Query MATCH: %s", kv.first.c_str() );
			}
		}

		// now try data files:
		for ( auto df : dataFiles ){
			auto objs = dirMap( df.second);
			for ( auto kv : objs ){
				DLOG( "[%s]=%s, typeMatch=%s", kv.first.c_str(), kv.second->ClassName(), bts( typeMatch( kv.second, type ) ).c_str() );
				if ( kv.first.substr( 0, pos ) == qc && typeMatch( kv.second, type ) ){
					names.push_back( df.first + "/" + kv.first );
				}
			}
		} // loop dataFiles
		




	} else {

	}
	return names;
}

void VegaXmlPlotter::setDefaultPalette(){
	const Int_t NRGBs = 5;
	const Int_t NCont = 255;

	Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
	Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
	Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
	Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
	TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
	gStyle->SetNumberContours(NCont);
}
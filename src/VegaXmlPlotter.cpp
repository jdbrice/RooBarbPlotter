#define LOGURU_IMPLEMENTATION 1
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

#include <thread>

void VegaXmlPlotter::init(){
	DSCOPE();
	LOG_F( INFO, "VegaXmlPlotter Starting" );
	string logfile = config[ "Log:url" ];
	if ( config.exists( "Log:url" ) ){
		LOG_F( INFO, "Writing log to %s", logfile.c_str() );
		loguru::add_file(logfile.c_str(), loguru::Truncate, loguru::Verbosity_MAX);
	} else {
		LOG_F( INFO, "Not writing logfile");
	}

	setDefaultPalette();

	handle_map[ "TCanvas"      ] = &VegaXmlPlotter::exec_TCanvas;
	handle_map[ "Data"         ] = &VegaXmlPlotter::exec_Data;
	handle_map[ "TFile"        ] = &VegaXmlPlotter::exec_TFile;
	handle_map[ "Plot"         ] = &VegaXmlPlotter::exec_Plot;
	handle_map[ "Loop"         ] = &VegaXmlPlotter::exec_Loop;
	handle_map[ "Axes"         ] = &VegaXmlPlotter::exec_Axes;
	handle_map[ "Export"       ] = &VegaXmlPlotter::exec_Export;
	handle_map[ "ExportConfig" ] = &VegaXmlPlotter::exec_ExportConfig;
	handle_map[ "Histo"        ] = &VegaXmlPlotter::exec_Histo;
	handle_map[ "TLine"        ] = &VegaXmlPlotter::exec_TLine;
	handle_map[ "TLatex"       ] = &VegaXmlPlotter::exec_TLatex;
	handle_map[ "TLegend"      ] = &VegaXmlPlotter::exec_TLegend;
	handle_map[ "Legend"       ] = &VegaXmlPlotter::exec_TLegend;

	handle_map[ "Canvas"       ] = &VegaXmlPlotter::exec_Canvas;
	handle_map[ "Pad"          ] = &VegaXmlPlotter::exec_Pad;
	handle_map[ "Margins"      ] = &VegaXmlPlotter::exec_Margins;

	handle_map[ "Transforms"   ] = &VegaXmlPlotter::exec_Transforms;
	handle_map[ "Transform"   ] = &VegaXmlPlotter::exec_Transforms;

	handle_map[ "Projection"   ] = &VegaXmlPlotter::exec_transform_Projection;
	handle_map[ "ProjectionX"  ] = &VegaXmlPlotter::exec_transform_ProjectionX;
	handle_map[ "ProjectionY"  ] = &VegaXmlPlotter::exec_transform_ProjectionY;
	handle_map[ "MultiAdd"     ] = &VegaXmlPlotter::exec_transform_MultiAdd;
	handle_map[ "Add"          ] = &VegaXmlPlotter::exec_transform_Add;
	handle_map[ "Divide"       ] = &VegaXmlPlotter::exec_transform_Divide;
	handle_map[ "Rebin"        ] = &VegaXmlPlotter::exec_transform_Rebin;
	handle_map[ "Scale"        ] = &VegaXmlPlotter::exec_transform_Scale;
	handle_map[ "Draw"         ] = &VegaXmlPlotter::exec_transform_Draw;
	handle_map[ "Clone"        ] = &VegaXmlPlotter::exec_transform_Clone;
	handle_map[ "Smooth"       ] = &VegaXmlPlotter::exec_transform_Smooth;
	handle_map[ "CDF"          ] = &VegaXmlPlotter::exec_transform_CDF;
	handle_map[ "Style"        ] = &VegaXmlPlotter::exec_transform_Style;
	handle_map[ "SetBinError"  ] = &VegaXmlPlotter::exec_transform_SetBinError;
	handle_map[ "BinLabels"    ] = &VegaXmlPlotter::exec_transform_BinLabels;
} // init

void VegaXmlPlotter::exec_node( string _path ){
	DSCOPE();
	if ( false == config.exists( _path ) )
		return;

	string tag = config.tagName( _path );
	DLOG( "exec_node( %s ), tag = %s", _path.c_str(), tag.c_str() );

	if ( 0 == handle_map.count( tag ) ){
		LOG_F( ERROR, "No Handler for %s", tag.c_str() );
		return;
	}

	exec( tag, _path );
} // exec_node

string VegaXmlPlotter::random_string( size_t length ){
	auto randchar = []() -> char
	{
		const char charset[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[ rand() % max_index ];
	};
	string str(length,0);
	generate_n( str.begin(), length, randchar );
	return str;
}

void VegaXmlPlotter::make(){
	DSCOPE();

	vector<string> paths = config.childrenOf( "", "ExportConfig" );
	for ( string p : paths ){
		exec_node( p );
	}

	// Load data first
	paths = config.childrenOf( "", "Data" );
	for ( string p : paths ){
		exec_node( p );
	}

	if ( paths.size() > 0 && dataFiles.size() == 0 && dataChains.size() == 0 ){
		LOG_F( WARNING, "No valid data files found, exiting" );
		return;
	}

	// if a TFile node is given for writing output, then execute it
	exec_node( "TFile" );

	// Top level nodes
	vector<string> tlp = { "TCanvas", "Plot", "Loop", "Canvas", "Transforms", "Transform" };
	paths = config.childrenOf( "", 1 );
	for ( string p : paths ){
		string tag = config.tagName( p );
		if ( std::find( tlp.begin(), tlp.end(), tag ) != tlp.end() ){
			exec_node( p );
		}
	}

	// Write data out if requested
	if ( dataOut && dataOut->IsOpen() ){
		dataOut->Write();
		dataOut->Close();
		LOG_F( INFO, "Write to %s completed", config.getString( "TFile:url" ).c_str() );
	}
} // make

map<string, shared_ptr<TF1> > VegaXmlPlotter::makeTF(string _path){
	DSCOPE();
	vector<string> paths = config.childrenOf( _path );
	map< string, shared_ptr<TF1> > funcs;
	for ( string path : paths ){
		string name = config.getString( path + ":name" );
		string tag = config.tagName(path);
		if ( "TF1" == tag || "XmlFunction" == tag ){
			funcs[ name ] = makerTF.make( config, path );

			if (nullptr == funcs[ name ]){
				LOG_F( INFO, "need to load TF! from file" );
			}

		}
	}
	return funcs;
}

void VegaXmlPlotter::loadDataFile( string _path ){
	DSCOPE();
	if ( config.exists( _path + ":name" ) && config.exists( _path + ":url" )  ){
		string name = config.getXString( _path+":name" );
		string url = config.getXString( _path+":url" );

		DLOG( "Data[%s] = %s", name.c_str(), url.c_str() );
		// LOG_S( INFO ) <<  "Data name=" << name << " @ " << url ;

		TFile * f = new TFile( url.c_str() );
		if ( false == f->IsOpen() ){
			LOG_F( ERROR, "%s cannot be opened", url.c_str() );
			return;
		}

		dataFiles[ name ] = f;
		LOG_F( INFO, "Data[%s] = %s", name.c_str(), url.c_str() );

		if ( config.getBool( _path + ":inline", false ) )
			inlineDataFile( _path, f );

	} else if ( config.exists( _path + ":name" ) ){
		string name = config.getXString( _path+":name" );
		string fname = "tmp_" + name + ".root";
		TFile *f = new TFile( fname.c_str(), "RECREATE" );
		f->cd();
		vector<string> paths = config.childrenOf( _path, "HistogramData" );
		for ( string p : paths ){

			TH1 * _h = XmlHistogram::fromXml( config, p );
			_h->Write();
			LOG_F( INFO, "Making %s = %p", p.c_str(), _h );
			dataFiles[ name ] = f;
		}
	}
} // loadDataFiles

void VegaXmlPlotter::inlineDataFile( string _path, TFile *_f ){
	DSCOPE();
	DLOG( "Inlining file @ %s", _path.c_str() );
	auto dm = dirMap( _f );
	string xml = XmlConfig::declarationV1;
	xml += "\n<config>";
	for ( auto obj : dm ){
		TH1 * h = dynamic_cast<TH1*>( obj.second );
		if ( nullptr != h ){
			DLOG( "inlining %s", obj.first.c_str() );
			xml += "\n" + XmlHistogram::toXml( h );
		}
	}

	xml += "\n</config>";

	XmlConfig tmpcfg;
	tmpcfg.loadXmlString( xml );
	// config.include( tmpcfg, _path, true );
	// now remove the url attribute to make it an inlinde data file
	config.deleteAttribute( _path + ":url" );

	// LOG_F( INFO, "resulting XML to inline : \n%s", xml.c_str() );
} // inlineDataFile

int VegaXmlPlotter::numberOfData() {
	DSCOPE();
	return dataFiles.size() + dataChains.size();
} // numberOfData

void VegaXmlPlotter::loadChain( string _path ){
	DSCOPE();
	string name     = config.getXString( _path + ":name" );
	string treeName = config.getXString( _path + ":treeName" );
	string url      = config.getXString( _path + ":url" );
	int maxFiles    = config.getInt( _path + ":maxFiles", -1 );
	int index       = config.getInt( _path + ":index", -1 );
	int splitBy     = config.getInt( _path + ":splitBy", 50 );

	dataChains[ name ] = new TChain( treeName.c_str() );
	
	if ( url.find( ".lis" ) != std::string::npos ){
		if ( index >= 0 ){
			ChainLoader::loadListRange( dataChains[ name ], url, index, splitBy );
			LOG_F( INFO, "Loading index=%d, splitBy=%d", index, splitBy );
		}else 
			ChainLoader::loadList( dataChains[ name ], url, maxFiles );
	} else 
		ChainLoader::load( dataChains[ name ], url, maxFiles );
	LOG_S(INFO) << "Loaded TTree [name=" << quote(name) << "] from url: " << url;
	int nFiles = 0;
	if ( dataChains[name]->GetListOfFiles() )
		nFiles = dataChains[name]->GetListOfFiles()->GetEntries();
	LOG_S(INFO) << "Chain has " << nFiles << plural( nFiles, " file", " files" );
} // loadChain

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

TObject* VegaXmlPlotter::findObject( string _path ){
	DSCOPE();

	string data = config.getXString( _path + ":data", config.getXString( _path + ":data" ) );
	string name = config.getXString( _path + ":name", config.getXString( _path + ":name" ) );

	if ( "" == data && name.find( "/" ) != string::npos ){
		data = dataOnly( name );
		//INFOC( "data from name " << quote( data ) );
		name = nameOnly( name );
	}

	//INFO( classname(), "Looking for [" << name << "] in " << data << "=(" << dataFiles[data] <<")" );
	// first check for a normal histogram from a root file
	if ( dataFiles.count( data ) > 0 && dataFiles[ data ] ){
		LOG_F( INFO, "Lokking in %s", data.c_str() );
		if ( nullptr == dataFiles[ data ]->Get( name.c_str() ) ) return nullptr;
		TObject * obj = (TH1*)dataFiles[ data ]->Get( name.c_str() );
		return obj;
	}

	// finally look for histos we made and named in the ttree drawing
	if ( globalHistos.count( name ) > 0 && globalHistos[ name ] ){
		LOG_F( INFO, "Lokking in globalHistos" );
		return globalHistos[ name ];
	}

	if ( globalGraphs.count( name ) > 0 && globalGraphs[ name ] ){
		LOG_F( INFO, "Lokking in globalGraphs" );
		return globalGraphs[ name ];
	}

	LOG_F( WARNING, "Object [%s/%s] was not found in any source", data.c_str(), name.c_str() );
	return nullptr;
} // findObject

TH1 *VegaXmlPlotter::findHistogram( string _data, string _name ){
	DSCOPE();
	DLOG( "_data=%s, _name=%s", _data.c_str(), _name.c_str() );

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
} // findHistogram

TH1* VegaXmlPlotter::findHistogram( string _path, int iHist, string _mod ){
	DSCOPE();
	DLOG( "_path=%s, iHist=%d, _mod=%s", _path.c_str(), iHist, _mod.c_str() );

	string data = config.getXString( _path + ":data" + _mod, config.getXString( _path + ":data" ) );
	string name = config.getXString( _path + ":name" + _mod, config.getXString( _path + ":name" + _mod ) );

	if ( "" == data && name.find( "/" ) != string::npos ){
		data = dataOnly( name );
		//INFOC( "data from name " << quote( data ) );
		name = nameOnly( name );
	}

	DLOG( "data=%s, name=%s, dataFiles.size()=%lu", data.c_str(), name.c_str(), dataFiles.size() );
	if ( "" == data && dataFiles.size() >= 1 ){
		data = dataFiles.begin()->first;
		DLOG( "data was not set -> setting to %s", data.c_str()  );
	}

	//INFO( classname(), "Looking for [" << name << "] in " << data << "=(" << dataFiles[data] <<")" );
	// first check for a normal histogram from a root file
	if ( dataFiles.count( data ) > 0 && dataFiles[ data ] ){
		
		
		TH1 * h = (TH1*)dataFiles[ data ]->Get( name.c_str() );
		if ( nullptr != h ){
			h = (TH1*)h->Clone( (string("hist_") + h->GetName() ).c_str() );
			if ( config.getBool( _path + ":setdir", true ) ){} 
			else 
				h->SetDirectory(0);
			//INFO( classname(), "Found Histogram [" << name << "]= " << h << " in data file " << data );
			return h;
		}
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
} //findHistogram

TGraph* VegaXmlPlotter::makeGraph( string _path, string &fqn ){
	DSCOPE();
	RooPlotLib rpl;

	TObject * obj = findObject( _path );
	LOG_F( INFO, "Object = %p", obj );
	TGraph * g = dynamic_cast<TGraph*>( obj );

	if ( nullptr == g ) {
		LOG_F( ERROR, "Cannot get Graph @ %s", _path.c_str() );
		return nullptr;
	}
	LOG_F( INFO, "Found Graph at %s", _path.c_str() );

	// set meta info
	config.set( "ClassName", g->ClassName() );

	string name = config.getXString( _path + ":name" );
	string data = config.getXString( _path + ":data" );
	
	fqn = fullyQualifiedName( data, name );


	rpl.style( g ).set( config, _path ).set( config, _path + ":style" ).set( config, _path + ".style" ).draw();

	return g;
} // makeGraph

TH1* VegaXmlPlotter::makeHistogram( string _path, string &fqn ){
	DSCOPE();
	RooPlotLib rpl;

	TH1* h = findHistogram( _path, 0 );
	LOG_F( INFO, "Found Histogram @%s = %p", _path.c_str(), h );
	

	for (auto kv : globalHistos ){
		//INFOC( "mem pool : " << kv.first );
	}

	if ( nullptr == h ) return nullptr;

	config.set( "ClassName", h->ClassName() );
	DLOG( "ClassName %s", config[ "ClassName" ].c_str() );
	
	string name = config.getXString( _path + ":name" );
	string data = config.getXString( _path + ":data" );
	
	fqn = fullyQualifiedName( data, name );

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
	//INFOC( "Style Ref : " << styleRef );
	if ( config.exists( styleRef ) ){
		rpl.style( h ).set( config, styleRef );
	}

	rpl.style( h ).set( config, _path ).set( config, _path + ".style" ).draw();

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
} // makeHistogram

TH1* VegaXmlPlotter::makeHistoFromDataTree( string _path, int iHist ){
	DSCOPE();

	string data = config.getXString( _path + ":data" );
	string hName = config.getXString( _path + ":name" );

	if ( "" == data && hName.find( "/" ) != string::npos ){
		data = dataOnly( hName );
		hName = nameOnly( hName );
	}

	if ( "" == data && dataChains.size() == 1 ){
		data = dataChains.begin()->first;
	} else if ( "" == data ){
		LOG_F( ERROR, "Must specify the data source" );
		return nullptr;
	}
	
	TChain * chain = dataChains[ data ];
	if ( nullptr == chain ){
		LOG_F( ERROR, "Chain is null" );
		return nullptr; 
	}
	string title = config.getXString( _path + ":title" );

	string drawCmd = config.getXString( _path + ":draw" ) + " >> " + hName;
	string selectCmd = config.getXString( _path + ":select" );
	string drawOpt = config.getString( _path + ":opt" );
	
	// if bins are given lets assume we need to make the histo first
	if ( config.exists( _path + ":bins_x" ) ){
		HistoBins bx( config, config.getXString( _path + ":bins_x" ) );
		HistoBins by( config, config.getXString( _path + ":bins_y" ) );
		HistoBins bz( config, config.getXString( _path + ":bins_z" ) );
		HistoBook::make( "D", hName, title, bx, by, bz );
	}

	long N = std::numeric_limits<long>::max();
	if ( config.exists( _path + ":N" ) ){
		N = config.get<long>( _path + ":N" );
	}
		
	LOG_F( INFO, "chain=%p", chain );
	LOG_S(INFO) << "TTree->Draw( \"" << drawCmd << "\", \"" << selectCmd << "\"" << "\", \"" << drawOpt << "\"," << N << " );";
	chain->Draw( drawCmd.c_str(), selectCmd.c_str(), drawOpt.c_str(), N );

	TH1 *h = (TH1*)gPad->GetPrimitive( hName.c_str() );

	if ( config.exists( _path +":after_draw" ) ){
		string cmd = ".x " + config[_path+":after_ draw"] + "( " + h->GetName() + " )";
		LOG_F( INFO, "Executing: %s", cmd.c_str()  );
		// gROOT->ProcessLine( cmd.c_str() );
		gROOT->LoadMacro( config[_path+":after_draw"].c_str() );
		gROOT->ProcessLine( "after_draw()" );
	}	

	globalHistos[ hName ] = h;

	return h;
} // makeHistoFromDataTree

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
} // positionOptStats

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
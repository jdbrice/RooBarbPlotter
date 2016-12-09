#include "VegaXmlPlotter.h"
#include "ChainLoader.h"

#include "TLatex.h"

void VegaXmlPlotter::initialize(){
}

void VegaXmlPlotter::make() {

	loadData();
	makeOutputFile();

	Logger::setGlobalLogLevel( "debug" );
	makePlots();

	if ( dataOut ){
		dataOut->Write();
		dataOut->Close();
	}
}

void VegaXmlPlotter::loadDataFile( string _path ){
	if ( config.exists( _path + ":name" ) ){
		string name = config.getXString( _path+":name" );
		string url = config.getXString( _path+":url" );
		INFO( classname(), "Data name=" << name << " @ " << url );
		TFile * f = new TFile( url.c_str() );
		dataFiles[ name ] = f;
	}
} // loadDataFiles

int VegaXmlPlotter::numberOfData() {
	return dataFiles.size() + dataChains.size();
}


void VegaXmlPlotter::loadChain( string _path ){
	string name     = config.getXString( _path + ":name" );
	string treeName = config.getXString( _path + ":treeName" );
	string url      = config.getXString( _path + ":url" );
	int maxFiles    = config.getInt( _path + ":maxFiles", -1 );

	dataChains[ name ] = new TChain( treeName.c_str() );
	ChainLoader::loadList( dataChains[ name ], url, maxFiles );
	INFO( classname(), "Loaded TTree[ name=" << name << "] from url: " << url );
	int nFiles = 0;
	if ( dataChains[name]->GetListOfFiles() )
		nFiles = dataChains[name]->GetListOfFiles()->GetEntries();
	INFO( classname(), "Chain has " << nFiles << plural( nFiles, " file", " files" ) );
}

void VegaXmlPlotter::loadData(){
	vector<string> data_nodes = config.childrenOf( nodePath, "Data" );

	for ( string path : data_nodes ){
		if ( config.exists( path + ":name" ) && !config.exists( path + ":treeName" ) ){
			loadDataFile( path );
		} else if ( config.exists( path + ":name" ) && config.exists( path + ":treeName" )){
			loadChain( path );
		}
	}
} // loadData


void VegaXmlPlotter::makeOutputFile(){
	if ( config.exists( "TFile:url" ) ){
		dataOut = new TFile( config.getXString( "TFile:url" ).c_str(), "RECREATE" );
	} else {
		INFO( classname(), "No output data file requested" );
	}
}

void VegaXmlPlotter::makePlots(){
	vector<string> plot_paths = config.childrenOf( nodePath, "Plot" );
	INFO( classname(), "Found " << plot_paths.size() << plural( plot_paths.size(), " Plot", " Plots" ) );
	for ( string path : plot_paths ){

		if ( config.exists( path + ".Palette" ) ){
			gStyle->SetPalette( config.getInt( path + ".Palette" ) );
		}

		makeHistograms( path );
		makeLatex(""); // global first
		makeLatex(path);
		makeLine( path );
		makeExports( path );

		// // clean up first
		// for ( TLatex * tl : activeLatex ){
		// 	INFO( classname(), "DELETEING LATEX" );
		// 	tl->Delete();
		// }
		// activeLatex.clear();
		// if ( gPad ) { gPad->Clear(); }
	}
}

void VegaXmlPlotter::makeMargins( string _path ){
	
	// Global first
	if ( config.exists( "Margins:top" ) )
		gPad->SetTopMargin( config.getDouble( "Margins:top" ) );
	if ( config.exists( "Margins:right" ) )
		gPad->SetRightMargin( config.getDouble( "Margins:right" ) );
	if ( config.exists( "Margins:bottom" ) )
		gPad->SetBottomMargin( config.getDouble( "Margins:bottom" ) );
	if ( config.exists( "Margins:left" ) )
		gPad->SetLeftMargin( config.getDouble( "Margins:left" ) );

	if ( config.exists( _path + ".Margins:top" ) )
		gPad->SetTopMargin( config.getDouble( _path + ".Margins:top" ) );
	if ( config.exists( _path + ".Margins:right" ) )
		gPad->SetRightMargin( config.getDouble( _path + ".Margins:right" ) );
	if ( config.exists( _path + ".Margins:bottom" ) )
		gPad->SetBottomMargin( config.getDouble( _path + ".Margins:bottom" ) );
	if ( config.exists( _path + ".Margins:left" ) )
		gPad->SetLeftMargin( config.getDouble( _path + ".Margins:left" ) );
}

TH1* VegaXmlPlotter::findHistogram( string _path, int iHist ){

	string data = config.getXString( _path + ":data" );
	string name = config.getXString( _path + ":name" );
	INFO( classname(), "Looking for [" << name << "] in " << data << "=(" << dataFiles[data] <<")" );
	// first check for a normal histogram from a root file
	if ( dataFiles.count( data ) > 0 && dataFiles[ data ] ){
		if ( nullptr == dataFiles[ data ]->Get( name.c_str() ) ) return nullptr;
		TH1 * h = (TH1*)dataFiles[ data ]->Get( name.c_str() )->Clone( ("hist_" + ts(iHist)).c_str() );
		INFO( classname(), "Found Histogram [" << name << "]= " << h << " in data file " << data );
		return h;
	}

	// look for a dataChain with the name
	if ( dataChains.count( data ) > 0 && dataChains[ data ] ){
		TH1 * h = makeHistoFromDataTree( _path, iHist );
		return h;
	}

	// finally look for histos we made and named in the ttree drawing
	if ( globalHistos.count( name ) > 0 && globalHistos[ name ] )
		return globalHistos[ name ];

	return nullptr;
}

void VegaXmlPlotter::makeHistograms( string _path ){
	Logger::setGlobalLogLevel( "debug" );

	TCanvas * c = makeCanvas( _path ); 
	c->cd();
	makeMargins( _path );
	RooPlotLib rpl;

	map<string, TH1*> histos;

	vector<string> hist_paths = config.childrenOf( _path, "Histo" );
	sort( hist_paths.begin(), hist_paths.end() );
	bool init = false;
	int iHist = 0;
	for ( string hpath : hist_paths ){
		string name= config[ hpath + ":name" ];
		INFO( classname(), "Found Histo [" << name << "]" );

		// string data = config.getXString( hpath + ":data" );
		// if ( dataFiles.count( data ) <= 0 || !dataFiles[ data ] ) continue;
		
		// TH1 * h = (TH1*)dataFiles[ data ]->Get( name.c_str() )->Clone( ("hist_" + ts(iHist)).c_str() );
		TH1 *h = findHistogram( hpath, iHist );
		INFO( classname(), "Got " << h );
		
		if ( nullptr == h ) continue;
		
		// These maight not be unique for multiple datasets so use at your own risk
		histos[ name ] = h;
		// always add the fqn -> data:name
		string data = config.getXString( hpath + ":data" );
		INFO( classname(), "[" << data + "/" + name << "] = " << h  );
		histos[ data + "/" + name ] = h;

		// transforms
		if ( config.exists( hpath + ".Scale" ) && config.getDouble( hpath + ".Scale" ) ){
			h->Scale( config.getDouble( hpath + ".Scale" ) );
		}
		if ( config.exists( hpath + ".Norm" ) && config.getBool( hpath + ".Norm", true ) ){
			h->Scale( 1.0 / h->Integral() );
		}
		if ( config.exists( hpath + ".RebinX" ) && config.getDouble( hpath + ".RebinX" ) ){
			h->RebinX( config.getDouble( hpath + ".RebinX" ) );
		}
		// if ( config.exists( hpath + ".RebinY" ) && config.getDouble( hpath + ".RebinY" ) ){
		// 	h->RebinY( config.getDouble( hpath + ".RebinY" ) );
		// }
		if ( config.exists( hpath + ".Divide" ) ){
			TH1 * hOther = findHistogram( hpath + ".Divide", iHist * 1000 );
			if ( hOther ){
				h->Divide( hOther );
			} else {
				ERROR( classname(), "Cannot divide by nullptr" );
			}
		}

		if ( config.exists( hpath + ".ProjectionY" ) ){
			string npy = data + "/" + name + "_py";

			int b1 = config.getInt( hpath + ".ProjectionY:b1", 
					 ((TH2*)h)->GetXaxis()->FindBin( config.getDouble( hpath + ".ProjectionY:x1", 0 ) ) );
			int b2 = config.getInt( hpath + ".ProjectionY:b2", 
			 		((TH2*)h)->GetXaxis()->FindBin( config.getDouble( hpath + ".ProjectionY:x2", -1 ) ) );

			TH1 * hOther = ((TH2*)h)->ProjectionY( npy.c_str(), b1, b2 );
			h = hOther;
		}

		if ( config.exists( hpath + ".ProjectionX" ) ){
			string npy = data + "/" + name + "_px";

			int b1 = config.getInt( hpath + ".ProjectionX:b1", 
					 ((TH2*)h)->GetYaxis()->FindBin( config.getDouble( hpath + ".ProjectionX:y1", 0 ) ) );
			int b2 = config.getInt( hpath + ".ProjectionX:b2", 
			 		((TH2*)h)->GetYaxis()->FindBin( config.getDouble( hpath + ".ProjectionX:y2", -1 ) ) );

			TH1 * hOther = ((TH2*)h)->ProjectionX( npy.c_str(), b1, b2 );
			h = hOther;
		}


		// offCan->cd();
		// // rpl.style( h ).draw();
		
		if ( !init ){
			init = true;
			h->Draw();
			// INFO( classname(), config.getXString( hpath + ".style:title" ) );
			// h->SetTitle( config.getXString( hpath + ".style:title" ).c_str() );
		} else {
			// h->Draw( "same" );
		}
		

		string styleRef = config.getXString( hpath + ":style" );
		INFO( classname(), "Style Ref : " << styleRef );
		if ( config.exists( styleRef ) ){
			rpl.style( h ).set( config, styleRef );
		}

		rpl.style( h ).set( config, hpath + ".style" ).draw();

		TPaveStats *st = (TPaveStats*)h->FindObject("stats");
		if ( nullptr != st  ){
			INFO( classname(), "Found Stats" );
			positionOptStats( hpath, st );
			// st->SetX1NDC( 0.7 ); st->SetX2NDC( 0.975 );
		}

		INFO( classname(), "Drawing " << name );
		iHist++;
	}

	makeLegend( _path, histos );

} // makeHistograms

void VegaXmlPlotter::makeLegend( string _path, map<string, TH1*> &histos ){
	if ( config.exists( _path + ".Legend" ) ){
		INFO( classname(), "Legend" );

		TLegend * leg = new TLegend( 
			config.getDouble( _path + ".Legend.Position:x1", 0.1 ),
			config.getDouble( _path + ".Legend.Position:y1", 0.7 ),
			config.getDouble( _path + ".Legend.Position:x2", 0.5 ),
			config.getDouble( _path + ".Legend.Position:y2", 0.9 ) );

		if ( config.exists(  _path + ".Legend:title" ) ) {
			leg->SetHeader( config.getXString( _path + ".Legend:title" ).c_str() );
		}
		leg->SetNColumns( config.getInt( _path + ".Legend:columns" ) );

		vector<string> entries = config.childrenOf( _path + ".Legend", "Entry" );

		for ( string entryPath : entries ){
			INFO( classname(), "Entry @" << entryPath );
			if ( config.exists( entryPath + ":name" ) != true ) continue;
			string name = config.getXString( entryPath + ":name" );
			INFO( classname(), "Entry name=" << name );
			if ( histos.count( name ) <= 0 || histos[ name ] == nullptr ) continue;
			TH1 * h = histos[ name ];
			INFO( classname(), "Entry histo=" << h );
			string t = config.getString(  entryPath + ":title", name );
			string opt = config.getString(  entryPath + ":opt", "l" );

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

		leg->Draw( );

	}
}

void VegaXmlPlotter::makeLatex( string _path ){
	INFO( classname(), "_path=" << quote(_path) );
	
	TLatex latex;
	vector<string> latex_paths = config.childrenOf( _path, "TLatex", 1 );
	for ( string ltpath : latex_paths ){
		if ( !config.exists( ltpath + ":text" ) ) continue;

		string text = config.getXString( ltpath + ":text" );
		
		INFO( classname(), "Latex @ " << ltpath );
		latex.SetTextSize( config.getFloat( ltpath + ":size", 0.05 ) );

		TLatex * TL = latex.DrawLatexNDC( config.getFloat( ltpath + ":x" ), 
							config.getFloat( ltpath + ":y" ), 
							config.getXString( ltpath + ":text" ).c_str() );
	}
}

void VegaXmlPlotter::makeLine( string _path ){
	INFO( classname(), "_path=" << quote(_path) );

	vector<string> line_paths = config.childrenOf( _path, "TLine", 1 );
	for ( string ltpath : line_paths ){
		
		INFO( classname(), "Line (" << config.getDouble( ltpath + ":x1", 0.0 ) << ", " << config.getDouble( ltpath + ":y1", 0.0 ) << ") -> (" << config.getDouble( ltpath + ":x2", 0.0 ) << ", " << config.getDouble( ltpath + ":y2", 0.0 ) << ")" )
		TLine * line = new TLine( 
			config.getDouble( ltpath + ":x1", 0.0 ),
			config.getDouble( ltpath + ":y1", 0.0 ),
			config.getDouble( ltpath + ":x2", 0.0 ),
			config.getDouble( ltpath + ":y2", 0.0 ) );

		INFO( classname(), "TLine @ " << ltpath );
		
		// TODO: add string color support here
		line->SetLineColor( config.getInt( ltpath + ":color", 1 ) );
		line->SetLineWidth( config.getInt( ltpath + ":width", 1 ) );
		line->SetLineStyle( config.getInt( ltpath + ":style", 2 ) );

		line->Draw("same");
	}

}

void VegaXmlPlotter::makeExports( string _path ){
	vector<string> exp_paths = config.childrenOf( _path, "Export" );
	for ( string epath : exp_paths ){
		if ( !config.exists( epath + ":url" ) ) continue;

		string url = config.getXString( epath + ":url" );
		gPad->Print( url.c_str() );
		INFO( classname(), "Exporting gPad to " << url );
	}
}

TH1* VegaXmlPlotter::makeHistoFromDataTree( string _path, int iHist ){
	string data = config.getXString( _path + ":data" );
	TChain * chain = dataChains[ data ];
	string title = config.getXString( _path + ":title" );
	string hName = config.getXString( _path + ":name" ) +"_" + ts( iHist );
	string drawCmd = config.getXString( _path + ":draw" ) + " >> " + hName;
	string selectCmd = config.getXString( _path + ":select" );
	
	// if bins are given lets assume we need to make the histo first
	if ( config.exists( _path + ":bins_x" ) ){
		HistoBins bx( config, config.getXString( _path + ":bins_x" ) );
		HistoBins by( config, config.getXString( _path + ":bins_y" ) );
		HistoBins bz( config, config.getXString( _path + ":bins_z" ) );
		HistoBook::make( "D", hName, title, bx, by, bz );
	}


	INFO( classname(), "TTree->Draw( \"" << drawCmd << "\", \"" << selectCmd << "\" )" );
	chain->Draw( drawCmd.c_str(), selectCmd.c_str() );


	TH1 *h = (TH1*)gPad->GetPrimitive( hName.c_str() );
	globalHistos[ hName ] = h;

	return h;
}

void VegaXmlPlotter::positionOptStats( string _path, TPaveStats * st ){
	if ( nullptr == st ) return;

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

	return c;
}

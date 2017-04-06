#ifndef VEGA_XML_PLOTTER_H
#define VEGA_XML_PLOTTER_H

#include "TaskRunner.h"
#include "RooPlotLib.h"
#include "HistoBook.h"
#include "XmlPad.h"

using namespace jdb;

#include <map>
#include <algorithm>
#include <vector>
#include <memory>
#include <string>

using namespace std;

#include "TFile.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TChain.h"
#include "TPad.h"
#include "TPaveStats.h"
#include "TApplication.h"
#include "TColor.h"

class VegaXmlPlotter : public TaskRunner
{
public:
	virtual const char* classname() const { return "VegaXmlPlotter"; }
	VegaXmlPlotter() {}
	~VegaXmlPlotter() {}

	virtual void init();
	virtual void make();

	// shared_ptr<HistoBook> book;
	map<string, TFile*> dataFiles;
	map<string, TChain *> dataChains;
	map<string, TH1 * > globalHistos;

	TFile * dataOut = nullptr;
	virtual void loadDataFile( string _path );
	virtual int numberOfData();
	virtual void loadChain( string _path );
	virtual void loadData();


	virtual void makeOutputFile();
	virtual void makePlots();
	virtual void makePlot( string _path, TPad *_pad = nullptr );
	virtual void makePlotTemplates();
	virtual void makeMargins( string _path );
	

	virtual TH1* findHistogram( string _data, string _name );
	virtual TH1* findHistogram( string _path, int iHist, string _mod="" );
	virtual TH1* makeAxes(string _path);
	virtual map<string, TH1*> makeHistograms( string _path );
	virtual TH1* makeHistogram( string _path, string &fqn );
	virtual void makeLegend( string _path, map<string, TH1*> &histos );
	virtual void makeLatex( string _path );
	virtual void makeLine( string _path );
	virtual void makeExports( string _path, TPad * _pad = nullptr );
	// virtual void makeHistoStack( map<string, TH1*> histos );


	// Transforms
	virtual void makeTransforms();
	virtual void makeTransform( string _tpath );
	virtual void makeProjection( string _path );
	virtual void makeProjectionX( string _path);
	virtual void makeProjectionY( string _path);
	virtual void makeMultiAdd( string _path);
	virtual void makeAdd( string _path);
	virtual void makeDivide( string _path);
	virtual void makeRebin( string _path);
	virtual void makeScale( string _path);
	virtual void makeDraw( string _path);

	// Canvas based form
	virtual void makeCanvases();
	virtual void drawCanvas( string _path );


	virtual TH1* makeHistoFromDataTree( string _path, int iHist );

	virtual void positionOptStats( string _path, TPaveStats * st );

	virtual TCanvas* makeCanvas( string _path );

	int color( string _color ){
		if ( _color[0] == '#' && _color.size() == 7 ){
			return TColor::GetColor( _color.c_str() );
		}
		return atoi( _color.c_str() );
	}

	int getProjectionBin( string _path, TH1 * _h, string _axis="x", string _n="1", int _def = -1 ){
		
		if ( nullptr == _h ) return 1;


		TAxis *axis = _h->GetXaxis();
		if ( "y" == _axis || "Y" == _axis ) axis = _h->GetYaxis();
		if ( "z" == _axis || "Z" == _axis ) axis = _h->GetZaxis();
		
		string binAttr = _path + ":" + _axis + "b" + _n;
		string valAttr = _path + ":" + _axis + _n;

		int b = config.getInt( binAttr, _def );
		
		if ( config.exists( valAttr ) ){
			double v = config.getDouble( _path + ":y1", _def );
			b = axis->FindBin( v );
		}
		return b;
	}

	string nameOnly( string fqn ){
		return fqn.substr( fqn.find( "/" ) + 1 );
	} 
	string dataOnly( string fqn ){
		return fqn.substr( 0, fqn.find( "/" ) );
	} 
	string fullyQualifiedName( string _data, string _name ){
		if ( "" == _data )
			return _name;
		return _data + "/" + _name;
	}

protected:
	
};



#endif
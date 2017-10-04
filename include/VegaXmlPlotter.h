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
#include "TDirectory.h"

#if VEGADEBUG > 0
	#define DLOG( ... ) LOG_F(INFO, __VA_ARGS__)
	#define DSCOPE() LOG_SCOPE_FUNCTION(INFO)
#else
	#define DLOG( ... ) do {} while(0)
	#define DSCOPE() do {} while(0)
#endif

// Handlers
#include "TFMaker.h"

class VegaXmlPlotter : public TaskRunner
{
protected:
	TFMaker makerTF;
public:
	virtual const char* classname() const { return "VegaXmlPlotter"; }
	VegaXmlPlotter() {}
	~VegaXmlPlotter() {}

	virtual void init();
	virtual void make();

	virtual void inlineDataFile( string _path, TFile *f );

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

	// Handlers
	virtual void makeHandlers();
	virtual void makeTF( string _path );

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
	virtual void makeClone( string _path );
	virtual void makeSmooth( string _path );
	virtual void makeCDF( string _path );
	virtual void transformStyle( string _path );
	virtual void makeSetBinError( string _path );

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
		DLOG( "getProjectionBin( _path=%s, axis=%s, n=%s, default=%d", _path.c_str(), _axis.c_str(), _n.c_str(), _def );
		if ( nullptr == _h ) return 1;

		TAxis *axis = _h->GetXaxis();
		if ( "y" == _axis || "Y" == _axis ) axis = _h->GetYaxis();
		if ( "z" == _axis || "Z" == _axis ) axis = _h->GetZaxis();
		
		string binAttr = _path + ":" + _axis + "b" + _n;
		string valAttr = _path + ":" + _axis + _n;


		int b = config.getInt( binAttr, _def );
		DLOG( "Checking for bin value @ %s = %d (default=%d)", binAttr.c_str(), b, _def );
		if ( config.exists( valAttr ) ){
			double v = config.getDouble( valAttr, _def );
			b = axis->FindBin( v );
			DLOG( "Checking for user value @ %s = %f, bin=%d (default=%d)", valAttr.c_str(), v, b, _def );
		} else {
			DLOG( "bin values DNE @ %s", valAttr.c_str() );
		}
		return b;
	}

	bool axesRangeGiven( string _path, string _axis ){
		string n = "1";
		string binAttr = _path + ":" + _axis + "b" + n;
		string valAttr = _path + ":" + _axis + n;

		if ( false == config.exists( binAttr ) && false == config.exists( valAttr ) )
			return false;
		n = "2";
		binAttr = _path + ":" + _axis + "b" + n;
		valAttr = _path + ":" + _axis + n;
		if ( false == config.exists( binAttr ) && false == config.exists( valAttr ) )
			return false;
		return true;
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

	bool typeMatch( TObject *obj, string type );
	vector<string> glob( string query );
	map<string, TObject*> dirMap( TDirectory *dir, string prefix ="", bool dive = true );

	string underscape( string in ){
		std::replace( in.begin(), in.end(), '/', '_' );
		return in;
	}

	void setDefaultPalette();

protected:
	
};



#endif
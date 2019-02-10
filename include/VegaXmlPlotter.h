#ifndef VEGA_XML_PLOTTER_H
#define VEGA_XML_PLOTTER_H

#include "TaskRunner.h"
#include "RooPlotLib.h"
#include "HistoBook.h"
#include "XmlPad.h"
#include "XmlCanvas.h"

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
	bool initializedGROOT = false;

	typedef void (VegaXmlPlotter::*MFP)(string);
	std::map <string, MFP> handle_map;


	map<string, TH1*> histos;
	map<string, TGraph*> graphs;
	map<string, TF1*> funcs;
	TH1C * current_frame = nullptr;

	XmlCanvas * xcanvas;


public:
	virtual const char* classname() const { return "VegaXmlPlotter"; }
	VegaXmlPlotter() {}
	~VegaXmlPlotter() {}

	virtual void init();
	virtual void make();

	virtual void exec_node( string _path );
	virtual void exec_children( string _path );
	virtual void exec_children( string _path, string tag_type );
	virtual void exec_Loop( string _path );
	virtual void exec_TCanvas( string _path );
	virtual void exec_Data( string _path );
	virtual void exec_Plot( string _path );
	virtual void exec_Pad( string _path );
	virtual void exec_Axes( string _path );
	virtual void exec_Export( string _path );
	virtual void exec_ExportConfig( string _path );
	virtual void exec_StatBox( string _path );
	virtual void exec_Histo( string _path );
	virtual void exec_Graph( string _path );
	virtual void exec_TF1( string _path );
	virtual void exec_TLine( string _path );
	virtual void exec_Rect( string _path );
	virtual void exec_Ellipse( string _path );
	virtual void exec_TLatex( string _path );
	virtual void exec_TLegend( string _path );
	virtual void exec_Canvas( string _path );
	virtual void exec_Margins( string _path );
	virtual void exec_TFile( string _path );
	virtual void exec_Script( string _path );
	virtual void exec_Palette( string _path );

	// Transforms
	virtual void exec_Transforms( string _path );
	virtual void exec_transform_Projection( string _path );
	virtual void exec_transform_ProjectionX( string _path);
	virtual void exec_transform_ProjectionY( string _path);
	virtual void exec_transform_FitSlices( string _path);
	virtual void exec_transform_MultiAdd( string _path);
	virtual void exec_transform_Add( string _path);
	virtual void exec_transform_Divide( string _path);
	virtual void exec_transform_Rebin( string _path);
	virtual void exec_transform_Scale( string _path);
	virtual void exec_transform_Normalize( string _path);
	virtual void exec_transform_Draw( string _path);
	virtual void exec_transform_Clone( string _path );
	virtual void exec_transform_Smooth( string _path );
	virtual void exec_transform_CDF( string _path );
	virtual void exec_transform_Style( string _path );
	virtual void exec_transform_SetBinError( string _path );
	virtual void exec_transform_BinLabels( string _path );
	virtual void exec_transform_Sumw2( string _path );
	virtual void exec_transform_ProcessLine( string _path );
	virtual void exec_transform_Assign( string _path );
	virtual void exec_transform_Format( string _path );
	virtual void exec_transform_Print( string _path );
	virtual void exec_transform_Proof( string _path );
	virtual void exec_transform_List( string _path );


	virtual bool exec( string tag, string _path ){
		bool e = handle_map.count( tag ) > 0;
		if ( false == e ) return false;
		LOG_F( INFO, "exec( %s @ %s )", tag.c_str(), _path.c_str() );
		MFP fp = handle_map[ tag ];
		(this->*fp)( _path );
		return true;
	}


	string random_string( size_t length );

	virtual void inlineDataFile( string _path, TFile *f );

	// shared_ptr<HistoBook> book;
	map<string, TFile*> dataFiles;
	map<string, TChain *> dataChains;
	map<string, TH1 * > globalHistos;
	map<string, TGraph * > globalGraphs;

	TFile * dataOut = nullptr;
	virtual void loadDataFile( string _path );
	virtual int numberOfData();
	virtual void loadChain( string _path );
	virtual void loadData();

	virtual TObject* findObject( string _data );
	virtual TH1* findHistogram( string _data, string _name );
	virtual TH1* findHistogram( string _path, int iHist, string _mod="" );

	// virtual TH1* makeHistogram( string _path, string &fqn );
	// virtual TGraph* makeGraph( string _path, string &fqn );
	// virtual map<string, shared_ptr<TF1> > makeTF( string _path );

	// Transforms
	// virtual void makeTransforms();
	// virtual void makeTransform( string _tpath );
	// virtual void makeProjection( string _path );
	// virtual void makeProjectionX( string _path);
	// virtual void makeProjectionY( string _path);
	// virtual void makeMultiAdd( string _path);
	// virtual void makeAdd( string _path);
	// virtual void makeDivide( string _path);
	// virtual void makeRebin( string _path);
	// virtual void makeScale( string _path);
	// virtual void makeDraw( string _path);
	// virtual void makeClone( string _path );
	// virtual void makeSmooth( string _path );
	// virtual void makeCDF( string _path );
	// virtual void transformStyle( string _path );
	// virtual void makeSetBinError( string _path );
	// virtual void makeBinLabels( string _path );

	// Canvas based form
	// virtual void makeCanvases();
	// virtual void drawCanvas( string _path );


	virtual TH1* makeHistoFromDataTree( string _path, int iHist );

	virtual void positionOptStats( string _path, TPaveStats * st );

	// virtual TCanvas* makeCanvas( string _path );

	int color( string _color ){
		if ( _color[0] == '#' && _color.size() == 7 ){
			return TColor::GetColor( _color.c_str() );
		} else if ( _color[0] == '#' && _color.size() == 4 ){
			string colstr = "#";
			colstr += _color[1];
			colstr += _color[1];
			colstr += _color[2];
			colstr += _color[2];
			colstr += _color[3];
			colstr += _color[3];
			return TColor::GetColor( colstr.c_str() );
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
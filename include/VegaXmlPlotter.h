#ifndef VEGA_XML_PLOTTER_H
#define VEGA_XML_PLOTTER_H

#include "TaskRunner.h"
#include "RooPlotLib.h"
#include "HistoBook.h"

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
#include "TPaveStats.h"

class VegaXmlPlotter : public TaskRunner
{
public:
	virtual const char* classname() const { return "VegaXmlPlotter"; }
	VegaXmlPlotter() {}
	~VegaXmlPlotter() {}

	virtual void initialize();
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
	virtual void makePlot( string _path );
	virtual void makePlotTemplates();
	virtual void makeMargins( string _path );
	virtual TH1* findHistogram( string _path, int iHist );
	virtual map<string, TH1*> makeHistograms( string _path );
	virtual void makeLegend( string _path, map<string, TH1*> &histos );
	virtual void makeLatex( string _path );
	virtual void makeLine( string _path );
	virtual void makeExports( string _path );

	virtual TH1* makeHistoFromDataTree( string _path, int iHist );

	virtual void positionOptStats( string _path, TPaveStats * st );

	virtual TCanvas* makeCanvas( string _path );
	
};



#endif
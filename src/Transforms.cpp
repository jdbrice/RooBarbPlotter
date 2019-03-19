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

// #include "TBufferJSON.h"

#include <thread>

void VegaXmlPlotter::exec_Transforms( string _path ){
	DSCOPE();

	// hmm
	exec_Loop( _path );
}

void VegaXmlPlotter::exec_transform_SetBinError( string _path ){
	DSCOPE();
	
	TH1 * h = findHistogram( _path, 0 );
	TH2 *h2 = dynamic_cast<TH2*>(h);
	TH3 *h3 = dynamic_cast<TH3*>(h);

	if ( nullptr == h ) {
		LOG_F( ERROR, "Histogram is not valid for Transform @ %s", _path.c_str() );
		return;
	}

	float value = config.getFloat( _path + ":value", 0 );

	if ( nullptr != h3  ){
		for ( int i = 1; i <= h3->GetXaxis()->GetNbins(); i++ ){
			for ( int j = 1; j <= h3->GetYaxis()->GetNbins(); j++ ){
				for ( int k = 1; k <= h3->GetZaxis()->GetNbins(); k++ ){
					int b = h3->GetBin( i, j, k );
					h3->SetBinError( b, value );
				}
			}
		}	
	}
	if ( nullptr != h2  ){
		for ( int i = 1; i <= h2->GetXaxis()->GetNbins(); i++ ){
			for ( int j = 1; j <= h2->GetYaxis()->GetNbins(); j++ ){
				int b = h2->GetBin( i, j );
				h2->SetBinError( b, value );
			}
		}	
	}
	for ( int i = 1; i <= h->GetXaxis()->GetNbins(); i++ ){
		h->SetBinError( i, value );
	}
}

void VegaXmlPlotter::exec_transform_Projection( string _path ){
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
		LOG_F( ERROR, "NULL Histogram is not valid for Transform @ %s", _path.c_str() );
		return;
	}

	if ( nullptr != h3 ){
		LOG_F( INFO, "Projecting TH3" );
		if ( "x" == axis || "X" == axis ){
			LOG_F( INFO, "Projecting 1D onto %s Axis", axis.c_str() );
			int by1 = getProjectionBin( _path, h, "y", "1",  0 );
			int by2 = getProjectionBin( _path, h, "y", "2", -1 );

			int bz1 = getProjectionBin( _path, h, "z", "1",  0 );
			int bz2 = getProjectionBin( _path, h, "z", "2", -1 );

			TH1 * hNew = h3->ProjectionX( nn.c_str(), by1, by2, bz1, bz2 );
			globalHistos[ nn ] = hNew;
		} else if ( "y" == axis || "Y" == axis ){
			LOG_F( INFO, "Projecting 1D onto %s Axis", axis.c_str() );
			int bx1 = getProjectionBin( _path, h, "x", "1",  0 );
			int bx2 = getProjectionBin( _path, h, "x", "2", -1 );

			int bz1 = getProjectionBin( _path, h, "z", "1",  0 );
			int bz2 = getProjectionBin( _path, h, "z", "2", -1 );

			TH1 * hNew = h3->ProjectionY( nn.c_str(), bx1, bx2, bz1, bz2 );
			globalHistos[ nn ] = hNew;
		} else if ( "z" == axis || "Z" == axis ){
			LOG_F( INFO, "Projecting 1D onto %s Axis", axis.c_str() );
			int bx1 = getProjectionBin( _path, h, "x", "1",  0 );
			int bx2 = getProjectionBin( _path, h, "x", "2", -1 );

			int by1 = getProjectionBin( _path, h, "y", "1",  0 );
			int by2 = getProjectionBin( _path, h, "y", "2", -1 );
			LOG_F( INFO, "ProjectionZ : x(%d, %d), y : (%d, %d)", bx1, bx2, by1, by2 );
			TH1 * hNew = h3->ProjectionZ( nn.c_str(), bx1, bx2, by1, by2 );
			globalHistos[ nn ] = hNew;
		} else {
			// lets do a projection in 3D
			int bx1 = getProjectionBin( _path, h, "x", "1",  0 );
			int bx2 = getProjectionBin( _path, h, "x", "2", -1 );

			int by1 = getProjectionBin( _path, h, "y", "1",  0 );
			int by2 = getProjectionBin( _path, h, "y", "2", -1 );

			int bz1 = getProjectionBin( _path, h, "z", "1",  0 );
			int bz2 = getProjectionBin( _path, h, "z", "2", -1 );

			if ( axesRangeGiven( _path, "x" ) ){
				LOG_F( INFO, "x : (%d, %d)", bx1, bx2 );
				h3->GetXaxis()->SetRange( bx1, bx2 );
			} else 
				h3->GetXaxis()->SetRange( 1, -1 );
			if ( axesRangeGiven( _path, "y" ) ){
				LOG_F( INFO, "y : (%d, %d)", by1, by2 );
				h3->GetYaxis()->SetRange( by1, by2 );
			} else 
				h3->GetYaxis()->SetRange( 1, -1 );
			if ( axesRangeGiven( _path, "z" ) ){
				LOG_F( INFO, "z : (%d, %d)", bz1, bz2 );
				h3->GetZaxis()->SetRange( bz1, bz2 );
			} else 
				h3->GetZaxis()->SetRange( 1, -1 );
			
			TH1 * hNew = (TH1*)h3->Project3D( axis.c_str() );
			hNew->SetName( nn.c_str() );
			globalHistos[ nn ] = hNew;
		} // else 2D projection

	} else if ( nullptr != h2 ){
		if ( "x" == axis || "X" == axis )
			return exec_transform_ProjectionX( _path );
		else 
			return exec_transform_ProjectionY( _path );
	}
}

void VegaXmlPlotter::exec_transform_ProjectionX( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string nn = config.getXString( _path + ":save_as" );
	
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		return;
	}

	double y1 = -999;
	int b1 = config.getInt( _path + ":b1", 0 );
	if ( config.exists( _path + ":y1" ) ){
		y1 = config.getDouble( _path + ":y1", -1 );
		
		b1 = ((TH2*)h)->GetYaxis()->FindBin( y1 );
	}
	
	double y2 = -999;
	int b2 = config.getInt( _path + ":b2", -1 );
	if ( config.exists( _path + ":y2" ) ){
		y2 = config.getDouble( _path + ":y2", -1 );
		
		b2 = ((TH2*)h)->GetYaxis()->FindBin( y2 );
	}

	TH1 * hOther = ((TH2*)h)->ProjectionX( nn.c_str(), b1, b2 );
	h = hOther;

	globalHistos[ nn ] = h;
}

void VegaXmlPlotter::exec_transform_ProjectionY( string _path){
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
	double sx1 = std::numeric_limits<double>::quiet_NaN();
	double sx2 = std::numeric_limits<double>::quiet_NaN();

	int b1 = config.getInt( _path + ":b1", 0 );
	if ( config.exists( _path + ":x1" ) ){
		double x1 = config.getDouble( _path + ":x1", -1 );
		b1 = ((TH2*)h)->GetXaxis()->FindBin( x1 );
		sx1 = x1;
	}

	
	int b2 = config.getInt( _path + ":b2", -1 );
	if ( config.exists( _path + ":x2" ) ){
		double x2 = config.getDouble( _path + ":x2", -1 );
		b2 = ((TH2*)h)->GetXaxis()->FindBin( x2 );
		sx2 = x2;
	}

	if ( sx1 != std::numeric_limits<double>::quiet_NaN() && sx2 != std::numeric_limits<double>::quiet_NaN() ){
		LOG_F( INFO, "ProjectionX [ %s ] b=(%d, %d), x=(%f, %f)", nn.c_str(), b1, b2, sx1, sx2 );
	} else {
		LOG_F( INFO, "ProjectionX [ %s ] b=(%d, %d)", nn.c_str(), b1, b2 );
	}

	TH1 * hOther = ((TH2*)h)->ProjectionY( nn.c_str(), b1, b2 );
	h = hOther;

	globalHistos[ nn ] = h;
}

void VegaXmlPlotter::exec_transform_FitSlices( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( ERROR, "Must provide \"save_as\" attribute to save transformation" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "Could not find histogram" );
		return;
	}
	
	string nn = config.getXString( _path + ":save_as" );
	string axis = config.getString( _path + ":axis" );


	TObjArray aSlices;
	if ( "y" == axis ){
		LOG_F( INFO, "FitSlicesY" );
		((TH2*)h)->FitSlicesY(0, 0, -1, 0, "QRN", &aSlices);
	} else {
		LOG_F( INFO, "FitSlicesX" );
		((TH2*)h)->FitSlicesX(0, 0, -1, 0, "QRN", &aSlices);
	} 

	globalHistos[ (nn + "_0") ] = ((TH1*)aSlices[0]->Clone( (nn + "_0").c_str() ));
	globalHistos[ (nn + "_1") ] = ((TH1*)aSlices[1]->Clone( (nn + "_1").c_str() ));
	globalHistos[ (nn + "_2") ] = ((TH1*)aSlices[2]->Clone( (nn + "_2").c_str() ));
	globalHistos[ (nn + "_3") ] = ((TH1*)aSlices[3]->Clone( (nn + "_3").c_str() ));

	LOG_F( INFO, "Added %s_0, %s_1, %s_2, %s_3", nn.c_str(), nn.c_str(), nn.c_str(), nn.c_str() );
}

void VegaXmlPlotter::exec_transform_MultiAdd( string _path ){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}


	string d = config.getXString( _path + ":data" );
	vector<string> n = config.getStringVector( _path + ":name" );
	if ( n.size() < 1 ) {
		n = config.getStringVector( _path + ":names" );
	}

	string nn = config.getXString( _path + ":save_as" );

	// TODO allow modifier for each
	// double mod = config.getDouble( _path + ":mod", 1.0 );

	TH1 * hFirst = findHistogram( d, n[0] );
	if ( nullptr == hFirst ){
		LOG_F( ERROR, "Cannot find first histogram %s/%s", d.c_str(), n[0].c_str() );
		return;
	}
	TH1 * hSum = (TH1*)hFirst->Clone( nn.c_str() );


	LOG_F( INFO, "Adding %lu Histograms", n.size() );
	for ( int i = 1; i < n.size(); i++ ){
		TH1 * h = findHistogram( d, n[i] );
		if ( nullptr == h ) {
			LOG_F( WARNING, "Cannot add n=(%s), nullptr", n[i].c_str() );
			continue;
		}
		hSum ->Add( h );
	}

	globalHistos[nn] = hSum;
}

void VegaXmlPlotter::exec_transform_Add( string _path){
	DSCOPE();
    
	if ( !config.exists( _path + ":save_as" ) ){
		return;
	}

	if ( config.exists( _path+":names" ) && config.getStringVector( _path+":names" ).size() > 1 ){
		exec_transform_MultiAdd( _path );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string nn = config.getXString( _path + ":save_as" );
	double mod = config.getDouble( _path + ":mod", 1.0 );

	TH1 * hA = findHistogram( _path, 0, "A" );
	TH1 * hB = findHistogram( _path, 0, "B" );
	if ( nullptr == hA || nullptr == hB ) {
		return;
	}

    LOG_F( INFO, "%s = Add( %s, %s )", nn.c_str(), hA->GetName(), hB->GetName() );
	TH1 * hSum = (TH1*) hA->Clone( nn.c_str() );
	hSum->Add( hB, mod );
	globalHistos[nn] = hSum;
}

void VegaXmlPlotter::exec_transform_Divide( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		// ERRORC( "Must provide " << quote( "save_as" ) << " attribute to save transformation" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string nn = config.getXString( _path + ":save_as" );
	string opt = config.get<string>( _path +":opt", "" );
	float c1 = config.get<float>( _path +":c1", 1.0 );
	float c2 = config.get<float>( _path +":c2", 1.0 );

	TH1 * hNum = findHistogram( _path, 0, "A" );
	TH1 * hDen = findHistogram( _path, 0, "B" );
	if ( nullptr == hNum || nullptr == hDen ) {
		LOG_F( ERROR, "Numerator=%p, Denominator=%p, something is NULL", hNum, hDen );
		return;
	}

	LOG_F( INFO, "%s = %s / %s", nn.c_str(), hNum->GetName(), hDen->GetName() );
	TH1 * hOther = (TH1*) hNum->Clone( nn.c_str() );
	
	if ( opt != "" || c1 != 1.0 || c2 != 1.0 ){
		LOG_F( INFO, "Divide( num=%s, den=%s, c1=%f, c2=%f, opt=%s )", hNum->GetName(), hDen->GetName(), c1, c2, opt.c_str() );
		hOther->Divide( hNum, hDen, c1, c2, opt.c_str() );
	} else {
		LOG_F( INFO, "Divide( num=%s, den=%s )", hNum->GetName(), hDen->GetName() );
		hOther->Divide( hDen );
	}
	globalHistos[nn] = hOther;
}

void VegaXmlPlotter::exec_transform_Difference( string _path){
	DSCOPE();
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( ERROR, "must have a save_as attribute" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string nn = config.getXString( _path + ":save_as" );

	bool rel  = config.exists( _path +":relative" ) | config.exists( _path +":rel" );
	bool absv = config.exists( _path +":absolute" ) | config.exists( _path +":abs" );

	TH1 * hA = findHistogram( _path, 0, "A" );
	TH1 * hB = findHistogram( _path, 0, "B" );
	if ( nullptr == hA || nullptr == hB ) {
		LOG_F( ERROR, "hA=%p, hB=%p, something is NULL", hA, hB );
		return;
	}


	if ( !rel && !absv ){
		LOG_F( INFO, "%s = %s - %s", nn.c_str(), hA->GetName(), hB->GetName() );
	} else if ( rel && !absv){
		LOG_F( INFO, "%s = (%s - %s) / %s", nn.c_str(), hA->GetName(), hB->GetName(), hA->GetName() );
	} else if ( !rel && absv ){
		LOG_F( INFO, "%s = |%s - %s|", nn.c_str(), hA->GetName(), hB->GetName() );
	} else if ( rel && absv){
		LOG_F( INFO, "%s = |%s - %s| / %s", nn.c_str(), hA->GetName(), hB->GetName(), hA->GetName() );
	}
	TH1 * hOther = (TH1*) hA->Clone( nn.c_str() );
	hOther->Reset(); // we just want the binning
	// TODO, support 2D, 3D explicitly if needed
	LOG_IF_F(ERROR, hA->GetNbinsX() != hB->GetNbinsX(), "Histograms must have the same number of bins! Cannot make difference histogram" );
	for ( size_t i = 1; i <= hA->GetNbinsX(); i++ ){
		float vA = hA->GetBinContent( i );
		float vB = hB->GetBinContent( i );

		float eA = hA->GetBinError( i );
		float eB = hB->GetBinError( i );

		float value = vA - vB;
		float error = eA + eB;

		if ( absv ){
			value = abs( value );
		}

		if ( rel  && abs(vA) > 0){
			value = value / vA;
			error = error / vA;
		}

		hOther->SetBinContent( i, value );
		hOther->SetBinError( i, error );

	}

	globalHistos[nn] = hOther;
}



void VegaXmlPlotter::exec_transform_Rebin( string _path ){
	DSCOPE();
	
	// rebins to array of bin edges
	bool in_place = false;
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( WARNING, "Rebin does not support in-place!" );
		in_place = true;
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		// ERRORC( "Could not find histogram " << quote( d + "/" + n ) );
		LOG_F( ERROR, "Could not find histogram @ %s, name=%s", _path.c_str(), n.c_str() );
		return;
	}


	string nn = config.getXString( _path + ":save_as" );
	TH1 * hOther = nullptr;//((TH1*)h)->Clone( nn.c_str() );

	HistoBins bx, by, bz;
	if ( config.exists( _path +":bins_x" )  ){
		if ( config.exists( config.getXString( _path+":bins_x" ) ) ){
			bx.load( config, config.getXString( _path+":bins_x" ) );
		} else {
			bx.load( config, _path + ":bins_x" );
		}
		if ( 0 == bx.nBins() ){
			LOG_F( WARNING, "Cannot make x bins" );
		}
	}
	if ( config.exists( _path +":bins_y" )  ){
		if ( config.exists( config.getXString( _path+":bins_y" ) ) ){
			by.load( config, config.getXString( _path+":bins_y" ) );
		} else {
			by.load( config, _path + ":bins_y" );
		}
	}
	if ( config.exists( _path +":bins_z" )  ){
		if ( config.exists( config.getXString( _path+":bins_z" ) ) ){
			bz.load( config, config.getXString( _path+":bins_z" ) );
		} else {
			bz.load( config, _path + ":bins_z" );
		}
	}

	int nDim = 0;
	if ( bx.nBins() > 0 ) nDim++;
	if ( by.nBins() > 0 ) nDim++;
	if ( bz.nBins() > 0 ) nDim++;

	DLOG( "nDim=%d", nDim );


	if ( nDim == 1 && bx.nBins() > 0 ){
		LOG_F( INFO, "Rebin1D of %s -> %s  using bins %s", n.c_str(), nn.c_str(), bx.toString().c_str() );
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
			LOG_F( ERROR, "Input histogram is not 2D" );
		} else {
			LOG_F( ERROR, "Cannot Rebin, check error for bins x and y" );
		}
	}

	if ( nDim == 3 && bx.nBins() > 0 && by.nBins() > 0 && bz.nBins() > 0 && nullptr != dynamic_cast<TH3*>( h ) ){
		hOther = new TH3D( nn.c_str(), h->GetTitle(), bx.nBins(), bx.bins.data(), by.nBins(), by.bins.data(), bz.nBins(), bz.bins.data() );
		HistoBins::rebin3D( dynamic_cast<TH3*>(h), dynamic_cast<TH3*>(hOther) );
		globalHistos[nn] = hOther;
	} else if ( nDim == 3 ){
		if ( nullptr == dynamic_cast<TH2*>( h ) ){
			LOG_F( ERROR, "Input histogram is not 3D" );
		} else {
			LOG_F( ERROR, "Cannot Rebin, check error for bins x, y, z" );
		}
	}
}

void VegaXmlPlotter::exec_transform_Scale( string _path ){
	DSCOPE();

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string fqn = fullyQualifiedName( d, n );
	bool inplace = false;
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( INFO, "Scaling in place %s", fqn.c_str() );
		inplace = true;
	}

	
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		// ERRORC( "Could not find histogram " << quote( d + "/" + n ) );
		return;
	}
	string nn = config.getXString( _path + ":save_as" );

	TH1 * hOther = h;
	
	if ( false == inplace  )
		hOther = (TH1*)h->Clone( nn.c_str() );

	double factor = config.getDouble( _path + ":factor", 1.0 );
	string opt    = config.getXString( _path +":opt", "" );

	LOG_F( INFO, "Scale( %0.3f, %s )", factor, opt.c_str() );
	hOther->Scale( factor, opt.c_str() );

	if ( false == inplace )
		globalHistos[nn] = hOther;
}

void VegaXmlPlotter::exec_transform_Normalize( string _path ){
	DSCOPE();
	// TODO:
	// add option to normalize in range given by x1, x2 or b1, b2 etc.

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );
	string fqn = fullyQualifiedName( d, n );
	bool inplace = false;
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( INFO, "Normalizing in place %s", fqn.c_str() );
		inplace = true;
	}
	
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "Could not find histogram %s", quote( d + "/" + n ).c_str() );
		return;
	}
	string nn = config.getXString( _path + ":save_as" );

	TH1 * hOther = h;
	
	if ( false == inplace  )
		hOther = (TH1*)h->Clone( nn.c_str() );

	int bx1 = getProjectionBin( _path, h, "x", "1",  1 );
	int bx2 = getProjectionBin( _path, h, "x", "2", -1 );

	if ( config.exists( _path + ":b1" ) )
		bx1 = config.getInt( _path + ":b1" );
	if ( config.exists( _path + ":b2" ) )
		bx2 = config.getInt( _path + ":b2" );

	LOG_F( INFO, "Normalizing from bin %d to %d", bx1, bx2 );
	hOther->Scale( 1.0 / h->Integral( bx1, bx2) );

	globalHistos[nn] = hOther;
}

void VegaXmlPlotter::exec_transform_Draw( string _path ){
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

void VegaXmlPlotter::exec_transform_Smooth( string _path ){
	DSCOPE();

	bool in_place = false;
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( WARNING, "Smoothing histogram in place!" );
		in_place = true;
	}

	string d = config.getString( _path + ":data" );
	string n = config.getString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "could not find histo %s %s", d.c_str(), n.c_str() );
		return;
	}

	int nSmooth = config.get<int>( _path + ":n", 1 );
	
	if ( in_place ){
		LOG_F( INFO, "Smoothing histogram %d times", nSmooth );
		h->Smooth( nSmooth );
	} else {
		string nn = config.getString( _path + ":save_as" );
		TH1 * hOther = (TH1*)h->Clone( nn.c_str() );
		hOther->Smooth( nSmooth );
		LOG_F( INFO, "Smoothing histogram %s %d times", nn.c_str(), nSmooth );
		globalHistos[nn] = hOther;
	}
}

void VegaXmlPlotter::exec_transform_CDF( string _path ){
	DSCOPE();

	
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( WARNING, "Cannot make CDF in place, abort" );
		return;
	}

	string d = config.getString( _path + ":data" );
	string n = config.getString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "could not find histo %s %s", d.c_str(), n.c_str() );
		return;
	}

	LOG_F( INFO, "I(histo) = %f", h->Integral() );
	bool forward = config.get<bool>( _path + ":forward", false );
	
	string nn = config.getString( _path + ":save_as" );
	TH1 * hOther = (TH1*)h->GetCumulative( forward )->Clone( nn.c_str() );
	LOG_F( INFO, "Made CDF for histogram %s, with forward=%s", nn.c_str(), bts(forward).c_str() );
	globalHistos[nn] = hOther;
}

void VegaXmlPlotter::exec_transform_BinLabels( string _path ){
	LOG_F( INFO, "" );
	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( WARNING, "Cannot make CDF in place, abort" );
		return;
	}

	string d = config.getString( _path + ":data" );
	string n = config.getString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "could not find histo %s %s", d.c_str(), n.c_str() );
		return;
	}
	
	string nn = config.getString( _path + ":save_as" );
	TH1 * hOther = (TH1*)h->Clone( nn.c_str() );

	vector<string> labels = config.getStringVector( _path + ":labels" );
	TAxis *axis = hOther->GetXaxis();

	for ( size_t i = 0; i < labels.size(); i++ ){
		axis->SetBinLabel( i+1, labels[i].c_str() );
		LOG_F( INFO, "Seeting bin[%lu] label to \"%s\" ", i+1, labels[i].c_str() );
	}
	// LOG_F( INFO, "Made CDF for histogram %s, with forward=%s", nn.c_str(), bts(forward).c_str() );
	globalHistos[nn] = hOther;
}

void VegaXmlPlotter::exec_transform_Clone( string _path ){

	if ( !config.exists( _path + ":save_as" ) ){
		LOG_F( ERROR, "<Clone/> Must have a save_as attribute" );
		return;
	}

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );

	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "could not find histo %s %s", d.c_str(), n.c_str() );
		return;
	}

	config.set( "uname", underscape(n) );

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

void VegaXmlPlotter::exec_transform_Style( string _path ){
	DSCOPE();
	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		return;
	}

	RooPlotLib rpl;

	DLOG( "settign style from: %s, and %s", (_path + ":style").c_str(), (_path + ".style").c_str() );
	rpl.style( h ).set( config, _path).set( config, _path + ":style" ).set( config, _path + ".style" );
}

void VegaXmlPlotter::exec_transform_Sumw2( string _path ){

	string d = config.getXString( _path + ":data" );
	string n = config.getXString( _path + ":name" );

	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr == h ) {
		LOG_F( ERROR, "could not find histogram @ %s", _path.c_str() );
		return;
	}
	config.set( "uname", underscape(n) );
	TH1 * hOther = h;
	string nn = nameOnly( n );

	if ( config.exists( _path + ":save_as" ) ){
		nn = config.getXString( _path + ":save_as" );
		hOther = (TH1*)h->Clone( nn.c_str() );
	} else {
		LOG_F( INFO, "Sumw2 in place" );
	}
	LOG_F( INFO, "Calling Sumw2()" );
	hOther->Sumw2();
	globalHistos[nn] = hOther;
}

void VegaXmlPlotter::exec_transform_Assign( string _path ){
	DSCOPE();

	string varname = config.getString( _path + ":var" );
	

	string d = config.getString( _path + ":data" );
	string n = config.getString( _path + ":name" );
	TH1 * h = findHistogram( _path, 0 );

	

	string expr = config.getString( _path + ":expr" );

	LOG_F( INFO, "gROOT->ProcessLine( \"%s\" )", expr.c_str() );
	
	// TNamed *tmp = new TNamed( "vestibule", "tmp storage"  );
	// gDirectory->Add( tmp );
	
	
	int error = 0;
	// LOG_F( INFO, "gDirectory=%p, gROOT=%p", gDirectory, gROOT );

	// Super crazy shit
	// first include sstr header (if not done already)
	// make a stringstream for conversion to char*
	// Get the TNamed object for passing data
	// store result of expression in sstr
	// store result in the title of the TNamed
	// 
	if ( false == initializedGROOT ){
		gROOT->ProcessLine( "#include \"sstream\" " );
		initializedGROOT = true;
		gROOT->ProcessLine( "std::stringstream sstr;" );
		gROOT->ProcessLine( ("TNamed * tn = 0;") );
		gROOT->ProcessLine( "TH1 * h = 0;" ) ;
	}


	if ( nullptr != h ){
		config.set( "name", h->GetName() );
		LOG_F( INFO, "Histogram %s available using as h or {name}", h->GetName() );
		LOG_F( INFO, "h = %s", h->GetName() );
		gROOT->ProcessLine( ( string("h = ") + h->GetName()).c_str() );
		
	}

	gROOT->ProcessLine( "sstr.str(\"\");" );
	// gROOT->ProcessLine( "gDirectory" );
	gROOT->ProcessLine( ("tn = new TNamed( \"" + varname + "\", \"tmp\" );").c_str() ) ;
	gROOT->ProcessLine( "gDirectory->Add( tn );" );
	// gROOT->ProcessLine( expr.c_str() );
	// gROOT->ProcessLine( "cout << sstr.str() << endl;" );
	// gROOT->ProcessLine( ("cout << " + expr + " << endl;").c_str() );
	expr = "sstr << " + expr + ";";
	gROOT->ProcessLine( expr.c_str() );
	gROOT->ProcessLine( "tn->SetTitle( sstr.str().c_str() );" );
	
	TNamed * tmp = ((TNamed*)gROOT->FindObject( varname.c_str() ));
	if ( nullptr == tmp ){
		LOG_F( ERROR, "Failed to memory map" );
		return;
	}
	LOG_F( INFO, "tn->GetTitle()=%s", tmp->GetTitle() );
	
	// 
	LOG_F( INFO, "ERROR = %d", error );
	LOG_F( INFO, "TNamed.title = %s", tmp->GetTitle() );
	LOG_F( INFO, "ASSIGN [%s] = [%s]", varname.c_str(), tmp->GetTitle() );
	config.set( varname, string(tmp->GetTitle()) );

	delete tmp;

}

void VegaXmlPlotter::exec_transform_Print( string _path ){
	string msg = config.getString( _path + ":msg" );
	LOG_F(INFO, "%s", msg.c_str());
}

void VegaXmlPlotter::exec_transform_Format( string _path ){
	DSCOPE();

	// where to store the result
	string varname = config.getString( _path + ":var", config.getString( _path + ":name" ) );
	string templ = config.getString( _path + ":form",  config.getString( _path + ":format",  config.getString( _path + ":template") ) );
	string input = config.getString( _path + ":expr" );

	string expr = "TString::Format(\"" + templ + "\", " + input + " )";
	LOG_F( INFO, "Format( \"%s\", \"%s\" ) ==> %s", templ.c_str(), input.c_str(), varname.c_str() );
	LOG_F( INFO, "%s", expr.c_str() );
	
	
	
	
	int error = 0;
	// LOG_F( INFO, "gDirectory=%p, gROOT=%p", gDirectory, gROOT );

	// Super crazy shit
	// first include sstr header (if not done already)
	// make a stringstream for conversion to char*
	// Get the TNamed object for passing data
	// store result of expression in sstr
	// store result in the title of the TNamed
	// 
	if ( false == initializedGROOT ){
		gROOT->ProcessLine( "#include \"sstream\" " );
		initializedGROOT = true;
		gROOT->ProcessLine( "std::stringstream sstr;" );
		gROOT->ProcessLine( ("TNamed * tn = 0;") );
	}

	gROOT->ProcessLine( "sstr.str(\"\");" );
	gROOT->ProcessLine( ("tn = new TNamed( \"" + varname + "\", \"tmp\" );").c_str() ) ;
	gROOT->ProcessLine( "gDirectory->Add( tn );" );
	
	expr = "sstr << " + expr + ";";
	gROOT->ProcessLine( expr.c_str() );
	gROOT->ProcessLine( "tn->SetTitle( sstr.str().c_str() );" );
	
	TNamed * tmp = ((TNamed*)gROOT->FindObject( varname.c_str() ));
	if ( nullptr == tmp ){
		LOG_F( ERROR, "Failed to memory map" );
		return;
	}
	LOG_F( INFO, "tn->GetTitle()=%s", tmp->GetTitle() );
	
	// 
	LOG_F( INFO, "ERROR = %d", error );
	LOG_F( INFO, "TNamed.title = %s", tmp->GetTitle() );
	LOG_F( INFO, "ASSIGN [%s] = [%s]", varname.c_str(), tmp->GetTitle() );
	config.set( varname, string(tmp->GetTitle()) );

	delete tmp;

}

void VegaXmlPlotter::exec_transform_ProcessLine( string _path ){

	TH1 * h = findHistogram( _path, 0 );
	if ( nullptr != h ){
		config.set( "name", h->GetName() );
		LOG_F( INFO, "Histogram %s available using {name}", h->GetName() );
	}

	string expr = config.getString( _path + ":expr" );
	LOG_F( INFO, "gROOT->ProcessLine( \"%s\" )", expr.c_str() );
	gROOT->ProcessLine( expr.c_str() );

}

void VegaXmlPlotter::exec_transform_Proof( string _path ){

	bool setup = config.get<bool>( _path + ":setup", true );
	string data = config.get<string>( _path + ":data" );
	bool on = config.get<bool>( _path + ":on", true );

	if ( setup )
		gROOT->ProcessLine( "TProof::Open( \"\" );" );

	if ( dataChains.count( data ) >= 0 ){
		dataChains[data]->SetProof( on );
	}

}

void VegaXmlPlotter::exec_transform_List( string _path ){

	string data = config.get<string>( _path + ":data" );

	if ( dataFiles.count( data ) > 0 ){
		LOG_F( INFO, "Listing data file: %s", data.c_str() );
		dataFiles[ data ]->ls();
	} else {
		LOG_F( WARNING, "Data file name=%s NOT FOUND", data.c_str() );
	}
}
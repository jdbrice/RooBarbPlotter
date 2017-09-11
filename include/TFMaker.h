#ifndef TF_MAKER_H
#define TF_MAKER_H

// RooBarb
#include "XmlConfig.h"
#include "XmlFunction.h"
#include "RooPlotLib.h"
using namespace jdb;

// STL
#include <string>

// ROOT
#include "TF1.h"

// Project
#include "loguru.h"

class TFMaker {
    XmlFunction xf;
public:
    void make( XmlConfig &config, string _path ){
        LOG_F( INFO, "Making TF1 at _path=%s", _path.c_str() );
        
        RooPlotLib rpl;
        xf.set( config, _path );
        LOG_F( INFO, "TF1=%s", xf.toString().c_str() );
        rpl.style( xf.getTF1().get() ).set( config, _path + ":style" ).set( config, _path + ".style" ).draw( "same" );
    }
protected:
};

#endif
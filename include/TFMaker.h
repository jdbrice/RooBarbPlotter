#ifndef TF_MAKER_H
#define TF_MAKER_H

// RooBarb
#include "XmlConfig.h"
#include "XmlFunction.h"
#include "RooPlotLib.h"
using namespace jdb;

// STL
#include <string>
#include <vector>

// ROOT
#include "TF1.h"

// Project
#include "loguru.h"

class TFMaker {
    vector<XmlFunction> functions;
public:
    shared_ptr<TF1>  make( XmlConfig &config, string _path ){
        LOG_F( INFO, "Making TF1 at _path=%s", _path.c_str() );
        if ( config.exists( _path + ":formula" ) == false ){
            return nullptr;
        }
        XmlFunction xf;
        RooPlotLib rpl;
        xf.set( config, _path );
        LOG_F( INFO, "TF1=%s", xf.toString().c_str() );
        rpl.style( xf.getTF1().get() ).set( config, _path ).set( config, _path + ":style" ).set( config, _path + ".style" ).draw( "same" );

        functions.push_back( xf );

        return xf.getTF1();
    }
protected:
};

#endif
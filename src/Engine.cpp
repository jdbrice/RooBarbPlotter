

// RooBarb
#include "XmlConfig.h"
#include "TaskEngine.h"
using namespace jdb;

#include "VegaXmlPlotter.h"



int main( int argc, char* argv[] ) {

	TaskFactory::registerTaskRunner<VegaXmlPlotter>( "VegaXmlPlotter" );

	TaskEngine engine( argc, argv );

	return 0;
}



// RooBarb
#include "XmlConfig.h"
#include "TaskEngine.h"
using namespace jdb;

#include "VegaXmlPlotter.h"

#include "loguru.h"


int main( int argc, char* argv[] ) {

	loguru::init(argc, argv);
	loguru::add_file("VegaXmlPlotter.log", loguru::Truncate, loguru::Verbosity_MAX);

	TaskFactory::registerTaskRunner<VegaXmlPlotter>( "VegaXmlPlotter" );

	TaskEngine engine( argc, argv, "VegaXmlPlotter" );

	return 0;
}

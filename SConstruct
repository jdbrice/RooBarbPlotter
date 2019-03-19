#!python
import subprocess
import os

SConscript('color_SConscript')
# SConscript( 'lib/XmlConfig/Sconstruct' )
# SConscript( 'lib/RooPlotLib/Sconstruct' )
Import( 'env' )

ROOTCFLAGS    	= subprocess.check_output( ['root-config',  '--cflags'] ).rstrip().decode( "utf-8" ).split( " " )
ROOTLDFLAGS    	= subprocess.check_output( ["root-config",  "--ldflags"] ).decode( "utf-8" )
ROOTLIBS      	= subprocess.check_output( ["root-config",  "--libs"] ).decode( "utf-8" )
ROOTGLIBS     	= subprocess.check_output( ["root-config",  "--glibs"] ).decode( "utf-8" )
ROOTLIBPATH 	= subprocess.check_output( ["root-config", "--libdir" ] ).decode( "utf-8" )
ROOT_SYS 		= os.environ[ "ROOTSYS" ]

cppDefines 		= {}
cppFlags 		= ['-Wall' ]#, '-Werror']
cxxFlags 		= ['-std=c++11', '-O3' ]
cxxFlags.extend( ROOTCFLAGS )

paths 			= [ '.', 			# dont really like this but ended up needing for root dict to work ok
					'include', 
					ROOT_SYS + "/include",
					'/usr/local/include/XmlConfig',
					'/usr/local/include/RooPlotLib',
					'/usr/local/include/TaskEngine',
					'/usr/local/include/RootAna'
					]
# paths.extend( Glob( "include/*" ) )


########################## Project Target #####################################
common_env = env.Clone()
common_env.Append(CPPDEFINES 	= cppDefines)
common_env.Append(CPPFLAGS 		= cppFlags)
common_env.Append(CXXFLAGS 		= cxxFlags)
common_env.Append(LINKFLAGS 	= cxxFlags ) #ROOTLIBS + " " + JDB_LIB + "/lib/libJDB.a"
common_env.Append(CPPPATH		= paths)
common_env.Append(LIBS 			= [ "libXmlConfig.a", "libRooPlotLib.a", "libTaskEngine.a", "libRootAna.a" ] )
common_env.Append(LIBPATH 		= [ "/usr/local/lib" ] )

common_env[ "_LIBFLAGS" ] = common_env[ "_LIBFLAGS" ] + " " + ROOTLIBS + " " 


jdb_log_level = ARGUMENTS.get( "ll", 10 )
vega_debug = ARGUMENTS.get( "debug", 0 )
common_env.Append(CXXFLAGS 		= "-DJDB_LOG_LEVEL=" + str(jdb_log_level) )

json_export = ARGUMENTS.get( "json", 1 )

if int(json_export) > 0 :
	print( "Compiling with support for JSON" )
	common_env.Append(LIBS = [ "libRIO" ] )
	common_env.Append(CXXFLAGS 		= "-DJSON_EXPORT=1" )

#print "DEBUG ", vega_debug
if int(vega_debug) > 0 :
	#print "DEBUG ENABLED"
	common_env.Append(CXXFLAGS 		= "-DVEGADEBUG=1" )

target = common_env.Program( target='bin/rbp', source=[Glob( "src/*.cpp" )] )

# Depends( target, Glob( JDB_LIB + "/include/jdb/*" ) )

# set as the default target
Default( target )



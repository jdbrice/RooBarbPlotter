RooBarbPlotter
---

Use XML to declaratively produce plots from ROOT data

## Drawing 

## Transformations
### Add
```xml
<Add save_as="hout" nameA="ha" nameB="hb" mod="1" />
```
Adds `ha` and `hb` together into `hout`  
OR for adding multiple histograms together:
```xml
<Add save_as="hout" names="ha, hb, hc" mod="1" />
Adds `ha`, `hb`, and `hc` into `hout`  
```

### Clone 
```xml
<Add save_as="hout" data="d" name="ha" />
```
Clones histogram `ha` from data `d` into `hout`  
### Draw histogram from TTree
```xml
<Draw save_as="hout" name="hout" data="tree_data_name" draw="y : x" select="sqrt( x*x + y*y ) > 0.1" 
      norm="true|false" opt="" bins_x="" bins_y="" bins_z="" N="{nEvents}" />
```



	virtual void makeProjection( string _path );
	virtual void makeProjectionX( string _path);
	virtual void makeProjectionY( string _path);
	
	virtual void makeDivide( string _path);
	virtual void makeRebin( string _path);
	virtual void makeScale( string _path);
	virtual void makeDraw( string _path);
	virtual void makeClone( string _path );
	virtual void makeSmooth( string _path );
	virtual void makeCDF( string _path );
	virtual void transformStyle( string _path );
	virtual void makeSetBinError( string _path );
	virtual void makeBinLabels( string _path );
	virtual void makeSumw2( string _path );




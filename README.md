RooBarbPlotter
---

Use XML to declaratively produce plots from ROOT data
## DEMO - styling anf plotting 

```xml
<?xml version="1.0" encoding="UTF-8"?>
<config>
	<!-- List your data sources -->
	<!-- Can be files or trees, later access them by name -->
	<Data name="sample" url="sample_data.root" />

	<!-- For standard canvas plots 
		width/w		width of canvas in px
		height/h	height of canvas in px
	-->
	<TCanvas width="3000" height="2400" />

	<!-- Set the margins for the global TCanvas -->
	<Margins> 0.03, 0.03, 0.12, 0.10</Margins>
	<!-- An array (top, right, bottom, lleft) -->
	<!-- can also be set individually as attributes top, right, bottom, left -->


	<!-- Basic Plot -->
	<Plot>
		<!-- Axes lsx/lsy		linspace x/y (min, max, nbins) -->
		<!-- Axis are option. Histograms will draw their own axes if "same" opt is not used -->
		<Axes lsx="-5, 50, 20" lsy="5e1, 7e4, 1" x-title="x (units)" y-tit="y (units)" />
		<StatBox v="0" />

		<!-- Draw Latex 
			x/y 	NDC coords
			ux/uy 	User coords (i.e. on axis coords)
			text 	Text to be displayed, can contain Latex
			size 	Raw size of text - in ROOT units (bleh)
			point 	Scaled text size, 12 point is reasonale size, 64 is large etc.
			font 	Font to use, ROOT int index
		-->
		<TLatex x="0.20" y="0.87" text="Dataset details here" point="18"/>
		
		
		<!-- Draw Histograms 
			data the name of the data file defined above
			name The name of the histogram within the data file. Can be path to object if within sub directories
			draw the draw options to apply

			Styling can be applied inline or referenced See full list of styling options 
		-->
		<Histo data="sample" name="h1" style="styles.TH1" draw="same" logy="1" />
		<Histo data="sample" name="h2" style="styles.TH1" draw="same " linecolor="blue" fca="#00F, 0.2" lw="0"/>
		<Histo data="sample" name="hSum" draw="same " linewidth="6" linecolor="black" />

		
		<!-- TLine
			endpoints:	
				x1,y1		starting point (x1,y1)
				x2,y2		ending point (x2, y2)
				OR
				p1/p2 		p1=(x1,y1), p2=(x2,y2)
				OR
				x/y			x=(x1,x2), y=(y1,y2)
			style		root line style
			width 		line width
			color		color - can use hex codes 3 or 6 digit
		-->
		<TLine  p1="5, 50" p2="5, 2e4" style="2" width="3" color="#000" />


		<!-- Draw a Legend with a transparent back and no border-->
		<Legend title="Components" fill-style="0" border-width="0">

			<!-- simple alignment -->
			<Position align="top right" />

			<!-- Specify entries by name, give the title and options -->
			<Entry name="h1" title="gaussian" opt="lpe" line-width="4" />
			<Entry name="h2" title="Landau" opt="fpe" />
			<!-- if you need to you can re-style the markers used in the legend -->
			<Entry name="hSum" title="Sum" opt="l" line-width="4"/> 
		</Legend>

		<!-- Export the canvas in various formats -->
		<Export url="demo.png" />
		<Export url="demo.pdf" />

		<!-- requires compiling with libRIO -->
		<Export url="demo.json" />
	</Plot>


	<!-- These styles can be easily re-used by reference -->
	<styles>
		<TH1 title="basic TH1 style; X [units]; Y [units]" color="red"/>
	</styles>


</config>
```

produces:

![](tests/demo.png)


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

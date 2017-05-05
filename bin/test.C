jdb::XmlConfig xfg( "test.xml" )
 jdb::XmlCanvas can( xfg, "Canvas" )
 can.activatePad("1")
TH1 * h = new TH1D( "H", "H", 100, -10, 10 );
h->FillRandom("gaus", 10000 );
gStyle->SetOptStat(0);
h->DrawClone();

can.activatePad("2")
h->DrawClone();

can.activatePad("3")
h->DrawClone();


can.activatePad("4")
h->DrawClone();






make_sample_data(){

    TFile *f = new TFile( "sample_data.root", "RECREATE" );

    TH1 * h1 = new TH1F( "h1", "h1;X;Y", 200, -10, 10 );
    h1->FillRandom( "gaus", 10000 );

    TH1 * h2 = new TH1F( "h2", "h2;X;Y", 200, -10, 10 );
    h2->FillRandom( "landau", 10000 );

    TF2 *f2 = new TF2("f2","[0]*TMath::Gaus(x,[1],[2])*TMath::Gaus(y,[3],[4])",0,10,0,10); 
    f2->SetParameters(1,3,1,4,1);

    TH2 * h3 = new TH2F( "h3", "h3;X;Y", 200, -10, 10, 200, -10, 10 );
    h3->FillRandom( "f2", 10000 );


    f->Write();

}
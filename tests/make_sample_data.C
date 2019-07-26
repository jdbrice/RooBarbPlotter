



void make_sample_data(){

    TFile *f = new TFile( "sample_data.root", "RECREATE" );

    TH1 * h1 = new TH1F( "h1", "h1;X;Y", 1000, -50, 50 );
    TF1 * fg = new TF1( "fg", "[0]*exp(-0.5*((x-[1])/[2])^2)/([2] *sqrt(2*3.14159))", -50, 50 );
    fg->Draw();
    fg->SetParameter( 0, 1 );
    fg->SetParameter( 1, 5 );
    fg->SetParameter( 2, 3 );
    h1->FillRandom( "fg", 1000000 );

    TH1 * h2 = new TH1F( "h2", "h2;X;Y", 1000, -50, 50 );
    h2->FillRandom( "landau", 1000000 );

    TH1 * hSum = (TH1*)h1->Clone( "hSum" );
    hSum->Add( h2 );

    TF2 *f2 = new TF2("f2","[0]*TMath::Gaus(x,[1],[2])*TMath::Gaus(y,[3],[4])",0,10,0,10); 
    f2->SetParameters(1,3,1,4,1);

    TH2 * h3 = new TH2F( "h3", "h3;X;Y", 200, -10, 10, 200, -10, 10 );
    h3->FillRandom( "f2", 1000000 );


    f->Write();

}
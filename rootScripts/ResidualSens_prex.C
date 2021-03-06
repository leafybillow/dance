#include "utilities.cc"
#include "plot_util.cc"
vector<Double_t> ResidualSens_prex(Int_t slug_number,Int_t kSwitch,Bool_t kPlot);

void ResidualSens_prex(){
  for(int i=1;i<=94;i++){
    ResidualSens_prex(i,0,kTRUE);
    ResidualSens_prex(i,1,kTRUE);
    ResidualSens_prex(i,2,kTRUE);
  }
}

vector<Double_t> ResidualSens_prex(Int_t slug_number,Int_t kSwitch,Bool_t kPlot){

  vector<Double_t> fTotalResidual;
  vector<Double_t> norm_counts;
  map<Int_t,Int_t> fArmMap = LoadArmMapBySlug(slug_number);
  vector<Int_t> fRunList = LoadRunListBySlug(slug_number);
  map< Int_t, vector<Int_t> > fblmap = LoadBadCycleList();

  TString tree_name;
  if(slug_number<=76)
    tree_name ="dit_slope1";
  else if(slug_number<=94)
    tree_name ="dit_slope3";
  else
    tree_name ="dit_slope1";

  TChain *slope_tree = new TChain(tree_name);
  Int_t nrun = fRunList.size();
  for(int i=0;i<nrun;i++){
    TString filename=Form("./dit-coeffs/prexPrompt_ditcoeffs_%d.root",
			  fRunList[i]);
    slope_tree->Add(filename);
  }

  TChain *sens = new TChain("sens");
  for(int i=0;i<nrun;i++){
    TString filename=Form("./dit-coeffs/prexPrompt_ditcoeffs_%d.root",
			  fRunList[i]);
    sens->Add(filename);
  }
  sens->AddFriend(slope_tree);

  Int_t coil_index[] = {1,3,4,6,7,2,5};

  vector<TString> mon_set1={"bpm4aX","bpm4eX","bpm4aY","bpm4eY","bpm11X12X"};  // run 3404 - 4980
  vector<TString> mon_set2={"bpm4aX","bpm4eX","bpm4aY","bpm4eY","bpm12X"}; // run 3130-3403
  vector<TString> mon_array;

  if(slug_number<=3)
    mon_array = mon_set2;
  else if(slug_number<=94)
    mon_array = mon_set1;
  else
    mon_array = mon_set2;

  vector<TString> det_array = {"usl","usr","dsl","dsr"};
  // "sam1","sam2","sam3","sam4",
  // "sam5","sam6","sam7","sam8"};
  vector<TString> at_array={"atl1","atl2","atr1","atr2"};
  if(slug_number>=26)
    det_array.insert(det_array.end(),at_array.begin(),at_array.end());

  const Int_t ncoil = sizeof(coil_index)/sizeof(*coil_index);
  const Int_t nmon = mon_array.size();
  const Int_t ndet = det_array.size();
  for(int i=0;i<ndet;i++){
    fTotalResidual.push_back(0.0);
    norm_counts.push_back(0.0);
  }
  map<Int_t , vector< vector<Double_t> > > fSlopeRunMap;
  TTree *slope_input;
  switch(kSwitch){
  case 0:{
    TString filename=Form("slopes/slug%d_dit_slope_cyclewise_average.root",slug_number); 
    TFile *input = TFile::Open(filename);
    slope_input = (TTree*)input->Get("dit");
    break;
  }
  case 1:{
    TString filename=Form("slopes/slug%d_dit_slope_merged_cycle_ovcn.root",slug_number); 
    TFile *input = TFile::Open(filename);
    slope_input = (TTree*)input->Get("dit1");
    break;
  }
  case 2:{
    TString filename=Form("slopes/slug%d_dit_slope_merged_cycle_ovcn.root",slug_number); 
    TFile *input = TFile::Open(filename);
    slope_input = (TTree*)input->Get("dit2");
    break;
  }
  }
  
  fSlopeRunMap = GetSlopeMap(det_array,mon_array,slope_input);

  vector<Double_t> fdummy_det(ndet,0.0);
  vector<Double_t> fdummy_mon(nmon,0.0);

  vector<vector<Double_t> > det_val(ncoil,fdummy_det);
  vector<vector<Double_t> > det_err(ncoil,fdummy_det);
  vector<vector<Double_t> > mon_val(ncoil,fdummy_mon);
  vector<vector<Double_t> > mon_err(ncoil,fdummy_mon);
  vector<vector<Double_t> > slope_val(ndet,fdummy_mon);
  vector<vector<Double_t> > slope_flag(ndet,fdummy_mon);
  Double_t cycID;
  Double_t run;
  sens->SetBranchAddress("cycID",&cycID);
  sens->SetBranchAddress("run",&run);
  TString branch_name;
  for(int icoil=0;icoil<ncoil;icoil++){
    for(int imon=0;imon<nmon;imon++){
      branch_name = Form("%s_coil%d_err",
			 mon_array[imon].Data(),coil_index[icoil]);
				 
      sens->SetBranchAddress(branch_name, &mon_err[icoil][imon]);

      branch_name = Form("%s_coil%d",
			 mon_array[imon].Data(),coil_index[icoil]);
      
      sens->SetBranchAddress(branch_name, &mon_val[icoil][imon]);
    }
    for(int idet=0;idet<ndet;idet++){
      branch_name = Form("%s_coil%d_err",
			 det_array[idet].Data(),coil_index[icoil]);
				 
      sens->SetBranchAddress(branch_name, &det_err[icoil][idet]);

      branch_name = Form("%s_coil%d",
			 det_array[idet].Data(),coil_index[icoil]);

      sens->SetBranchAddress(branch_name, &det_val[icoil][idet]);
    }
  }

  for(int idet=0;idet<ndet;idet++){
    for(int imon=0;imon<nmon;imon++){
      TString ch_name = Form("%s_%s",det_array[idet].Data(),mon_array[imon].Data());
      TString flag_name = Form("%s_%s_flag",det_array[idet].Data(),mon_array[imon].Data());
      sens->SetBranchAddress(ch_name,&slope_val[idet][imon]);
      sens->SetBranchAddress(flag_name,&slope_flag[idet][imon]);
    }
  }

  Int_t ncycles=0;
  vector< Double_t > fdmy_vec;
  vector< vector<Double_t > > fRes_cyc(ndet*ncoil,fdmy_vec);
  vector< vector<Double_t > > fResXcord_cyc(ndet*ncoil,fdmy_vec);
  vector< vector<Double_t > > fRes(ndet*ncoil,fdmy_vec);
  vector< vector<Double_t > > fResRaw(ndet*ncoil,fdmy_vec);
  vector< vector<Double_t > > fResXcord(ndet*ncoil,fdmy_vec);
  vector< Int_t > fCycleArray;
  TH1D hist1("hist1","",30,-1.5,1.5);
  vector<TH1D> hPull(ndet*ncoil,hist1);
  Int_t nevt = sens->GetEntries();
  Bool_t kMatch;
  vector<Int_t> fRunLabel;
  vector<Double_t> fRunLabelXcord;
  Int_t last_run = 0.0;
  for(int ievt=0;ievt<nevt;ievt++){
    sens->GetEntry(ievt);
    if(find(fRunList.begin(),fRunList.end(),(Int_t)run)==fRunList.end())
      kMatch = kFALSE;
    else
      kMatch = kTRUE;
    if(kMatch){
      ncycles++;
      fCycleArray.push_back(cycID);
      if(run!=last_run){
	fRunLabel.push_back(run);
	fRunLabelXcord.push_back(ncycles-1);
	last_run=run;
      }
      if(fSlopeRunMap.find(run) == fSlopeRunMap.end()){
	cout << " -- Warning: " 
	     << " dit slope for run " << run 
	     << " is not calculated. Will skip " << endl;
	continue;
      }
      
      Int_t arm_flag = fArmMap[(Int_t)run];
      for(int icoil=0;icoil<ncoil;icoil++){
	if(IsGoodCoil(mon_err[icoil])
	   && !IsBadCycle(fblmap,cycID,coil_index[icoil] )){
	  for(int idet=0;idet<ndet;idet++){
	    Double_t residual = compute_residual(det_val[icoil][idet],mon_val[icoil],
						 fSlopeRunMap[run][idet]);
	    
	    if(arm_flag==1 && det_array[idet].Contains("l"))
	      continue;
	    if(arm_flag==2 && det_array[idet].Contains("r"))
	      continue;

	    fTotalResidual[idet] += pow(residual,2);
	    norm_counts[idet]++;

	    fRes[idet*ncoil+icoil].push_back(residual*1e3);
	    hPull[idet*ncoil+icoil].Fill(residual*1e3);
	    fResRaw[idet*ncoil+icoil].push_back(det_val[icoil][idet]*1e3);
	    fResXcord[idet*ncoil+icoil].push_back(ncycles-1);
	    if(slope_flag[idet][0]>0){
	      residual = compute_residual(det_val[icoil][idet],mon_val[icoil],slope_val[idet]);
	      fRes_cyc[idet*ncoil+icoil].push_back(residual*1e3);
	      fResXcord_cyc[idet*ncoil+icoil].push_back(ncycles-1);
	    }
	  } // end of det loop
	} // end of if its good coil
      } // end of coil loop
    }// end of if kMatch
  } // end of event loop

  gStyle->SetTitleSize(0.3);
  gStyle->SetLabelSize(0.07);
  gStyle->SetOptStat("emrou");
  gStyle->SetStatH(0.2);
  gStyle->SetStatW(0.35);
  TCanvas *c2 = new TCanvas("c2","c2",1200,600);
  c2->cd();
  TPad *p1 = new TPad("p1","",0,0.5,0.7,1);
  p1->Draw();
  TPad *p2 = new TPad("p2","",0,0,0.7,0.5);
  p2->Draw();
  TPad *p3 = new TPad("p3","",0.7,0,1,1);
  p3->Draw();
  
  p1->SetRightMargin(0.015);
  p1->SetTopMargin(0.15);
  p1->SetBottomMargin(0);
  p1->SetGridx();
  p2->SetTopMargin(0);
  p2->SetRightMargin(0.015);
  p2->SetGridx();

  Int_t ntext = fRunLabelXcord.size();
  
  for(int idet=0;idet<ndet;idet++){
    for(int icoil=0;icoil<ncoil;icoil++){
      TString title = Form("%s_coil%d",det_array[idet].Data(),coil_index[icoil]);
      TGraph *g_res = GraphVector(fRes[idet*ncoil+icoil],fResXcord[idet*ncoil+icoil]);
      TGraph *g_res_cyc = GraphVector(fRes_cyc[idet*ncoil+icoil],fResXcord_cyc[idet*ncoil+icoil]);
      TGraph *g_raw = GraphVector(fResRaw[idet*ncoil+icoil],fResXcord[idet*ncoil+icoil]);
      p1->cd();
      g_res->SetMarkerColor(kRed);
      g_res->SetMarkerStyle(20);
      g_res_cyc->SetMarkerColor(kBlue);
      g_res_cyc->SetMarkerStyle(20);
      TMultiGraph *mg_res = new TMultiGraph();
      mg_res->Add(g_res_cyc,"lp");
      mg_res->Add(g_res,"lp");
      mg_res->Draw("A");
      mg_res->SetTitle("Residual Sensitivity in " + title);
      mg_res->GetYaxis()->SetTitle("ppm/count");
      mg_res->GetYaxis()->SetTitleSize(0.06);
      mg_res->GetYaxis()->SetTitleOffset(0.6);
      mg_res->GetYaxis()->SetLabelSize(0.08);
      TH1F *hres = mg_res->GetHistogram();
      hres->GetXaxis()->Set(ncycles,-0.5,ncycles-0.5);
      mg_res->GetXaxis()->SetNdivisions(ncycles);

      Double_t ymin1 = mg_res->GetYaxis()->GetXmin();
      Double_t ymax1 = mg_res->GetYaxis()->GetXmax();
      for(int i=0;i<ntext;i++){
	TText *t1 = new TText(fRunLabelXcord[i],ymax1,Form("%d",fRunLabel[i]));
	TLine *line1 = new TLine(fRunLabelXcord[i],ymin1,
				 fRunLabelXcord[i],ymax1);
	line1->SetLineColor(kBlack);
	line1->SetLineWidth(1);
	line1->SetLineStyle(7);
	t1->SetTextAngle(27);
	t1->SetTextSize(0.05);
	line1->Draw("same");
	t1->Draw("same");
      }

      p2->cd();
      g_raw->SetMarkerStyle(20);
      g_raw->SetMarkerColor(kBlack);
      g_raw->Draw("ALP");
      g_raw->SetTitle(" ");
      g_raw->GetYaxis()->SetTitle("Raw Sensitivity (ppm/count)");
      g_raw->GetYaxis()->SetTitleSize(0.06);
      g_raw->GetYaxis()->SetTitleOffset(0.8);
      g_raw->GetYaxis()->SetLabelSize(0.08);
      TH1F *hraw = g_raw->GetHistogram();
      hraw->GetXaxis()->Set(ncycles,-0.5,ncycles-0.5);
      for(int i=0;i<ncycles;i++)
	hraw->GetXaxis()->SetBinLabel(i+1,Form("%d",fCycleArray[i]));
      hraw->SetMarkerColor(kWhite);
      hraw->Draw("same p");

      Double_t ymin = g_raw->GetYaxis()->GetXmin();
      Double_t ymax = g_raw->GetYaxis()->GetXmax();
      for(int i=0;i<ntext;i++){
	TText *t1 = new TText(fRunLabelXcord[i]-0.5,ymax,Form("%d",fRunLabel[i]));
	TLine *line1 = new TLine(fRunLabelXcord[i],ymin,
				 fRunLabelXcord[i],ymax);
	line1->SetLineColor(kBlack);
	line1->SetLineWidth(1);
	line1->SetLineStyle(7);
	t1->SetTextAngle(27);
	t1->SetTextSize(0.03);
	line1->Draw("same");
	t1->Draw("same");
      }

      p3->cd();
      hPull[idet*ncoil+icoil].Draw();
      hPull[idet*ncoil+icoil].GetXaxis()->SetTitle("residual (ppm/count)");
      if(kPlot)
	c2->SaveAs(Form("./plots/slug%d_dit_res_buff_%s.pdf",slug_number,title.Data()));
    } // end of coil loop
  } // end of det loop

  TString pdf_label;
  switch(kSwitch){
  case 0:{
    pdf_label = "cyclewise_avg";
    break;}
  case 1:{
    pdf_label = "ranges_ovcn";
    break;}
  case 2:{
    pdf_label = "run_ovcn";
    break; }
  }
  if(kPlot)
    gSystem->Exec(Form("pdfunite $(ls -rt ./plots/slug%d_dit_res_buff_*.pdf) ./plots/slug%d_dit_res_%s.pdf",slug_number,slug_number,pdf_label.Data()));

  gSystem->Exec(Form("rm -f ./plots/slug%d_dit_res_buff_*.pdf ",slug_number));
  for(int i=0;i<ndet;i++){
    if(norm_counts[i]!=0)
      fTotalResidual[i] = fTotalResidual[i]/norm_counts[i];
  }

  return fTotalResidual;
}

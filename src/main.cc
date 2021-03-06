/**********************************   
Dithering Analysis and Correction Engine
	author: Tao Ye
	<tao.ye@stonybrook.edu>
	last update: Oct 2019
************************************/
#include "TaInput.hh"
#include "TaOutput.hh"
#include "TaConfig.hh"
#include "VAnalysisModule.hh"
#include "TaRegression.hh"

#include "TStopwatch.h"
#include "TString.h"
#include "TROOT.h"
#include <iostream>

using namespace std;
int main(int argc, char** argv){
  TStopwatch tsw;

  tsw.Start();
  cout << "\n*********************************************** " <<endl;
  cout << "Dithering Analysis and Correction Engine "  << endl;
  cout << " (DANCE) " << endl;
  cout<< "\t author: Tao Ye " << endl;
  cout<< "\t <tao.ye@stonybrook.edu>" << endl;
  cout<< "\t last update: Oct 2019" << endl;
  cout << "***********************************************\n " <<endl;

  if(argc==1){
    return 0;
  }

  int opt;
  
  TString confFileName;
  TString japanFileName;
  TString ext_filename;
  Int_t run_number=0;
  while( (opt=getopt(argc,argv,":f:r:l:c:"))!=-1){
    switch(opt){
    case ':':
      cerr << argv[optind-1] << "requires value. " <<endl;
      return 1;
    case '?':
      cerr << "Unknown arguments " <<optopt << endl;
      return 1;
    case 'f':
      japanFileName=TString(optarg);
      break;
    case 'c':
      confFileName=TString(optarg);
      break;
    case 'l':
      ext_filename=TString(optarg);
      break;
    case 'r':
      run_number= atoi(optarg);
      break;
    }
  }
  
  TaConfig *fConfig= new TaConfig();
  fConfig->SetRunNumber(run_number);
  fConfig->SetInputName(japanFileName);
  fConfig->ParseFile(confFileName);
  TaInput *fInput = new TaInput(fConfig);
  TaOutput *fOutput = new TaOutput(fConfig); 
  fInput->LoadROOTFile();
  fInput->WriteRawChannels(fOutput);
  
  vector<VAnalysisModule*> fAnalyses = fConfig->GetAnalysisArray();
  vector<VAnalysisModule*>::iterator iter_ana = fAnalyses.begin();
  Int_t kMod=0;
  while(iter_ana!=fAnalyses.end()){
    cout << "Loading Module " <<  kMod++ << endl;
    (*iter_ana)->LoadInput(fInput);
    (*iter_ana)->Process(fOutput);
    (*iter_ana)->End();
    iter_ana++;
  }

  fOutput->Write();
  fOutput->Close();
  fInput->Close();  

  cout <<" -- Done in " ;
  tsw.Print();
  return 0;
}

#include "TaInput.hh"
#include "TaOutput.hh"

#include <iostream>
#include "TLeaf.h"
#include "TBranch.h"
#include "TString.h"
#include "TEventList.h"
#include "TCut.h"

ClassImp(TaInput);
using namespace std;

TaInput::TaInput(TaConfig *aConfig){
  run_number= aConfig->GetRunNumber();
  seg_number=aConfig->GetSegNumber();
  isExternalConstraint=kFALSE;
  InitChannels(aConfig);
  input_name = aConfig->GetInputName();
  input_path=aConfig->GetConfigParameter("input_path");
  input_prefix=aConfig->GetConfigParameter("input_prefix");
  TString cMinirun_size =(aConfig->GetConfigParameter("minirun_size"));
  if(cMinirun_size=="burst-counter"){
    kUseBurstCounter = kTRUE;
    cout << " -- minirun will be defined by burst counter " << endl;
  }else{
    minirun_size = cMinirun_size.Atof();
    kUseBurstCounter = kFALSE;
  }
  TString output_mini_flag = aConfig->GetConfigParameter("mini_only");
  if(output_mini_flag=="on")
    kMiniOnly = kTRUE;
  else
    kMiniOnly = kFALSE;
}
TaInput::~TaInput(){}

Bool_t TaInput::LoadROOTFile(){
#ifdef NOISY
  cout <<  __FUNCTION__ << endl;
#endif  
  if(run_number!=0){
    if(input_path=="") 
      input_path = TString(getenv("QW_ROOTFILES")); 
    input_name = input_path+"/"+input_prefix
      +Form("%d.%03d.root",(int)run_number,(int)seg_number);
    cout << " -- Opening "
	 << input_name << endl;
    input_file = TFile::Open(input_name);
  }
  else{
    cout << " -- Opening "
	 << input_name << endl;
    input_file = TFile::Open(input_name);
  }

  if(input_file==NULL){
    cerr << __PRETTY_FUNCTION__
	 << " Error: Input file is not found !! " << endl;
    return kFALSE;
  }

  evt_tree = (TTree*)input_file->Get("evt");
  bmw_tree = (TTree*)input_file->Get("evt_bmw");
  mul_tree = (TTree*)input_file->Get("mul");
  mulc_tree = (TTree*)input_file->Get("mulc");
  if(mulc_tree!=NULL)
    mul_tree->AddFriend(mulc_tree);
  
  return kTRUE;
}

void TaInput::InitChannels(TaConfig *aConfig){
#ifdef NOISY
  cout <<  __FUNCTION__ << endl;
#endif  
  vector<TaDefinition*> device_list = aConfig->GetDeviceDefList();
  int nDevice = device_list.size();
  for(int i=0;i<nDevice;i++){
    TaChannel* aChannel = new TaChannel("mul",device_list[i]);
    fChannelArray.push_back(aChannel);
    fChannelNames.push_back(device_list[i]->GetName());
    fChannelMap[device_list[i]->GetName()]=aChannel;
  }
  // Load Definition and connect channels
  fChannelErrorFlag = new TaChannel("mul","ErrorFlag");
  fChannelArray.push_back(fChannelErrorFlag);
  fChannelNames.push_back("ErrorFlag");
  fChannelMap["ErrorFlag"]=fChannelErrorFlag;

  fChannelCutFlag = new TaChannel("mul","ok_cut");
}

void TaInput::WriteRawChannels(TaOutput *aOutput){
#ifdef NOISY
  cout <<  __PRETTY_FUNCTION__ << endl;
#endif  

  TCut default_cut("ErrorFlag==0");
  TEventList *elist_mul = new TEventList("elist_mul");  
  Int_t nGoodPatterns = mul_tree->Draw(">>+elist_mul", default_cut, "goff");
  cout << " -- nGoodPatterns in Mul Tree : " << nGoodPatterns << endl;
  
  mul_tree->SetBranchStatus("*",0);
  int nch = fChannelNames.size();
  for(int i=0;i<nch;i++){
    mul_tree->SetBranchStatus(fChannelNames[i],1);
    TBranch *aBranch = mul_tree->GetBranch(fChannelNames[i]);
    if(aBranch!=NULL){
      fChannelArray[i]->RegisterBranchAddress(aBranch);
      // if combined channel is already defined in mulc 
      fChannelArray[i]->SetDefUsage(kFALSE);
    }
    else{
      cout << " ** TBranch " <<fChannelNames[i] << " is not found in JAPAN mulc " << endl;
      if(fChannelArray[i]->HasUserDefinition()){
	cout << " -- " << fChannelNames[i] << " finds definition from user.    " << endl;
	fChannelArray[i]->SetDefUsage(kTRUE);
      }else{
	cout << " ** " <<fChannelNames[i] << " is invalid.    " << endl;
	fChannelArray[i]->SetInvalid();
      }
    }
  }
  
  ConnectChannels();
  TString leaflist = "hw_sum/D:block0:block1:block2:block3";
  for(int ich=0;ich<nch;ich++){
    if(!(fChannelArray[ich]->isValidChannel()))
      continue;
    if(!kMiniOnly)
      fChannelArray[ich]->ConstructTreeBranch(aOutput,leaflist);
    fChannelArray[ich]->ConstructMiniTreeBranch(aOutput,"mini");
    fChannelArray[ich]->ConstructSumTreeBranch(aOutput,"sum");
  }
  if(!kMiniOnly)
    fChannelCutFlag->ConstructTreeBranch(aOutput);
  
  Int_t mini_id=0;
  Int_t kRunNumber = (Int_t)run_number;
  aOutput->ConstructTreeBranch("sum","run",kRunNumber);
  aOutput->ConstructTreeBranch("mini","mini",mini_id);
  aOutput->ConstructTreeBranch("mini","run",kRunNumber);
  if(!kMiniOnly)
    aOutput->ConstructTreeBranch("mul","mini",mini_id);
  Int_t ievt =0;
  Double_t goodCounts=0;
  Int_t mini_start =0;
  Int_t mini_end =0;
  Bool_t isGoodPattern=kFALSE;
  Int_t fCurrentBurst = 0;
  Short_t fBurstCounter;
  mul_tree->SetBranchStatus("BurstCounter",1);
  mul_tree->SetBranchAddress("BurstCounter",&fBurstCounter);
  while((mul_tree->GetEntry(ievt))>0){

    if(kUseBurstCounter && fBurstCounter > fCurrentBurst){
      cout << "fCurrentBurst=" << fCurrentBurst << endl;
      fCurrentBurst  = fBurstCounter;
      mini_end = ievt-1;
      minirun_range.push_back(make_pair(mini_start,mini_end));
      cout << " -- Mini-run ends at event: " << mini_end << endl;
      mini_start = mini_end+1;
      cout << " -- Next Mini-run starts at event: " << mini_start << endl;
	
      for(int ich=0;ich<nch;ich++){
	fChannelArray[ich]->UpdateMiniStat();
	fChannelArray[ich]->ResetMiniAccumulator();
      }
      aOutput->FillTree("mini");
      mini_id++;
    }

    if(elist_mul->GetIndex(ievt)!=-1){
      isGoodPattern=kTRUE;
      goodCounts++;
      fChannelCutFlag->fOutputValue.hw_sum = 1; // FIXME
      fChannelCutFlag->FillDataArray();
    }
    else{
      isGoodPattern=kFALSE;
      fChannelCutFlag->fOutputValue.hw_sum = 0; // FIXME
      fChannelCutFlag->FillDataArray();
    }
    for(int ich=0;ich<nch;ich++){
      fChannelArray[ich]->FillDataArray();
      if(isGoodPattern){
	fChannelArray[ich]->AccumulateRunSum();
	fChannelArray[ich]->AccumulateMiniSum();
      }
    }
    if(!kMiniOnly)
      aOutput->FillTree("mul");
    ievt++;
    
    if(goodCounts==minirun_size && !kUseBurstCounter){
      mini_end = ievt-1;
      minirun_range.push_back(make_pair(mini_start,mini_end));
      cout << " -- Mini-run ends at event: " << mini_end << endl;
      mini_start = mini_end+1;
      cout << " -- Next Mini-run starts at event: " << mini_start << endl;
      goodCounts = 0;
      int nMinirun = minirun_range.size();
      Bool_t is_last_minrun = kFALSE;
      if(nGoodPatterns-minirun_size*nMinirun<minirun_size){
	is_last_minrun = kTRUE;
	mini_start = minirun_range[nMinirun-1].first;
	minirun_range.pop_back();
	
	cout << " -- Meeting the last mini-run, " << endl;
	cout << " -- the rest will be merged into this mini-run  "  << endl;
      }
      if(!is_last_minrun){
	for(int ich=0;ich<nch;ich++){
	  fChannelArray[ich]->UpdateMiniStat();
	  fChannelArray[ich]->ResetMiniAccumulator();
	}
	aOutput->FillTree("mini");
	mini_id++;
      }
    }
  }
  cout << " -- last mini-run ends at event: " << ievt-1 << endl;
  minirun_range.push_back(make_pair(mini_start,ievt-1));

  for(int ich=0;ich<nch;ich++){
    fChannelArray[ich]->UpdateMiniStat();
    fChannelArray[ich]->UpdateRunStat();
    fChannelArray[ich]->ResetMiniAccumulator();
  }
  aOutput->FillTree("mini");
  aOutput->FillTree("sum");
}


void TaInput::Close(){
  input_file->Close();
}

TaChannel* TaInput::GetChannel(TString name){
  return  fChannelMap[name];
}

void TaInput::ConnectChannels(){
  int nch = fChannelArray.size();
  for(int i=0;i<nch;i++){
    if(fChannelArray[i]->IsUsingDefinition()){
      vector<TString> fRawElementName = fChannelArray[i]->GetRawChannelList();
      vector<Double_t> fPrefactors = fChannelArray[i]->GetFactorArray();
      vector<TaChannel*> fRawChannel;
      auto iter_ele = fRawElementName.begin();
      while(iter_ele!=fRawElementName.end()){
	TaChannel *channel_ptr = fChannelMap[*iter_ele];
	fRawChannel.push_back(channel_ptr);
	iter_ele++;
      }
      fChannelArray[i]->ConnectChannels(fRawChannel,fPrefactors);
    }
  }
}

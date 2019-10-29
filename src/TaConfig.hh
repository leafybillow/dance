#ifndef __TaConfig_hh__
#define __TaConfig_hh__

#include "TaInput.hh"
#include "TaOutput.hh"

#include "TString.h"
#include "Rtypes.h"

#include <vector>
#include <map>
#include <iostream>
#include <fstream>

using namespace std;
class TaOutput;
class VAnalysisModule;
class TaRegression;
class TaConfig{
public:
  TaConfig();
  virtual ~TaConfig(){};

  Bool_t ParseFile(TString fileName);

  inline vector<TString> GetDVlist() const { return fDVlist;};
  inline vector<TString> GetIVlist(Int_t ana_index) { return fIVMap[ana_index];};
  TString GetConfigParameter(TString key);
  TString GetAnalysisParameter(Int_t index, TString key);
  inline vector<TString> GetAnalysisTypeArray() const {return fAnalysisTypeArray;};

  inline vector<TString> GetDependentVarArray() const { return fDependentVarArray;};
  inline vector<TString> GetDetectorList() const { return fDetectorArray;};
  inline vector<TString> GetRawElementArray() const { return fRawElementArray;};
  inline vector< pair<TString,TString> > GetDataElementDefinitions() const { 
    return fDataElementDefinitions;
  };

  inline vector<TString> GetCoilList(Int_t i) {  return fCoilMap[i];};
  inline vector<TString> GetMonitorList(Int_t i) {  return fMonitorMap[i];};

  inline Int_t GetRunNumber() const { return run_number;};
  inline vector<TString> GetDeviceList() const {return device_list;};
  inline TString GetInputName() const { return input_name;};

  inline void SetInputName(TString str){input_name=str;};
  inline void SetRunNumber(Int_t i){run_number=i;};
  
  vector<VAnalysisModule*> GetAnalysisArray();
  vector<TString> ParseChannelDefinition(TString );
  
private:

  TString configName;
  Int_t run_number;
  TString input_name;

  map<TString, TString> fConfigParameters;
  map< pair<Int_t,TString> , TString> fAnalysisParameters;
  vector<TString>  fDVlist;
  map< Int_t , vector<TString> > fIVMap;
  map<TString,Int_t> device_map;
  vector<TString> device_list;
  vector< TString > fAnalysisTypeArray;
  // map< pair<TString, TString>, Int_t > fAnalysisMap;

  vector<TString>  ParseLine(TString, TString);
  TString ParseAnalysisType(TString);

  // Beam Mod elements
  vector<TString> fDetectorArray;
  vector<TString> fDependentVarArray;  // for dithering sensitivity

  vector<TString> fRawElementArray;
  vector<TString> fDefinedElementArray;
  vector< pair<TString, TString> > fDataElementDefinitions;

  map< Int_t, vector<TString> > fMonitorMap; // by Analysis Index
  map< Int_t, vector<TString> > fCoilMap;

  ClassDef(TaConfig,0);
};

#endif

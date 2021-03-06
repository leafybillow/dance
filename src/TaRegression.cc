#include "TaRegression.hh"
#include "TBranch.h"
#include "TLeaf.h"
#include "TEventList.h"
#include "TStopwatch.h"
ClassImp(TaRegression);

TaRegression::TaRegression(Int_t ana_index, TaConfig *fConfig){
#ifdef NOISY
  cout << __PRETTY_FUNCTION__ << endl;
#endif
  Init(ana_index,fConfig);
}

void TaRegression::Process(TaOutput *fOutput){
#ifdef NOISY
  cout << __PRETTY_FUNCTION__ << endl;
#endif

  ConstructOutputs(fOutput);
  Int_t nMini =  minirun_range.size();
  const Int_t nDV = sDVlist.size();
  const Int_t nIV = sIVlist.size();

  vector<Double_t> fvector(nIV,0.0); // place-holder
  Double_t coeff[nDV][nIV];
  TString branch_desc = Form("coeff[%d][%d]/D",nDV,nIV);
  fOutput->ConstructTreeBranch("mini_"+tree_name,"coeff",branch_desc,coeff);

  for(int ich=0;ich<nDV;ich++){
    fCorrections[ich]->ConnectChannels(fIndependentVar,fvector);
    fCorrections[ich]->ConstructSlopeBranch(fOutput,"mini_"+tree_name);
  }

  for(int imini=0; imini<nMini;imini++){
    TMatrixD CovDM = GetDetMonCovMatrix(imini);
    TMatrixD CovMM = GetMonMonCovMatrix(imini);
    vector<vector<Double_t> > fSlopes = Solve(CovDM,CovMM);
    
    for(int idv=0;idv<nDV;idv++)
      for(int iiv=0;iiv<nIV;iiv++)
	coeff[idv][iiv] = fSlopes[idv][iiv];

    for(int ich=0;ich<nDV;ich++){
      vector<Double_t> fprefactors = fSlopes[ich];
      fCorrections[ich]->ConnectChannels(fIndependentVar,fprefactors);
    }

    int istart = minirun_range[imini].first;
    int iend = minirun_range[imini].second;
    for(int ievt=istart;ievt<=iend;ievt++){
      GetEntry(ievt);
      CalcCombination();
      AccumulateMiniSum();
      AccumulateRunSum();
      if(!kOutputMiniOnly)
	fOutput->FillTree(tree_name);
    } // end of  event loop
    UpdateMiniStat();
    fOutput->FillTree("mini_"+tree_name);
    ResetMiniAccumulator();
  } // end of mini run loop
  UpdateRunStat();
  fOutput->FillTree("sum_"+tree_name);
}


vector<vector<Double_t> > TaRegression::Solve(TMatrixD CovDM, TMatrixD CovMM){
#ifdef NOISY
  cout << __FUNCTION__ << endl;
#endif

  TMatrixD lhsM(CovMM);
  TMatrixD rhsM(CovDM);
    
  TMatrixD invlhsM = lhsM.Invert();
  TMatrixD solutionM = invlhsM*rhsM;
#ifdef NOISY	
  cout << " -- Slopes Matrix (nMon x nDet) " << endl;
  solutionM.Print();
#endif
  vector<vector<Double_t> >  fSlopesContainer;
  Int_t ncol = solutionM.GetNcols();
  for(Int_t icol=0;icol<ncol;icol++)
    fSlopesContainer.push_back(GetColumnVector(solutionM,icol));
  return fSlopesContainer;
}


void TaRegression::WriteSummary(){
#ifdef NOISY
  cout << __FUNCTION__ << endl;
#endif

}

Double_t TaRegression::GetCovariance(TaChannel *c1, TaChannel *c2,Int_t imini){
  Double_t cov;
  Int_t start_evt = minirun_range[imini].first;
  Int_t end_evt = minirun_range[imini].second;
  TaAccumulator fCovAccumulator;
  
  for(int ievt=start_evt;ievt<=end_evt;ievt++){
    if(fChannelCutFlag->GetEntry(ievt))
      fCovAccumulator.Update(c1->GetEntry(ievt),c2->GetEntry(ievt));
  }
  cov = fCovAccumulator.GetM2()/fCovAccumulator.GetN();

  return cov;
}

TMatrixD TaRegression::GetDetMonCovMatrix(Int_t imini){
  Int_t nDV = fDependentVar.size();
  Int_t nIV = fIndependentVar.size();

  TMatrixD retMatrix(nIV,nDV);
  for(int irow=0;irow<nIV;irow++)
    for(int icol=0;icol<nDV;icol++)
      retMatrix[irow][icol]=GetCovariance(fDependentVar[icol],
					  fIndependentVar[irow],
					  imini);
#ifdef NOISY
  cout << __PRETTY_FUNCTION__<< endl;
  retMatrix.Print();
#endif
  return retMatrix;
}

TMatrixD TaRegression::GetMonMonCovMatrix(Int_t imini){
  Int_t nIV = fIndependentVar.size();
  TMatrixD retMatrix(nIV,nIV);

  for(int irow=0;irow<nIV;irow++)
    for(int icol=irow;icol<nIV;icol++){
      retMatrix[irow][icol]=GetCovariance(fIndependentVar[icol],
					  fIndependentVar[irow],
					  imini);
      if(icol!=irow)
      	retMatrix[icol][irow] = retMatrix[irow][icol];
    }
#ifdef NOISY
  cout << __PRETTY_FUNCTION__<< endl;
  retMatrix.Print();
#endif
  return retMatrix;
}

vector<Double_t> TaRegression::GetColumnVector(TMatrixD matrix_in, Int_t icol ){
  Int_t nrow = matrix_in.GetNrows();
  vector<Double_t> ret_vector;
  for(int irow=0;irow<nrow;irow++)
    ret_vector.push_back(matrix_in(irow,icol));
  return ret_vector;
}

vector<Double_t> TaRegression::GetRowVector(TMatrixD matrix_in, Int_t irow ){
  Int_t ncol = matrix_in.GetNcols();
  vector<Double_t> ret_vector;
  for(int icol=0;icol<ncol;icol++)
    ret_vector.push_back(matrix_in(irow,icol));
  return ret_vector;
}

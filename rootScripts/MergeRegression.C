void MergeRegression(Int_t slug);
void MergeRegression(TString label);

void MergeRegression(){
  for(int i=1;i<=94;i++)
    MergeRegression(i);
}

void MergeRegression(Int_t slug){
  TString label = Form("slug%d",slug);
  MergeRegression(label);
}

void MergeRegression(TString label){
  TString filename  = label+".list";
  TString path = "./prex-runlist/simple_list/";
  FILE *runlist = fopen((path+filename).Data(),"r");

  vector<TString> tree_name_array={"mini","mini_reg"};
  vector<TTree*> output_tree_array;
  Int_t nTree = tree_name_array.size();
  map<TString,TList*> list_map;
  map<TString, vector<Int_t> >  runID_map;
  for(int i=0;i<nTree;i++){
    list_map[tree_name_array[i]]= new TList();
    output_tree_array.push_back( new TTree() );
  }
  TString postpan_path="./LagrangeRootfiles/";
  if(runlist!=NULL){
    TFile *output = TFile::Open("./slug_sum/newReg_"+label+".root","RECREATE");
    while(!feof(runlist)){
      Int_t run_number=0;
      fscanf(runlist,"%d/n",&run_number);
      if(run_number==0)
	continue;
      
      Int_t seg_number = 0;
      TFile *this_file = TFile::Open(postpan_path+Form("prexPrompt_regress_%d.%03d.root",
						       run_number,seg_number));
      while(this_file!=NULL){
	cout << this_file->GetName() << endl;
	vector<TString>::iterator it_tree  = tree_name_array.begin();
	while(it_tree!=tree_name_array.end()){
	  TTree *aTree_ptr = (TTree*)this_file->Get(*it_tree);
	  if(aTree_ptr!=NULL){
	    list_map[*it_tree]->Add(aTree_ptr);
	    Int_t nburst = aTree_ptr->GetEntries();
	    for(int i=0;i<nburst;i++)
	      runID_map[*it_tree].push_back(run_number);
	  }
	  it_tree++;
	} //end of tree loop
	seg_number++;
	this_file = TFile::Open(postpan_path+Form("prexPrompt_regress_%d.%03d.root",
						  run_number,seg_number));
      } // end of while TFile is not NULL 
    } // end of the loop over run in a list 
    fclose(runlist);
    output->cd();
    
    for(int i =0;i<nTree;i++){
      output_tree_array[i] = TTree::MergeTrees( list_map[tree_name_array[i]] );
      Int_t run;
      TTree *this_tree =output_tree_array[i];
      TBranch *branch_run = this_tree->Branch("run",&run,"run/I");
      Int_t nevt = this_tree->GetEntries();
      vector<Int_t> runlist = runID_map[tree_name_array[i]];
      for(int i = 0; i<nevt;i++){
	this_tree->GetEntry(i);
	run = runlist[i];
	branch_run->Fill();
      }
      this_tree->SetMarkerStyle(20);
      this_tree->Write();
    }
    cout << "-- Closing " << output->GetName()<< endl;
    output->Close();
  } // end of if runlist exists
}


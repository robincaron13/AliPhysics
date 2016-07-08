/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

// --- ROOT system ---
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <TROOT.h>
#include <TLine.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TLegend.h>
#include <TString.h>
#include <TLatex.h>

#include <Riostream.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <alloca.h>
#include <string>
#include <cstring>

// --- ANALYSIS system ---
#include <AliCalorimeterUtils.h>
#include <AliEMCALGeometry.h>
#include <AliAnaCaloChannelAnalysis.h>
#include <AliAODEvent.h>

using namespace std;

/// \cond CLASSIMP
ClassImp(AliAnaCaloChannelAnalysis);
/// \endcond


//________________________________________________________________________
AliAnaCaloChannelAnalysis::AliAnaCaloChannelAnalysis():
		TObject(),
		fCurrentRunNumber(-1),fPeriod(),fPass(),fTrigger(),fNoOfCells(),
		fRunListFileName(),fWorkdir(),fTrial(),fExternalFileName(),
		fMergeOutput(),fAnalysisOutput(),fAnalysisInput(),
		fMergedFileName(),fFilteredFileName(),fQADirect(),fRunList(),
		fCaloUtils(),fAnalysisVector()
{
	fCurrentRunNumber = 254381;
	fPeriod           = "LHC16h";
	fPass             = "muon_caloLego";
	fTrigger          = "AnyINT";

	Init();
}
//________________________________________________________________________
AliAnaCaloChannelAnalysis::AliAnaCaloChannelAnalysis(TString period, TString pass, TString trigger, Int_t RunNumber):
		TObject(),
		fCurrentRunNumber(-1),fPeriod(),fPass(),fTrigger(),fNoOfCells(),
		fRunListFileName(),fWorkdir(),fTrial(),fExternalFileName(),
		fMergeOutput(),fAnalysisOutput(),fAnalysisInput(),
		fMergedFileName(),fFilteredFileName(),fQADirect(),fRunList(),
		fCaloUtils(),fAnalysisVector()
{
	fCurrentRunNumber = RunNumber;
	fPeriod           = period;
	fPass             = pass;
	fTrigger          = trigger;

	Init();
}
//________________________________________________________________________
void AliAnaCaloChannelAnalysis::Init()
{
	//......................................................
	//..Default values - can be set by functions
	fWorkdir="./";
	fRunListFileName="runList.txt";
	fTrial = 0;
	fExternalFileName="";

	//..Settings for the input/output structure (hard coded)
	fAnalysisInput  ="AnalysisInput";
	fMergeOutput    ="ConvertOutput";
	fAnalysisOutput ="AnalysisOutput";
	//..Stuff for the convert function
	gSystem->mkdir(fMergeOutput);
	fMergedFileName= Form("%s/%s_%s_Merged.root", fMergeOutput.Data(), fPeriod.Data(),fPass.Data());
	fQADirect      = Form("CaloQA_%s",fTrigger.Data());
	fRunList       = Form("%s/%s/%s/%s", fAnalysisInput.Data(), fPeriod.Data(), fPass.Data(), fRunListFileName.Data());

	fFilteredFileName = Form("%s/%s_%s_Filtered.root", fMergeOutput.Data(), fPeriod.Data(),fPass.Data());
	//.. make sure the vector is empty
	fAnalysisVector.clear();

	//......................................................
	//..Initialize EMCal/DCal geometry
	fCaloUtils = new AliCalorimeterUtils();
	//..Create a dummy event for the CaloUtils
	AliAODEvent* aod = new AliAODEvent();
	fCaloUtils->SetRunNumber(fCurrentRunNumber);
	fCaloUtils->AccessGeometry(aod);
	//..Set the AODB calibration, bad channels etc. parameters at least once
	//fCaloUtils->AccessOADB(aod);


    fNoOfCells    =fCaloUtils->GetEMCALGeometry()->GetNCells(); //..Very important number, never change after that point!
    Int_t NModules=fCaloUtils->GetEMCALGeometry()->GetNumberOfSuperModules();

    //..This is how the calorimeter looks like in the current period (defined by example run ID fCurrentRunNumber)
	cout<<"Number of cells: "<<fNoOfCells<<endl;
	cout<<"Number of supermod: "<<NModules<<endl;
	//cout<<"Number of supermod utils: "<<fCaloUtils->GetNumberOfSuperModulesUsed()<<endl; //..will always be 22 unless set by hand

	//......................................................
	//..Initialize arrays that store cell information
	//..In the histogram: bin 1= cellID 0, bin 2= cellID 1 etc
	//..In the arrays: array[cellID]= some information
	fnewBC  = new Int_t[fNoOfCells];
	fnewDC  = new Int_t[fNoOfCells];
	fpexclu = new Int_t[fNoOfCells];
	fexclu  = new Int_t[fNoOfCells];

	for(Int_t i=0; i<7; i++)
	{
		fpflag[i]  = new Int_t*[fNoOfCells];
		fpflag[i]  = new Int_t*[fNoOfCells];
	}

/*	for(Int_t i=0; i<NrCells; i++)
	{
		fnewBC[i]=0;       // starts at newBC[0] stores cellIDs  (cellID = bin-1)
		fnewDC[i]=0;       // starts at newDC[0] stores cellIDs  (cellID = bin-1)
		fpexclu[i]=0;     // starts at 0 pexclu[CellID] stores 0 not excluded, 1 excluded
		fexclu[i]=0;       // is the same as above
		fpflag[i]=0;   // pflag[cellID][crit] = 1(ok),2(bad),0(bad)     start at 0 (cellID 0 = histobin 1)
		fflag[i]=0;     // is the same as above
	}*/


	//..set all fields to -1
	memset(fnewBC,-1, fNoOfCells *sizeof(int));
	memset(fnewDC,-1, fNoOfCells *sizeof(int));

	//......................................................
	//..setings for the 2D histogram
	Int_t fNMaxCols = 48;  //eta direction
	Int_t fNMaxRows = 24;  //phi direction
	Int_t fNMaxColsAbs=2*fNMaxCols;
	Int_t fNMaxRowsAbs= Int_t (NModules/2)*fNMaxRows; //multiply by number of supermodules (20)
}
//________________________________________________________________________
AliAnaCaloChannelAnalysis::~AliAnaCaloChannelAnalysis()
{
	//..Destructor

}
//________________________________________________________________________
void AliAnaCaloChannelAnalysis::Run()
{
	///..First use Convert() to merge historgrams from a runlist .txt file
	///..The merged outputfile contains 3 different histograms
	///..In a second step analyse these merged histograms
	///..by calling BCAnalysis()
	TString inputfile = "";

	if(fExternalFileName=="")
	{
		cout<<". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . ."<<endl;
		cout<<". . .Start process by converting files. . . . . . . . . . . ."<<endl;
		cout<<endl;
		fMergedFileName = Convert();
		cout<<endl;
	}
	else
	{
		fMergedFileName = Form("%s/%s", fMergeOutput.Data(), fExternalFileName.Data());
	}

	cout<<". . .Load inputfile with name: "<<inputfile<<" . . . . . . . ."<<endl;
	cout<<". . .Continue process by . . . . . . . . . . . ."<<endl;
	cout<<endl;
	cout<<"o o o Bad channel analysis o o o"<<endl;


	BCAnalysis();


	cout<<endl;
	cout<<". . .End of process . . . . . . . . . . . . . . . . . . . . ."<<endl;
	cout<<". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . ."<<endl;

}
//________________________________________________________________________
TString AliAnaCaloChannelAnalysis::Convert()
{
	//..Creates one file for the analysis from several QA output files listed in runlist.txt

	cout<<"o o o Start conversion process o o o"<<endl;
	cout<<"o o o period: " << fPeriod << ", pass: " << fPass << ",  trigger: "<<fTrigger<< endl;

	//..Create histograms needed for adding all the files together
	TH1D *hNEventsProcessedPerRun = new TH1D("hNEventsProcessedPerRun","Number of processed events vs run number",200000,100000,300000);
	//ELI a little problematic to hard code properties of histograms??
	TH2F *hCellAmplitude          = new TH2F("hCellAmplitude","Cell Amplitude",200,0,10,23040,0,23040);
	TH2F *hCellTime               = new TH2F("hCellTime","Cell Time",250,-275,975,23040,0,23040);

	//..Open the text file with the run list numbers and run index
	cout<<"o o o Open .txt file with run indices. Name = " << fRunList << endl;
	FILE *pFile = fopen(fRunList.Data(), "r");
	if(!pFile)cout<<"count't open file!"<<endl;
	Int_t Nentr;
	Int_t q;
	Int_t ncols;
	Int_t nlines = 0 ;
	Int_t RunId[500] ;
	while (1)
	{
		ncols = fscanf(pFile,"  %d ",&q);
		if (ncols< 0) break;
		RunId[nlines]=q;
		nlines++;
	}
	fclose(pFile);


	//..Open the different .root files with help of the run numbers from the text file
	const Int_t nRun = nlines ;
	TString base;
	TString infile;

	cout<<"o o o Start merging process of " << nRun <<" files"<< endl;
	//..loop over the amount of run numbers found in the previous text file.
	for(Int_t i = 0 ; i < nRun ; i++)
	{
		base  = Form("%s/%s/%s/%d", fAnalysisInput.Data(), fPeriod.Data(), fPass.Data(), RunId[i]);
		if ((fPass=="cpass1_pass2")||(fPass=="cfPass1-2"))
		{
			if (fTrigger=="default")
			{
				infile = Form("%s_barrel.root",base.Data());
			}
			else
			{
				infile = Form("%s_outer.root",base.Data());
			}
		}
		else
		{   //..This is a run2 case
			infile = Form("%s.root",base.Data()) ;
		}

		cout<<"    o Open .root file with name: "<<infile<<endl;
		TFile *f = TFile::Open(infile);

		//..Do some basic checks
		if(!f)
		{
			cout<<"Couldn't open/find .root file: "<<infile<<endl;
			continue;
		}
		TDirectoryFile *dir = (TDirectoryFile *)f->Get(fQADirect);
		if(!dir)
		{
			cout<<"Couln't open directory file in .root file: "<<infile<<", directory: "<<fQADirect<<endl;
			continue;
		}
		TList *outputList = (TList*)dir->Get(fQADirect);
		if(!outputList)
		{
			cout << "Couln't get TList from directory file: "<<fQADirect<<endl;
			continue;
		}

		//ELI should one maybe clone the hAmpId histos eg to hCellAmplitude, then one does't need to hard code them.
		TH2F *hAmpId;
		TH2F *hTimeId;
		TH2F *hNEvents;

		hAmpId =(TH2F *)outputList->FindObject("EMCAL_hAmpId");
		if(!hAmpId)
		{
			Printf("hAmpId not found");
			outputList->ls();
			continue;
		}
		hTimeId =(TH2F *)outputList->FindObject("EMCAL_hTimeId");
		if(!hTimeId)
		{
			Printf("hTimeId not found");
			outputList->ls();
			continue;
		}
		hNEvents =(TH2F *)outputList->FindObject("hNEvents");
		if(!hNEvents)
		{
			Printf("hNEvents not found");
			outputList->ls();
			continue;
		}
		Nentr =  (Int_t)hNEvents->GetEntries();

		//..does that mean do not merge small files?
		if (Nentr<100)
		{
			cout <<"    o File to small to be merged. Only N entries " << Nentr << endl;
			continue ;
		}
		cout <<"    o File with N entries " << Nentr<<" will be merged"<< endl;

		hNEventsProcessedPerRun->SetBinContent(RunId[i]-100000,(Double_t)Nentr);
		hCellAmplitude->Add(hAmpId);
		hCellTime->Add(hTimeId);

		outputList->Delete();
		dir->Delete();
		f->Close();
		delete f;
	}

	//.. Save the merged histograms
	cout<<"o o o Save the merged histogramms to .root file with name: "<<fMergedFileName<<endl;
	TFile *BCF = TFile::Open(fMergedFileName,"recreate");
	hNEventsProcessedPerRun->Write();
	hCellAmplitude->Write();
	hCellTime->Write();
	BCF->Close();
	cout<<"o o o End conversion process o o o"<<endl;
	return fMergedFileName;
}
//_________________________________________________________________________
void AliAnaCaloChannelAnalysis::BCAnalysis()
{
	//..Configure a complete analysis with different criteria, it provides bad+dead cells lists
	//..You can manage criteria used and their order, the first criteria will use the original
	//..output file from AliAnalysisTaskCaloCellsQA task, then after each criteria it will use a
	//..filtered file without the badchannel previously identified
	cout<<"o o o Bad channel analysis o o o"<<endl;

    TArrayD PeriodArray;
    for(Int_t i=0;i<fAnalysisVector.size();i++)
    {
		if(i==0)TFile::Open(fMergedFileName);
		else    TFile::Open(fFilteredFileName);
		PeriodArray=fAnalysisVector.at(i);
    		PeriodAnalysis(PeriodArray.At(0),PeriodArray.At(1),PeriodArray.At(2),PeriodArray.At(3));
    }

	//provide dead cells list from original file and draw bad cells candidate from indicated file
	TFile::Open(fMergedFileName);
	PeriodAnalysis(7,0.,0.,0.);

	cout<<"o o o End of bad channel analysis o o o"<<endl;
}
//________________________________________________________________________
void AliAnaCaloChannelAnalysis::AddPeriodAnalysis(Int_t criteria, Double_t Nsigma, Double_t Emin, Double_t Emax)
{
  TArrayD PeriodArray;
  PeriodArray.Set(4);
  PeriodArray.AddAt((Double_t)criteria,0);
  PeriodArray.AddAt(Nsigma,1);
  PeriodArray.AddAt(Emin,2);
  PeriodArray.AddAt(Emax,3);
  fAnalysisVector.push_back(PeriodArray);
}
//________________________________________________________________________
void AliAnaCaloChannelAnalysis::Draw2(Int_t cell, Int_t cellref)
{
	//ELI not used anywhere
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(0);
	gStyle->SetFillColor(kWhite);
	gStyle->SetTitleFillColor(kWhite);
	gStyle->SetPalette(1);
	char out[120]; char title[100]; char name[100];char name2[100];
	TString slide(Form("Cells %d-%d",cell,cell));

	sprintf(out,"%d.gif",cell);
	TH2 *hCellAmplitude = (TH2*) gFile->Get("hCellAmplitude");
	TH1 *hCellref = hCellAmplitude->ProjectionX("badcells",cellref+1,cellref+1);

	TCanvas *c1 = new TCanvas("badcells","badcells",600,600) ;
	c1->SetLogy();

	// hCellref->Rebin(3);
	TLegend *leg = new TLegend(0.7, 0.7, 0.9, 0.9);

	sprintf(name,"Cell %d",cell) ;
	TH1 *hCell = hCellAmplitude->ProjectionX(name,cell+1,cell+1);

	sprintf(title,"Cell %d      Entries : %d Enties ref: %d",cell, (Int_t)hCell->GetEntries(),(Int_t)hCellref->GetEntries()) ;
	hCell->SetLineColor(2)  ;
	// cout<<title<<endl ;
	hCell->SetMaximum(1e5);
	// hCell->Rebin(3);
	hCell->SetAxisRange(0.,10.);
	hCell->GetXaxis()->SetTitle("E (GeV)");
	hCell->GetYaxis()->SetTitle("N Entries");
	hCellref->SetAxisRange(0.,10.);
	hCell->SetLineWidth(1) ;
	hCellref->SetLineWidth(1) ;
	hCell->SetTitle(title);
	hCellref->SetLineColor(1)  ;
	leg->AddEntry(hCellref,"reference","l");
	leg->AddEntry(hCell,"current","l");
	hCell->Draw() ;
	hCellref->Draw("same") ;
	leg->Draw();
	sprintf(name2,"Cell%dLHC13MB.gif",cell) ;
	c1->SaveAs(name2);

}

//________________________________________________________________________
void AliAnaCaloChannelAnalysis::SaveBadCellsToPDF(Int_t cell[], Int_t iBC, Int_t nBC, TString PdfName, const Int_t cellref)
{
	//Allow to produce a pdf file with badcells candidates (red) compared to a refence cell (black)
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(0);
	gStyle->SetFillColor(kWhite);
	gStyle->SetTitleFillColor(kWhite);
	gStyle->SetPalette(1);
	//char out[120];
	char title[100];
	char name[100];
	if(cell[iBC]==-1)cout<<"### strange shouldn't happen 1, cell id: "<<iBC<<endl;
	if(cell[iBC+nBC-1]==-1)cout<<"### strange shouldn't happen 2"<<endl;

	TString slide     = Form("Cells %d-%d",cell[iBC],cell[iBC+nBC-1]);
	TString reflegend = Form("reference Cell %i",cellref);
	//sprintf(out,"%d-%d.gif",cell[iBC],cell[iBC+nBC-1]);

	TH2 *hCellAmplitude = (TH2*) gFile->Get("hCellAmplitude");
	TH1 *hCellref = hCellAmplitude->ProjectionX("badcells",cellref+1,cellref+1);

	TCanvas *c1 = new TCanvas("badcells","badcells",1000,750);
	if(nBC > 6) c1->Divide(3,3);
	else if (nBC > 3)  c1->Divide(3,2);
	else  c1->Divide(3,1);

	TLegend *leg = new TLegend(0.7, 0.7, 0.9, 0.9);
	for(Int_t i=0; i<nBC ; i++)
	{
		sprintf(name, "Cell %d",cell[iBC+i]) ;
		TH1 *hCell = hCellAmplitude->ProjectionX(name,cell[iBC+i]+1,cell[iBC+i]+1);
		sprintf(title,"Cell %d      Entries : %d  Ref : %d",cell[iBC+i], (Int_t)hCell->GetEntries(), (Int_t)hCellref->GetEntries() ) ;

		c1->cd(i%9 + 1);
		c1->cd(i%9 + 1)->SetLogy();
		hCell->SetLineColor(2);
		hCell->SetMaximum(1e6);
		hCell->SetAxisRange(0.,10.);
		hCell->GetXaxis()->SetTitle("E (GeV)");
		hCell->GetYaxis()->SetTitle("N Entries");
		hCell->SetLineWidth(1) ;
		hCell->SetTitle(title);
		hCellref->SetAxisRange(0.,8.);
		hCellref->SetLineWidth(1);
		hCellref->SetLineColor(1);

		if(i==0)
		{
			leg->AddEntry(hCellref,reflegend,"l");
			leg->AddEntry(hCell,"current","l");
		}
		hCell->Draw() ;
		hCellref->Draw("same") ;
		leg->Draw();
	}

	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//..Store the created canvas in a .pdf file
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	if(nBC<9)
	{
		cout<<"Teeeeest1"<<endl;
		PdfName +=")";  //ELI this is strange
		cout<<"Print canvas to file: "<<PdfName.Data()<<endl;
		c1->Print(PdfName.Data());
	}
	else if(iBC==0)
	{
		cout<<"Teeeeest2"<<endl;
		PdfName +="("; //ELI this is strange
		cout<<"Print canvas to file: "<<PdfName.Data()<<endl;
		c1->Print(PdfName.Data());
	}
	else
	{
		cout<<"Print canvas to file: "<<PdfName.Data()<<endl;
		c1->Print(PdfName.Data());
	}

	//c1->Update();
	//c1->WaitPrimitive();

	delete hCellref;
	delete c1;
	delete leg;
}
//_________________________________________________________________________
//_________________________________________________________________________

void AliAnaCaloChannelAnalysis::Process(Int_t *pflag[23040][7], TH1* inhisto, Double_t Nsigma, Int_t dnbins, Double_t dmaxval)
{  
	//  1) create a distribution for the input histogram;
	//  2) fit the distribution with a gaussian
	//  3) define good area within +-Nsigma to identfy badcells
	//
	// inhisto -- input histogram;
	// dnbins  -- number of bins in distribution;
	// dmaxval -- maximum value on distribution histogram.

	gStyle->SetOptStat(1); // MG modif
	gStyle->SetOptFit(1);  // MG modif
	Int_t crit = *pflag[0][0] ; //identify the criterum processed
	if(crit==1)cout<<"    o Fit average energy per hit distribution"<<endl;
	if(crit==2)cout<<"    o Fit average hit per event distribution"<<endl;

	//..setings for the 2D histogram
	Int_t fNMaxCols = 48;  //eta direction
	Int_t fNMaxRows = 24;  //phi direction
	Int_t fNMaxColsAbs=2*fNMaxCols;
	Int_t fNMaxRowsAbs=Int_t (20/2)*fNMaxRows; //multiply by number of supermodules (20)
	Int_t CellColumn=0,CellRow=0;
	Int_t CellColumnAbs=0,CellRowAbs=0;
	Int_t Trash;

	TString HistoName=inhisto->GetName();
	Double_t goodmax= 0. ;
	Double_t goodmin= 0. ;
	*pflag[0][0] =1; //ELI why is this done??
	if (dmaxval < 0.)
	{
		dmaxval = inhisto->GetMaximum()*1.01;  // 1.01 - to see the last bin
		if(crit==2 && dmaxval > 1) dmaxval =1. ;
	}

	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//. . .build the distribution of average values
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	TH1 *distrib = new TH1F(Form("%sDistr",(const char*)HistoName), "", dnbins, inhisto->GetMinimum(), dmaxval);
	distrib->SetXTitle(inhisto->GetYaxis()->GetTitle());
	distrib->SetYTitle("Entries");
	//distrib->GetXaxis()->SetNdivisions(505);
	//..fill the distribution of avarge cell values
	for (Int_t c = 1; c <= inhisto->GetNbinsX(); c++)
	{
		distrib->Fill(inhisto->GetBinContent(c));
	}
	//..build two dimensional histogram with values row vs. column
	TH2F *Plot2D = new TH2F(Form("%s_HitRowColumn",(const char*)HistoName),Form("%s_HitRowColumn",(const char*)HistoName),fNMaxColsAbs+2,-1.5,fNMaxColsAbs+0.5, fNMaxRowsAbs+2,-1.5,fNMaxRowsAbs+0.5);
	Plot2D->GetXaxis()->SetTitle("cell column (#eta direction)");
	Plot2D->GetYaxis()->SetTitle("cell row (#phi direction)");


	for (Int_t c = 1; c <= inhisto->GetNbinsX(); c++)
	{
		//..Do that only for cell ids also accepted by the code
		if(!fCaloUtils->GetEMCALGeometry()->CheckAbsCellId(c-1))continue;
		//..Get Row and Collumn for cell ID c
		fCaloUtils->GetModuleNumberCellIndexesAbsCaloMap(c-1,0,CellColumn,CellRow,Trash,CellColumnAbs,CellRowAbs);
		if(CellColumnAbs> fNMaxColsAbs || CellRowAbs>fNMaxRowsAbs)
		{
			cout<<"Problem! wrong calculated number of max col and max rows"<<endl;
			cout<<"current col: "<<CellColumnAbs<<", max col"<<fNMaxColsAbs<<endl;
			cout<<"current row: "<<CellRowAbs<<", max row"<<fNMaxRowsAbs<<endl;
		}
		Plot2D->SetBinContent(CellColumnAbs,CellRowAbs,inhisto->GetBinContent(c));
	}

	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//. . .draw histogram + distribution
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

	//Produ
	TCanvas *c1 = new TCanvas(HistoName,HistoName,900,900);
	c1->ToggleEventStatus();
	TPad*    upperPad    = new TPad("upperPad", "upperPad",.005, .5, .995, .995);
	TPad*    lowerPadLeft = new TPad("lowerPadL", "lowerPadL",.005, .005, .5, .5);
	TPad*    lowerPadRight = new TPad("lowerPadR", "lowerPadR",.5, .005, .995, .5);
	upperPad->Draw();
	lowerPadLeft->Draw();
	lowerPadRight->Draw();

	upperPad->cd();
	upperPad->SetLeftMargin(0.045);
	upperPad->SetRightMargin(0.03);
	upperPad->SetLogy();
	inhisto->SetTitleOffset(0.6,"Y");
	inhisto->GetXaxis()->SetRangeUser(0,17000);

	inhisto->SetLineColor(kBlue+1);
	inhisto->Draw();

	lowerPadRight->cd();
	lowerPadRight->SetLeftMargin(0.09);
	lowerPadRight->SetRightMargin(0.06);
	Plot2D->Draw("colz");

	lowerPadLeft->cd();
	lowerPadLeft->SetLeftMargin(0.09);
	lowerPadLeft->SetRightMargin(0.06);
	lowerPadLeft->SetLogy();
	distrib->SetLineColor(kBlue+1);
	distrib->Draw();

	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//. . .fit histogram
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	Int_t higherbin=0,i;
	for(i = 2; i <= dnbins; i++)
	{
		if(distrib->GetBinContent(higherbin) < distrib->GetBinContent(i))  higherbin = i ;
	}
	//..good range is around the max value as long as the
	//..bin content is larger than 2 entries
	for(i = higherbin ; i<=dnbins ; i++)
	{
		if(distrib->GetBinContent(i)<2) break ;
		goodmax = distrib->GetBinCenter(i);
	}
	for(i = higherbin ; i>1 ; i--)
	{
		if(distrib->GetBinContent(i)<2) break ;
		goodmin = distrib->GetBinLowEdge(i);
	}
	//cout<<"higherbin : "<<higherbin<<endl;
	//cout<<"good range : "<<goodmin<<" - "<<goodmax<<endl;

	TF1 *fit2 = new TF1("fit2", "gaus");
	//..start the fit with a mean of the highest value
	fit2->SetParameter(1,higherbin);

	distrib->Fit(fit2, "0LQEM", "", goodmin, goodmax);
	Double_t sig, mean, chi2ndf;
	// Marie midif to take into account very non gaussian distrig
	mean    = fit2->GetParameter(1);
	sig     = fit2->GetParameter(2);
	chi2ndf = fit2->GetChisquare()/fit2->GetNDF();

	if (mean <0.) mean=0.; //ELI is this not a highly problematic case??

	goodmin = mean - Nsigma*sig ;
	goodmax = mean + Nsigma*sig ;

	if (goodmin<0) goodmin=0.;

	cout<<"    o Result of fit: "<<endl;
	cout<<"    o  "<<endl;
	cout<<"    o Mean: "<<mean <<" sigma: "<<sig<<endl;
	cout<<"    o good range : "<<goodmin <<" - "<<goodmax<<endl;

	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//. . .Add info to histogram
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	TLine *lline = new TLine(goodmin, 0, goodmin, distrib->GetMaximum());
	lline->SetLineColor(kGreen+2);
	lline->SetLineStyle(7);
	lline->Draw();

	TLine *rline = new TLine(goodmax, 0, goodmax, distrib->GetMaximum());
	rline->SetLineColor(kGreen+2);
	rline->SetLineStyle(7);
	rline->Draw();

	TLegend *leg = new TLegend(0.60,0.82,0.9,0.88);
	leg->AddEntry(lline, "Good region boundary","l");
	leg->Draw("same");

	fit2->SetLineColor(kOrange-3);
	fit2->SetLineStyle(1);//7
	fit2->Draw("same");

	TLatex* text = 0x0;
	if(crit==1) text = new TLatex(0.2,0.8,Form("Good range: %.2f-%.2f",goodmin,goodmax));
	if(crit==2) text = new TLatex(0.2,0.8,Form("Good range: %.2f-%.2fx10^-5",goodmin*100000,goodmax*100000));
	text->SetTextSize(0.06);
	text->SetNDC();
	text->SetTextColor(1);
	//text->SetTextAngle(angle);
	text->Draw();
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//. . .Save histogram
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	c1->Update();
	gSystem->mkdir(fAnalysisOutput);
	TString name   =Form("%s/criteria-_%d.gif", fAnalysisOutput.Data(), crit);
	if(crit==1)name=Form("%s/AverageEperHit_%s.gif", fAnalysisOutput.Data(), (const char*)HistoName);
	if(crit==2)name=Form("%s/AverageHitperEvent_%s.gif", fAnalysisOutput.Data(), (const char*)HistoName);
	c1->SaveAs(name);


	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//. . . Mark the bad cells in the pflag array
	//. . .(0= bad because cell average value lower than min allowed)
	//. . .(2= bad because cell average value higher than max allowed)
	//. . .(1 by default)
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	cout<<"    o Flag bad cells that are outside the good range "<<endl;
	Int_t cel;
	//..*pflag[1][0] stores the number of cells
	for(Int_t c = 1; c <= *pflag[1][0]; c++)
	{
		cel=(Int_t)(inhisto->GetBinLowEdge(c)+0.1);  //ELI what does that 0.1 stand for?
		//cel=0 and c=1, cel=1 and c=2
		if (inhisto->GetBinContent(c) <= goodmin)
		{
			*pflag[cel][crit]=0;
		}
		else if (inhisto->GetBinContent(c) > goodmax)
		{
			*pflag[cel][crit]=2;
		}
	}
	cout<<"    o "<<endl;

}
//_________________________________________________________________________
//_________________________________________________________________________

void AliAnaCaloChannelAnalysis::TestCellEandN(Int_t *pflag[23040][7], Double_t Emin, Double_t Emax, Double_t Nsigma)
{
	//..here the average hit per event and the average energy per hit is caluclated for each cell.
	Int_t dnbins = 200;
	TH2 *hCellAmplitude = (TH2*) gFile->Get("hCellAmplitude");

	//..binning parameters
	Int_t ncells  = hCellAmplitude->GetNbinsY();
	Double_t amin = hCellAmplitude->GetYaxis()->GetXmin();
	Double_t amax = hCellAmplitude->GetYaxis()->GetXmax();
	cout<<"    o Calculate average cell hit per event and average cell energy per hit "<<endl;

	TH1F *hCellNtotal = new TH1F(Form("hCellNtotal_E%.2f-%.2f",Emin,Emax),Form("Number of hits per events, %.2f < E < %.2f GeV",Emin,Emax), ncells,amin,amax);
	hCellNtotal->SetXTitle("AbsId");
	hCellNtotal->SetYTitle("Av. hits per events");
	hCellNtotal->GetXaxis()->SetNdivisions(505);

	TH1F *hCellEtoNtotal = new TH1F(Form("hCellEtoNtotal_E%.2f-%.2f",Emin,Emax),Form("Average energy per hit, %.2f < E < %.2f GeV",Emin,Emax), ncells,amin,amax);
	hCellEtoNtotal->SetXTitle("AbsId");
	hCellEtoNtotal->SetYTitle("Av. energy per hit, GeV");
	hCellEtoNtotal->GetXaxis()->SetNdivisions(505);

	TH1* hNEventsProcessedPerRun = (TH1*) gFile->Get("hNEventsProcessedPerRun");
	Double_t totalevents = hNEventsProcessedPerRun->Integral(1, hNEventsProcessedPerRun->GetNbinsX());

	//..here the average hit per event and the average energy per hit is caluclated for each cell.
	for (Int_t c = 1; c <= ncells; c++)
	{
		Double_t Esum = 0;
		Double_t Nsum = 0;

		for (Int_t j = 1; j <= hCellAmplitude->GetNbinsX(); j++)
		{
			Double_t E = hCellAmplitude->GetXaxis()->GetBinCenter(j);
			Double_t N = hCellAmplitude->GetBinContent(j, c);
			if (E < Emin || E > Emax) continue;
			Esum += E*N;
			Nsum += N;
		}
		if(totalevents> 0.)hCellNtotal   ->SetBinContent(c, Nsum/totalevents);  //..number of hits per event
		if(Nsum > 0.)      hCellEtoNtotal->SetBinContent(c, Esum/Nsum);         //..average energy per hit
		//ELI maybe plot 2-dimensional hit/event eta vs. phi??
	}
	delete hCellAmplitude;

	if(*pflag[0][0]==1) Process(pflag,hCellEtoNtotal,Nsigma,dnbins,-1);
	if(*pflag[0][0]==2 && Emin==0.5) Process(pflag,hCellNtotal,   Nsigma,dnbins*9000,-1);//ELI I did massivley increase the binning now but it helps a lot
	if(*pflag[0][0]==2 && Emin>0.5)  Process(pflag,hCellNtotal,   Nsigma,dnbins*17,-1);
}

//_________________________________________________________________________
//_________________________________________________________________________

void AliAnaCaloChannelAnalysis::TestCellShapes(Int_t *pflag[23040][7], Double_t fitEmin, Double_t fitEmax, Double_t Nsigma)
{
	//ELI this method is currently not used
	// Test cells shape using fit function f(x)=A*exp(-B*x)/x^2.
	// Produce values per cell + distributions for A,B and chi2/ndf parameters.

	TString hname= "hCellAmplitude";
	Int_t dnbins = 1000;
	TH2 *hCellAmplitude = (TH2*) gFile->Get(Form("%s",(const char*)hname));

	// binning parameters
	Int_t  ncells = hCellAmplitude->GetNbinsY();
	Double_t amin = hCellAmplitude->GetYaxis()->GetXmin();
	Double_t amax = hCellAmplitude->GetYaxis()->GetXmax();
	cout << "ncells " << ncells << " amin = " << amin << "amax = " << amax<< endl;

	// initialize histograms
	TH1 *hFitA = new TH1F(Form("hFitA_%s",(const char*)hname),"Fit A value", ncells,amin,amax);
	hFitA->SetXTitle("AbsId");
	hFitA->SetYTitle("A");

	TH1 *hFitB = new TH1F(Form("hFitB_%s",(const char*)hname),"Fit B value", ncells,amin,amax);
	hFitB->SetXTitle("AbsId");
	hFitB->SetYTitle("B");

	TH1 *hFitChi2Ndf = new TH1F(Form("hFitChi2Ndf_%s",(const char*)hname),"Fit #chi^{2}/ndf value", ncells,amin,amax);
	hFitChi2Ndf->SetXTitle("AbsId");
	hFitChi2Ndf->SetYTitle("#chi^{2}/ndf");

	Double_t maxval1=0., maxval2=0., maxval3=0.;
	Double_t prev=0., MSA=0., AvA = 0. ; //those param are used to automaticaly determined a reasonable maxval1
	Double_t prev2=0., MSB=0., AvB = 0.  ; //those param are used to automaticaly determined a reasonable maxval2
	Double_t prev3=0., MSki2=0., Avki2 = 0. ; //those param are used to automaticaly determined a reasonable maxval3
	Double_t ki2=0.0 ;
	for (Int_t k = 1; k <= ncells; k++)
	{
		TF1 *fit = new TF1("fit", "[0]*exp(-[1]*x)/x^2");
		TH1 *hCell = hCellAmplitude->ProjectionX("",k,k);
		if (hCell->GetEntries() == 0) continue;
		// hCell->Rebin(3);
		hCell->Fit(fit, "0QEM", "", fitEmin, fitEmax);
		delete hCell;

		if(fit->GetParameter(0) < 5000.)
		{
			hFitA->SetBinContent(k, fit->GetParameter(0));
			if(k<3000)
			{
				AvA +=  fit->GetParameter(0);
				if(k==2999)  maxval1  = AvA/3000. ;
				if (prev < fit->GetParameter(0)) MSA += fit->GetParameter(0) - prev;
				else MSA -= (fit->GetParameter(0) - prev) ;
				prev = fit->GetParameter(0);
			}
			else
			{
				if((fit->GetParameter(0) - maxval1) > 0. && (fit->GetParameter(0) - maxval1) < (MSA/1000.))
				{
					maxval1 = fit->GetParameter(0);
				}
			}
		}
		else hFitA->SetBinContent(k, 5000.);

		if(fit->GetParameter(1) < 5000.)
		{
			hFitB->SetBinContent(k, fit->GetParameter(1));
			if(k<3000)
			{
				AvB +=  fit->GetParameter(1);
				if(k==2999)  maxval2  = AvB/3000. ;
				if (prev2 < fit->GetParameter(1)) MSB += fit->GetParameter(1) - prev2;
				else MSB -= (fit->GetParameter(1) - prev2) ;
				prev2 = fit->GetParameter(1);
			}
			else
			{
				if((fit->GetParameter(1) - maxval2) > 0. && (fit->GetParameter(1) - maxval2) < (MSB/1000.))
				{
					maxval2 = fit->GetParameter(1);
				}
			}
		}
		else hFitB->SetBinContent(k, 5000.);


		if (fit->GetNDF() != 0 ) ki2 =  fit->GetChisquare()/fit->GetNDF();
		else ki2 = 1000.;

		if(ki2 < 1000.)
		{
			hFitChi2Ndf->SetBinContent(k, ki2);
			if(k<3000)
			{
				Avki2 +=  ki2;
				if(k==2999)  maxval3  = Avki2/3000. ;
				if (prev3 < ki2) MSki2 += ki2 - prev3;
				else MSki2 -= (ki2 - prev3) ;
				prev3 = ki2;
			}
			else
			{
				if((ki2 - maxval3) > 0. && (ki2 - maxval3) < (MSki2/1000.))
				{
					maxval3 = ki2;
				}
			}
		}
		else hFitChi2Ndf->SetBinContent(k, 1000.);

		delete fit ;
	}

	delete hCellAmplitude;

	// if you have problem with automatic parameter :
	//  maxval1 =
	//  maxval2 =
	//  maxval3 =
	if(*pflag[0][0]==3)
		Process(pflag, hFitChi2Ndf, Nsigma, dnbins, maxval3);
	if(*pflag[0][0]==4)
		Process(pflag, hFitA, Nsigma, dnbins,  maxval1);
	if(*pflag[0][0]==5)
		Process(pflag, hFitB, Nsigma, dnbins, maxval2);
}
//_________________________________________________________________________
//_________________________________________________________________________
void AliAnaCaloChannelAnalysis::ExcludeCells(Int_t *pexclu[23040], Int_t NrCells)
{
	//..This function finds cells with zero entries
	//..to exclude them from the analysis
	//cout<<"In ExcludeCells: Name of current file: "<<gFile->GetName()<<endl;
	TH2 *hCellAmplitude = (TH2*) gFile->Get("hCellAmplitude");

	Int_t SumOfExcl=0;

	//..Direction of cell ID
	for (Int_t c = 1; c <= NrCells; c++)
	{
		Double_t Nsum = 0;
		//..Direction of amplitude
		for (Int_t amp = 1; amp <= hCellAmplitude->GetNbinsX(); amp++)
		{
			Double_t N = hCellAmplitude->GetBinContent(amp,c);
			Nsum += N;
		}
		//..If the amplitude in one cell is basically 0
		//..mark the cell as excluded
		//ELI I just wonder how you can have less than one but more than 0.5
		//shouldnt everything below 1 be excluded?
		if(Nsum >= 0.5 && Nsum < 1)cout<<"-----------------------small but non zero!!!!"<<endl;
		if(Nsum < 0.5 && Nsum != 0)cout<<"-----------------------non zero!!!!"<<endl;

		if(Nsum < 0.5 && *pexclu[c-1]!=5)
		{
			//..histogram bin=cellID+1
			*pexclu[c-1]=1;
			SumOfExcl++;
		}
		else *pexclu[c-1]=0;
	}
	delete hCellAmplitude;
	cout<<"    o Number of excluded cells: "<<SumOfExcl<<endl;
	cout<<"     ("<<SumOfExcl<<")"<<endl;
}

//_________________________________________________________________________
//_________________________________________________________________________
void AliAnaCaloChannelAnalysis::KillCells(Int_t filter[], Int_t nbc)
{
	cout<<"    o Kill cells -> set bin content of "<<nbc<<" bad cells to 0 "<<endl;
	// kill a cell : put its entry to 0
	TH2 *hCellAmplitude          = (TH2*) gFile->Get("hCellAmplitude");
	TH1* hNEventsProcessedPerRun = (TH1*) gFile->Get("hNEventsProcessedPerRun");

	//..loop over number of identified bad cells. ID is stored in filer[] array
	for(Int_t i =0; i<nbc; i++)
	{
		if(filter[i]==-1)cout<<"#### That is strange - shouln't happen"<<endl;
		//..set all amplitudes for a given cell to 0
		for(Int_t amp=0; amp<= hCellAmplitude->GetNbinsX() ;amp++)
		{
			//(amplitiude,cellID,new value)
			//..CellID=0 is stored in bin1 so we need to shift+1
			hCellAmplitude->SetBinContent(amp,filter[i]+1,0);
		}
	}
	gSystem->mkdir(fAnalysisOutput);
//	TFile *tf = new TFile(Form("%s/filter.root", fAnalysisOutput.Data()),"recreate");
	TFile *tf = new TFile(fFilteredFileName.Data(),"recreate");

	hCellAmplitude->Write();
	hNEventsProcessedPerRun->Write();
	tf->Write();
	tf->Close();
	delete hCellAmplitude;
	delete hNEventsProcessedPerRun;
}
//_________________________________________________________________________
//_________________________________________________________________________
void AliAnaCaloChannelAnalysis::PeriodAnalysis(Int_t criterum, Double_t Nsigma, Double_t Emin, Double_t Emax)
{
	cout<<""<<endl;
	cout<<""<<endl;
	cout<<""<<endl;
	cout<<"o o o o o o o o o o o o o o o o o o o o o o  o o o"<<endl;
	cout<<"o o o PeriodAnalysis for flag "<<criterum<<" o o o"<<endl;
	cout<<"o o o Done in the energy range E "<<Emin<<"-"<<Emax<<endl;
	TH2 *hCellAmplitude = (TH2*) gFile->Get("hCellAmplitude");

	//..This function does perform different checks depending on the given criterium variable
	//..diffrent possibilities for criterium are:
	// 1 : average E for E>Emin
	// 2 : entries for E>Emin
	// 3 : ki²/ndf  (from fit of each cell Amplitude between Emin and Emax)
	// 4 : A parameter (from fit of each cell Amplitude between Emin and Emax)
	// 5 : B parameter (from fit of each cell Amplitude between Emin and Emax)
	// 6 :
	// 7 : give bad + dead list
	//ELI Number of cells - (24*48)  16 full modules and 4 1/3 modules == 19,968 check that number!!
	static const Int_t NrCells=17663;//19968; //17665;//23040;  //ELI this is in fact a very important number!!

	//ELI a comment about the array positions
	//..In the histogram: bin 1= cellID 0, bin 2= cellID 1 etc
	//..In the arrays: array[cellID]= some information
	Int_t newBC[NrCells];       // starts at newBC[0] stores cellIDs  (cellID = bin-1)
	Int_t newDC[NrCells];       // starts at newDC[0] stores cellIDs  (cellID = bin-1)
	Int_t *pexclu[NrCells];     // starts at 0 pexclu[CellID] stores 0 not excluded, 1 excluded
	Int_t exclu[NrCells];       // is the same as above
	Int_t *pflag[NrCells][7];   // pflag[cellID][crit] = 1(ok),2(bad),0(bad)     start at 0 (cellID 0 = histobin 1)
	Int_t flag[NrCells][7];     // is the same as above
	//..set all fields to -1
	memset(newBC,-1, NrCells *sizeof(int));
	memset(newDC,-1, NrCells *sizeof(int));

	Int_t CellID, nb1=0, nb2=0;
	//INIT
	TString output, bilan, DeadPdfName, BadPdfName;
	for(CellID=0;CellID<NrCells;CellID++)
	{
		exclu[CellID] =0;
		pexclu[CellID]=&exclu[CellID];
		for(Int_t j=0;j<7;j++)
		{
			flag[CellID][j] =1;
			pflag[CellID][j]=&flag[CellID][j];
		}
	}
	//..Side note [x][y=0] is never used as there is no criterium 0
	//..use the [0][0] to store the criterum that is tested
	flag[0][0]=criterum ;
	//..use the [0][1] to store the number of cells tested
	flag[1][0]=NrCells;

	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//.. CELLS EXCLUDED
	//.. exclude cells from analysis (will not appear in results)
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	cout<<"o o o Exclude Cells o o o"<<endl;
	ExcludeCells(pexclu,NrCells);


	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//.. ANALYSIS
	//.. Build average distributions and fit them
	//.. Three tests for bad cells:
	//.. 1) Average energy per hit;
	//.. 2) Average hit per event;
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	if(criterum < 6)cout<<"o o o Analyze average cell distributions o o o"<<endl;
	//..For case 1 or 2
	if (criterum < 3)      TestCellEandN(pflag, Emin, Emax,Nsigma);
	//..For case 3, 4 or 5
	else if (criterum < 6) TestCellShapes(pflag, Emin, Emax, Nsigma);


	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//.. RESULTS
	//.. 1) Print the bad cells
	//..    and write the results to a file
	//.. 2) Kill cells function...
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	if(criterum < 6)
	{
		//..Print the results on the screen and
		//..write the results in a file
		output.Form("%s/Criterion%d_Emin-%.2f_Emax-%.2f.txt", fAnalysisOutput.Data(), criterum,Emin,Emax);
		ofstream file(output, ios::out | ios::trunc);
		if(!file)
		{
			cout<<"#### Major Error. Check the textfile!"<<endl;
		}
		file<<"Criterion : "<<criterum<<", Emin = "<<Emin<<" GeV"<<", Emax = "<<Emax<<" GeV"<<endl;
		file<<"Bad by lower value : "<<endl;
		cout<<"    o bad cells by lower value (for cell E between "<<Emin<<"-"<<Emax<<")"<<endl;
		cout<<"      ";
		nb1=0;
		for(CellID=0;CellID<NrCells;CellID++)
		{
			if(flag[CellID][criterum]==0 && exclu[CellID]==0)
			{
				newBC[nb1]=CellID;
				nb1++;
				file<<CellID<<", ";
				cout<<CellID<<",";
			}
		}
		file<<"("<<nb1<<")"<<endl;
		cout<<"("<<nb1<<")"<<endl;
		file<<"Bad by higher value : "<<endl;
		cout<<"    o bad cells by higher value (for cell E between "<<Emin<<"-"<<Emax<<")"<<endl;
		cout<<"      ";
		nb2=0;
		for(CellID=0;CellID<NrCells;CellID++)
		{
			if(flag[CellID][criterum]==2 && exclu[CellID]==0)
			{
				newBC[nb1+nb2]=CellID;
				nb2++;
				file<<CellID<<", ";
				cout<<CellID<<",";
			}
		}
		file<<"("<<nb2<<")"<<endl;
		cout<<"("<<nb2<<")"<<endl;

		file<<"Total number of bad cells"<<endl;
		file<<"("<<nb1+nb2<<")"<<endl;
		file.close();
		cout<<"    o Total number of bad cells "<<endl;
		cout<<"      ("<<nb1+nb2<<")"<<endl;

		//..create a filtered file
		KillCells(newBC,nb1+nb2) ;
	}

	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	//.. CRITERUM 7 : FINAL RESULT
	//.. 1) summarize all dead and bad cells in a text file
	//.. 2) plot all bad cell E distributions in a .pdf file
	//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	if(criterum ==7)
	{

		DeadPdfName = Form("%s/%s%sDC_SummaryResults_V%i.pdf", fAnalysisOutput.Data(), fPeriod.Data(), fPass.Data(), fTrial);
		BadPdfName  = Form("%s/%s%sBC_SummaryResults_V%i.pdf", fAnalysisOutput.Data(), fPeriod.Data(), fPass.Data(), fTrial);
		bilan       = Form("%s/%s%sBC_SummaryResults_V%i.txt", fAnalysisOutput.Data(), fPeriod.Data(), fPass.Data(), fTrial); ;
		cout<<"    o Final results o "<<endl;
		cout<<"    o write results into .txt file: "<<bilan<<endl;
		cout<<"    o write results into .pdf file: "<<BadPdfName<<endl;
		ofstream file(bilan, ios::out | ios::trunc);
		if(file)
		{
			file<<"Dead cells : "<<endl;
			cout<<"    o Dead cells : "<<endl;
			nb1 =0;
			for(CellID=0; CellID<NrCells; CellID++)
			{
				if(exclu[CellID]==1)
				{
					newDC[nb1]=CellID;
					file<<CellID<<"\n" ;
					cout<<CellID<<"," ;
					exclu[CellID]=5;
					nb1++;
				}
			}
			file<<"("<<nb1<<")"<<endl;
			cout<<"("<<nb1<<")"<<endl;

			//TFile::Open(Form("%s/filter.root", fAnalysisOutput.Data()));
			TFile::Open(fFilteredFileName.Data());

			ExcludeCells(pexclu,NrCells);
			file<<"Bad cells (excluded): "<<endl;
			cout<<"    o Total number of bad cells (excluded): "<<endl;
			nb2=0;
			for(CellID=0;CellID<NrCells;CellID++)
			{
				if(exclu[CellID]==1)
				{
					newBC[nb2]=CellID;
					file<<CellID<<"\n" ;
					cout<<CellID<<"," ;
					nb2++;
				}
			}
			file<<"("<<nb2<<")"<<endl;
			cout<<"("<<nb2<<")"<<endl;
		}
		file.close();

		cout<<"    o Open original file: "<<fMergedFileName<<endl;
		TFile::Open(fMergedFileName);
		Int_t c;
		//..loop over the bad cells in packages of 9
		cout<<"    o Save the Dead channel spectra to a .pdf file"<<endl;
		for(Int_t w=0; (w*9)<nb1; w++)
			//nb1=100;
			//for(Int_t w=0; (w*9)<nb1; w++)
		{
			if(9<=(nb1-w*9)) c = 9 ;
			else c = nb1-9*w ;
			SaveBadCellsToPDF(newDC, w*9, c,DeadPdfName);
		}

		TFile::Open(fMergedFileName);
		nb2--;
		cout<<"    o Save the bad channel spectra to a .pdf file,nbch: "<< nb2<<endl;
		for(Int_t w=0; (w*9)<nb2; w++)
			//for(Int_t w=0; (w*9)<1; w++)
			//for(Int_t w=0; (w*9)<10; w++)
		{
			if(9<=(nb2-w*9)) c = 9 ;
			else c = nb2-9*w ;
			SaveBadCellsToPDF(newBC, w*9, c,BadPdfName) ;
		}

	}

}

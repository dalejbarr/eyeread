#include <Rcpp.h>
#include "Eyedat.hpp"
#include <fstream>
#include <iostream>
#include <cstring>

using namespace Rcpp;
using namespace std;

/////////////////////////////////////////////////
// Generic functions
/////////////////////////////////////////////////

Eyedat::Eyedat() {
  m_ulExpectedSize = 0;
}

/////////////////////////////////////////////////
// Eyelink functions
/////////////////////////////////////////////////

EyedatEyelink::EyedatEyelink() : Eyedat() {
	m_pEDFFile = NULL;
	m_nTrials = 0;
	m_uiBookmarkStart =0;
}

int EyedatEyelink::GotoTrial(int i) {
  BOOKMARK bm;
  bm.id = m_uiBookmarks[i];
	if (edf_goto_bookmark(m_pEDFFile, &bm)) { // error!
		cout << "error reading from file" << endl;
		CloseFile();
		return -1;
	} else {}
}

int EyedatEyelink::ReturnToBeginningOfFile() {
	BOOKMARK bm;
	bm.id = m_uiBookmarkStart;
	edf_goto_bookmark(m_pEDFFile, &bm);
}

int EyedatEyelink::OpenFile(const char *fname) {
  int errval = 0;

	m_pEDFFile = edf_open_file(fname, 0, 1, 1, &errval);

	if (m_pEDFFile == NULL) {
		cout << "file open returned error " << errval << endl;
		return -1;
	} else {}
	//cout << "file opened successfully" << endl;
	
	//m_nTrials = edf_get_trial_count(m_pEDFFile);
	//cout << m_nTrials << " trials" << endl;	

	BOOKMARK bm;
	edf_set_bookmark(m_pEDFFile, &bm);
	m_uiBookmarkStart = bm.id;

	if (edf_jump_to_trial(m_pEDFFile, 0)) {
		cout << "error reading trial info from file" << endl;
		return -1;
	} else {}

	TRIAL trialinf;
	edf_get_trial_header(m_pEDFFile, &trialinf);
	m_eye = trialinf.rec->eye;
	switch(m_eye) {
	case 1 :
		//cout << "recorded from left eye" << endl;
		break;
	case 2 :
		//cout << "recorded from right eye" << endl;
		break;
	case 3 :
		//cout << "recorded from both eyes" << endl;
		break;
	default :
		cout << "error! eye flag not defined!" << endl;
	}

	ReturnToBeginningOfFile();
	
	return 0;
}

int EyedatEyelink::AddMessage(int i, unsigned int sttime, int nRead, const char *pMsg) {
	m_viTrial.push_back(i+1);
	m_viTime.push_back(sttime);
	m_viF.push_back(nRead+1);
	m_vsMessage.push_back(pMsg);
}

int EyedatEyelink::LoadMessages(const char *fname, const char *tbegin, const char *tend) {
	ALLF_DATA * pFD = NULL;
	bool bRead = true;
	int nRead = 0;
	int nType = 0;
	char * pChar = NULL;

	if (OpenFile(fname)) {
    return -1;
	} else {}

  FindTrialDelimiters(tbegin, tend);

	for (int i=0; i < m_nTrials; i++) {
		GotoTrial(i);
		AddMessage(i, m_uiTrialBegin[i], 0, m_vsTrialMsg[i].c_str());
		//cout << "Trial " << i+1 << ": "<< m_uiTrialBegin[i] << ' ' << 
		//m_uiTrialEnd[i] << ' ';
		bRead = true;
		nRead = 1;
		while (bRead) {
			nType = edf_get_next_data(m_pEDFFile);
			switch (nType) {
			case MESSAGEEVENT :
				pFD = edf_get_float_data(m_pEDFFile);
				if (pFD->fe.sttime <= m_uiTrialEnd[i]) {
					AddMessage(i, pFD->fe.sttime, nRead, &(pFD->fe.message->c));
					/*
					m_viTrial.push_back(i+1);
					m_viTime.push_back(pFD->fe.sttime);
					m_viF.push_back(nRead+1);
					pChar = &(pFD->fe.message->c);
					m_vsMessage.push_back(pChar);
					*/
					nRead++;
				} else {
					bRead = false;
				}
				if (pFD->fe.sttime == m_uiTrialEnd[i]) {
					bRead = false;
				} else {}
				break;
			case NO_PENDING_ITEMS :
				bRead = false;
				if (i < (m_nTrials-1)) {
					cout << "error: reached EOF at trial " << i+1 << endl;
					CloseFile();
					return -1;
				} else {}
				break;
			} // matches switch
		} // matches while
		//cout << ' ' << nRead << endl;		
	}

	CloseFile();
	
	return 0;
}

int EyedatEyelink::CloseFile() {
	if (m_pEDFFile != NULL) {
		edf_close_file(m_pEDFFile);	
	} else {}
	return 0;
}

int EyedatEyelink::LoadTrial(int i) {
	ALLF_DATA * pFD = NULL;
	int nType = 0;
	bool bRead = true;
	int nRead = 0;

	GotoTrial(i);
	bRead = true;
	nRead = 0;
	while (bRead) {
		nType = edf_get_next_data(m_pEDFFile);
		switch (nType) {
		case SAMPLE_TYPE :
			pFD = edf_get_float_data(m_pEDFFile);
			if (pFD->fs.time <= m_uiTrialEnd[i]) {
				if (m_eye>=1 && m_eye<=3) {
					m_viTrial.push_back(i+1);
					m_viTime.push_back(pFD->fs.time);
					m_viF.push_back(nRead+1);
				} else {}
				if (m_eye==1 || m_eye==3) {
					m_vfLX.push_back(pFD->fs.gx[0]);
					m_vfLY.push_back(pFD->fs.gy[0]);
				} else {}
				if (m_eye==2 || m_eye==3) {
					m_vfRX.push_back(pFD->fs.gx[1]);
					m_vfRY.push_back(pFD->fs.gy[1]);
				} else {}
				nRead++;
			} else {
				bRead = false;
			}
			if (pFD->fs.time == m_uiTrialEnd[i]) {
				bRead = false;
			} else {}
			break;
		case NO_PENDING_ITEMS :
			bRead = false;
			if (i < (m_nTrials-1)) {
				cout << "error: reached EOF at trial " << i+1 << endl;
				CloseFile();
				return -1;
			} else {}
			break;
		} // matches switch
	} // matches while
		//cout << ' ' << nRead << endl;
	return 0;
}

int EyedatEyelink::LoadSamples(const char *fname, const char *tbegin, const char *tend) {

	if (OpenFile(fname)) {
		return -1;
	} else {}

	FindTrialDelimiters(tbegin, tend);

	for (int i=0; i < m_nTrials; i++) {
		LoadTrial(i);
		/*
		GotoTrial(i);
		bRead = true;
		nRead = 0;
		while (bRead) {
			nType = edf_get_next_data(m_pEDFFile);
			switch (nType) {
			case SAMPLE_TYPE :
				pFD = edf_get_float_data(m_pEDFFile);
				if (pFD->fs.time <= m_uiTrialEnd[i]) {
					if (m_eye>=1 && m_eye<=3) {
						m_viTrial.push_back(i+1);
						m_viTime.push_back(pFD->fs.time);
						m_viF.push_back(nRead+1);
					} else {}
					if (m_eye==1 || m_eye==3) {
						m_vfLX.push_back(pFD->fs.gx[0]);
						m_vfLY.push_back(pFD->fs.gy[0]);
					} else {}
					if (m_eye==2 || m_eye==3) {
						m_vfRX.push_back(pFD->fs.gx[1]);
						m_vfRY.push_back(pFD->fs.gy[1]);
					} else {}
					nRead++;
				} else {
					bRead = false;
				}
				if (pFD->fs.time == m_uiTrialEnd[i]) {
					bRead = false;
				} else {}
				break;
			case NO_PENDING_ITEMS :
				bRead = false;
				if (i < (m_nTrials-1)) {
					cout << "error: reached EOF at trial " << i+1 << endl;
					CloseFile();
					return -1;
				} else {}
				break;
			} // matches switch
		} // matches while
		//cout << ' ' << nRead << endl;
		*/
	} // matches for loop
	CloseFile();
	return 0;
}

DataFrame EyedatEyelink::BuildDataFrame() {
	DataFrame df;
	long lFrames = m_viF.size();
	IntegerVector tseq = IntegerVector(m_viTrial.begin(), m_viTrial.end());
	IntegerVector fid = IntegerVector(m_viF.begin(), m_viF.end());
	IntegerVector msec = IntegerVector(m_viTime.begin(), m_viTime.end());
	if ((m_vfLX.size() == lFrames) && (m_vfRX.size() == lFrames)) {
		df = DataFrame::create(Named("TSeq")=tseq,
													 Named("FrameID")=fid,
													 Named("Msec")=msec,
													 Named("lx")=NumericVector(m_vfLX.begin(), m_vfLX.end()),
													 Named("ly")=NumericVector(m_vfLY.begin(), m_vfLY.end()),
													 Named("rx")=NumericVector(m_vfRX.begin(), m_vfRX.end()),
													 Named("ry")=NumericVector(m_vfRY.begin(), m_vfRY.end()) );
	} else if (m_vfLX.size() == lFrames) {
		df = DataFrame::create(Named("TSeq")=tseq,
													 Named("FrameID")=fid,
													 Named("Msec")=msec,
													 Named("x")=NumericVector(m_vfLX.begin(), m_vfLX.end()),
													 Named("y")=NumericVector(m_vfLY.begin(), m_vfLY.end()) );
	} else if (m_vfRX.size() == lFrames) {
		df = DataFrame::create(Named("TSeq")=tseq,
													 Named("FrameID")=fid,
													 Named("Msec")=msec,
													 Named("x")=NumericVector(m_vfRX.begin(), m_vfRX.end()),
													 Named("y")=NumericVector(m_vfRY.begin(), m_vfRY.end()) );
	} else {
		df = DataFrame::create(Named("FrameID")=IntegerVector(m_viF.begin(), m_viF.end()));
	}
			
	return df;
}

/////////////////////////////////////////////////
// functions exported to R
/////////////////////////////////////////////////

// [[Rcpp::export]]
DataFrame readEyelinkSamples(CharacterVector cv, CharacterVector cvBegin, CharacterVector cvEnd) {
  string fname, tbegin, tend;
  EyedatEyelink edtEyelink;

  fname.assign(cv[0]);
	tbegin.assign(cvBegin[0]);
	tend.assign(cvEnd[0]);

	edtEyelink.LoadSamples(fname.c_str(), tbegin.c_str(), tend.c_str());

	return edtEyelink.BuildDataFrame();
}

// [[Rcpp::export]]
DataFrame readEyelinkMessages(CharacterVector cv, CharacterVector cvBegin, CharacterVector cvEnd) {
  string fname, tbegin, tend;
  EyedatEyelink edtEyelink;

  fname.assign(cv[0]);
	tbegin.assign(cvBegin[0]);
	tend.assign(cvEnd[0]);

	edtEyelink.LoadMessages(fname.c_str(), tbegin.c_str(), tend.c_str());

	IntegerVector tseq = IntegerVector(edtEyelink.m_viTrial.begin(), edtEyelink.m_viTrial.end());
	IntegerVector fid = IntegerVector(edtEyelink.m_viF.begin(), edtEyelink.m_viF.end());
	IntegerVector msec = IntegerVector(edtEyelink.m_viTime.begin(), edtEyelink.m_viTime.end());
	DataFrame df = DataFrame::create(Named("TSeq")=tseq,
												 Named("MsgID")=fid,
												 Named("Msec")=msec,
												 Named("Msg")=CharacterVector(edtEyelink.m_vsMessage.begin(), edtEyelink.m_vsMessage.end()) );
	return df;
}

// [[Rcpp::export]]
DataFrame readTrialDelimiters(CharacterVector cvFname, CharacterVector cvBegin, CharacterVector cvEnd) {
	std::string fname, tbegin, tend;
  EyedatEyelink edtEyelink;
	fname.assign(cvFname[0]);
	tbegin.assign(cvBegin[0]);
	tend.assign(cvEnd[0]);
	edtEyelink.OpenFile(fname.c_str());

	edtEyelink.FindTrialDelimiters(tbegin.c_str(), tend.c_str());
	IntegerVector ivec = IntegerVector(edtEyelink.m_nTrials);
	for (int i = 0; i < edtEyelink.m_nTrials; i++) {
		ivec[i] = i+1;
	}
	DataFrame df = DataFrame::create(Named("TSeq")=ivec,
																	 Named("msBegin")=IntegerVector(edtEyelink.m_uiTrialBegin.begin(), 
																																	edtEyelink.m_uiTrialBegin.end()),
																	 Named("msEnd")=IntegerVector(edtEyelink.m_uiTrialEnd.begin(), 
																																edtEyelink.m_uiTrialEnd.end()));
	return df;
}

// [[Rcpp::export]]
DataFrame readEyelinkTrials(IntegerVector trials, CharacterVector cv, CharacterVector cvBegin,
														CharacterVector cvEnd) {
	std::string fname, tbegin, tend;
  EyedatEyelink edtEyelink;
	fname.assign(cv[0]);
	tbegin.assign(cvBegin[0]);
	tend.assign(cvEnd[0]);
	edtEyelink.OpenFile(fname.c_str());

	edtEyelink.FindTrialDelimiters(tbegin.c_str(), tend.c_str());
	for (int i = 0; i < trials.size(); i++) {
		edtEyelink.LoadTrial(trials[i]-1);
	}

	return edtEyelink.BuildDataFrame();
}


int EyedatEyelink::FindTrialDelimiters(const char *tbegin, const char *tend) {
	std::string str, strBegin, strEnd;
	char * pChar = NULL;
	bool bRead = true;
  bool bInTrial = false;
	int nType = 0;
	ALLF_DATA * pFD = NULL;
  BOOKMARK bm;

	strBegin.assign(tbegin);
  strEnd.assign(tend);
	ReturnToBeginningOfFile();
	m_nTrials = 0;
  //cout << "delimiter is '"<< strBegin << "'" << endl;

	while (bRead) {
		nType = edf_get_next_data(m_pEDFFile);
		switch (nType) {
		case MESSAGEEVENT :
			pFD = edf_get_float_data(m_pEDFFile);
			str.assign(&(pFD->fe.message->c));
			if (str.substr(0, std::strlen(strBegin.c_str()))==strBegin) {
				edf_set_bookmark(m_pEDFFile, &bm);
        if (bInTrial) {
           cout << "Error!" << endl;
        } else {
          bInTrial = true;
        }
				//cout << m_nTrials+1 << ' ' << pFD->fe.sttime << ' ' << str << ' ';
        m_uiBookmarks.push_back(bm.id);
        m_uiTrialBegin.push_back(pFD->fe.sttime);
				m_vsTrialMsg.push_back(str);
				m_nTrials++;
			} else if (str.substr(0, std::strlen(strEnd.c_str()))==strEnd) {
        if (!bInTrial) {
          cout << "error!!!" << endl;
        } else {
          bInTrial = false;
          //cout << pFD->fe.sttime << endl;
          m_uiTrialEnd.push_back(pFD->fe.sttime);
        }
      }
      break;
		case NO_PENDING_ITEMS :
			bRead = false;
			//cout << "reached EOF" << endl;
			break;
		} // matches switch
	} // matches while
	//cout << m_nTrials << " trials" << endl;
	return 0;
}

#include <vector>
#include "Eyelink/edf.h"
#include <string>
#include <Rcpp.h>

using std::vector;

class Eyedat {
protected:
  unsigned long m_ulExpectedSize;
public: 
  vector<int> m_viX;
  vector<int> m_viY;
  vector<int> m_viF;
  Eyedat();
  int alloc(unsigned long ulSize);
  void resize(unsigned long ul);
  void storeValues(unsigned long frameID, int x, int y);
 };

class EyedatEyelink : public Eyedat {
protected:
  EDFFILE * m_pEDFFile;
public: 
	int m_nTrials;
	char m_pcStartTrial[1024];
	char m_pcEndTrial[1024];
	vector<int> m_viTrial;
	vector<unsigned long> m_viTime;
	vector<float> m_vfLX;
	vector<float> m_vfLY;
	vector<float> m_vfRX;
	vector<float> m_vfRY;
	vector<std::string> m_vsMessage;
	vector<std::string> m_vsTrialMsg;
	vector<unsigned int> m_uiBookmarks;
  vector<unsigned int> m_uiTrialBegin;
  vector<unsigned int> m_uiTrialEnd;
	unsigned int m_uiBookmarkStart;
	byte m_eye;
	//public:
	EyedatEyelink();
	int LoadTrial(int i);
	int OpenFile(const char *fname);
	int CloseFile();
	int GotoTrial(int i);
	int LoadMessages(const char *fname, const char *tbegin, const char *tend);
	int LoadSamples(const char *fname, const char *tbegin, const char *tend);
	int FindTrialDelimiters(const char *, const char *);
	int ReturnToBeginningOfFile();
	int AddMessage(int, unsigned int, int, const char *);
	Rcpp::DataFrame BuildDataFrame();
};

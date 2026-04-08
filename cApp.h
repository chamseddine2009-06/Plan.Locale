#ifndef CAPP
#define CAPP
#include <wx/wx.h>
#include "utils.hpp"
#include "server.h"
#include "cMain.h"

extern asio::io_context* contextIN; //platform specific interface, this is the space of asio to work , and its job list, as my indrstanding
extern asio::io_context* contextOUT; 

class cApp : public wxApp
{
private:
	cMainLogIn* login = nullptr;
	std::thread IContextThr;
	std::thread OContextThr;
public:
	cMain* frame1 = nullptr;
	
	cApp();
	~cApp();
	virtual bool OnInit();
};
#endif

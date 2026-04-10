#include "cApp.h"
#include "login.hpp"
#include "server.h"
#include "utils.hpp"
#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address.hpp>
#include <thread>
#include <unistd.h>
#include "soundIO.hpp"
asio::io_context* contextIN; //platform specific interface, this is the space of asio to work , and its job list, as my indrstanding
asio::io_context* contextOUT;
asio::ip::tcp::acceptor* acceptor;
cApp::cApp(){
}

cApp::~cApp(){
	//soundIOStop();
	for(int i = 0 ; i < cone.size() ; i++){
		cone[i]->sendClose();
	}
	for(int i = servers.size()-1 ; i>=0 ; i--){
		servers[i]->close();
	}
	for(int i = cone.size()-1 ; i>=0 ; i--){
		cone[i]->Close();
	}
	contextIN->stop();//thank you for your sevice , ASIO
	contextOUT->stop();
	if(IContextThr.joinable()){
		IContextThr.join();//join the context thread
	}
	if(OContextThr.joinable()){
		OContextThr.join();//join the context thread
	}
	//servers.clear();
	//cone.clear();


}
bool cApp::OnInit(){
		
	contextOUT = new asio::io_context();
	contextIN = new asio::io_context();
	
	acceptor = new ip::tcp::acceptor(*contextIN,asio::ip::tcp::endpoint(asio::ip::tcp::v4() , LISNT_PORT));//accept conections
	

	auto workI = make_work_guard(*contextIN);
	auto workO = make_work_guard(*contextOUT);
	
		

	
	
	IContextThr = std::thread([&](){contextIN->run();});
	OContextThr = std::thread([&](){contextOUT->run();});	
	
	ClientHandl(*acceptor, *contextIN,*contextOUT);
	
	
	

	//std::cout<<acceptor.local_endpoint().address();
	//asio::ip::tcp::socket skt(context);

	//skt.connect(asio::ip::tcp::endpoint(ip::make_address("0.0.0.0"),LISNT_PORT));
	//logMsgs(skt.local_endpoint().address().to_string(), "");
	if(needLogin()){
		login = new cMainLogIn();
		login->Show();
	}else{
		
		updateLogs();
		//std::thread([&](){		
			dicover(*contextOUT, networkDiscoverBase, networkDiscoverEnd);
		//}).detach();
		soundIOInit();
		frame1 = new cMain();
		frame1->Show();
		
	}
	return true;
}

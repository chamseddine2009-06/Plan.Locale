#include <wx/wx.h>

#include "cApp.h"
#include "cMain.h"
#include "login.hpp"
#include "utils.hpp"
#include <asio/buffer.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>

#include <asio/system_error.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
//#include <QApplication>
#include <string>
#include <sys/socket.h>
#include <unistd.h> //sleep
#include <system_error>
#include <thread>
#include <valarray>
#include <vector>



#include "server.h"
#include "config.hpp"

//using namespace std;





using namespace asio;





void EchoIp(io_context & contx);
bool IsTargitRuning(ip::tcp::socket & skt);
void msgCon(io_context& context);





wxIMPLEMENT_APP(cApp);

/*
void EchoIp(io_context & contx){
	std::string hostname = ip::host_name();
	std::cout << "Host Name : "<<hostname <<"\nYour adress is : ";

	ip::tcp::resolver reslv(contx);//this can resolve ips and much of hulpful things form the host name
	ip::tcp::resolver::results_type endpoints = reslv.resolve(hostname, ""); //resolve the ip adress frome the host name , this is may be useful!


	std::string HostIP;
	for(auto &endp : endpoints){
			if(endp.endpoint().address().is_v4()){
			HostIP=endp.endpoint().address().to_string();
		}
	}
	std::cout<<HostIP<<"\nAnd your name is " << user_name << "\n";


}
/*bool IsTargitRuning(ip::tcp::socket & skt){
	std::error_code ec;
	Message* ping = (Message*)malloc( (1024 + sizeof(Message)) & 1024 );
	ping->Mgic=MAGIC;
	ping->TYPE=PING;
	skt.write_some(buffer((char*)ping , sizeof(Message) ),ec);
	std::cout << "\n---Wating for pong---\n";
	skt.wait(skt.wait_read);
	std::cout << "\nDone";
	skt.read_some(buffer((char*)ping  , sizeof(Message)) , ec);
	if(ping->TYPE != PONG || ec){
		free(ping);
		return false;
	}
	std::string targetName = ping->MES;
	std::cout << "\n" << "Device Name : "  ;
	for(int i = 0 ; i < 20 ; i++)std::cout<<ping->MES[i];;
	free(ping);
	return true;
}
*/

/*void msgCon(io_context& context){
	
	std::error_code ec;

connecting:
	//ip::tcp::socket socket(context);//creat a context

	std::cout<<"\nIP adress of the target device : ";

	std::string targIp="127.0.0.1";

	std::cin>>targIp;
	try{
		ip::tcp::endpoint endp(ip::make_address(targIp,ec),LISNT_PORT);
		/*if(!socket.is_open()){
			//socket.connect(endp);
		}else {
			socket.bind(endp);
		}*
	}catch(std::error_code e){
		std::cout<<"\n[ERROR] "<<e.message();
		goto connecting;
	}
	/*if(!IsTargitRuning(socket)){
		std::cout << "\n[ERROR] Targit is probaly not runing the app.";
		goto connecting;
	}*
	std::cout << "\n ************************* " << targIp << " ************************* ";
	ip::tcp::endpoint ep(asio::ip::make_address_v4(targIp),LISNT_PORT);
	int cc = addConection(ep, context);
	std::string send_;
	
	for(;cone[cc]->is_open();){


		std::cout<<"\n>> ";
				//std::cin>>Message;
		std::getline(std::cin , send_);
		//std::cin>>send_;
		if(send_[0] == '/'){
			std::string ff = send_.substr(1,send_.size());
			cone[cc]->sendFile(ff);
			
		}else if( send_[0] =='\\'){
			saveImageFromeCamera("cam.jpg", 0);
			cone[cc]->sendFile("cam.jpg");
		}
		else{
			cone[cc]->sendMSG(send_);
		}
		//std::cout	
		//Message.append("\n>> ");
		//socket.write_some(buffer(Message.data() , Message.size() ),ec);
		//Message msg;
		//cone[cc]->sendFile("gui.h");
		//cone[cc]->ping();
		if(ec){
			std::cout<<"\n[ERROR] "<<ec.message();
			goto connecting;
		}
	}
	return;
}*/

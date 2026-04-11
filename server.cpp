#include "server.h"
#include "login.hpp"
#include "utils.hpp"
#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/impl/config.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/post.hpp>
#include <asio/registered_buffer.hpp>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <ios>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <unistd.h>

std::function<void(Image &img,unsigned int ID)> imageHandlingReq = [](auto,auto){return;};

std::function<void(float* sound,unsigned int Size,unsigned int ID)> soundHandlerRequast = [](auto,auto,auto){return;};;

std::function<void(char* msg , unsigned int msLng, unsigned int ID)> readMsgHandlerRequast = [](char* ms , unsigned int l, auto ){
	ConsoleCl(30);
	for(int i = 0 ; i < l ; i++){
		std::cout<<ms[i];
	}
	resetConsolCl();
};


std::function<void(unsigned int ID)> closeHandlerRequast = [](unsigned int ID){
	for(int i  =  0 ; i < cone.size() ; i++){
		if(cone[i]->getID() == ID){
			logMsgs("CLOSING CONCETION", " @ " + cone[i]->getEndpoint().address().to_string() + " aka."+cone[i]->name);
			break;
		}
	}
	return ;
};



std::function<int(std::string fileName,unsigned int size,unsigned int ID)> fileHandlerRequast = [](auto fn,auto s,auto ID){logMsgs(cone[getConPos(ID)]->name+" SENDE A FILE", fn+" @"+std::to_string(s)+"byte");return 1;};

std::function<void(std::string fileName,unsigned int size,unsigned int ID)> fileHandlerRequastComplite = [](auto fn,auto s,auto ID){logMsgs(cone[getConPos(ID)]->name+" SENDE A FILE", fn+" @"+std::to_string(s)+"byte");return 1;};




void closeSocket(ip::tcp::socket& skt){
	error_code ec;
	skt.shutdown(skt.shutdown_both , ec);
	if(ec){
		logMsgs("ERORR CLOSING SOCKET", ec.message() + ", closing any way...");
		
	}
	if(ec!=asio::error::not_connected){
		skt.close();
	}
	
}

#define FREE(x)\
	if(x==nullptr){\
		std::cout<<#x<<" IT IS A BAD ONE !!!!!!!!!!!!\n";\
	}else {\
		free(x);\
		x=nullptr;\
	}
//NOTE: do more with this , this is helpful


////////////////////////////////////////////////////////////////////////



connection::connection (asio::ip::tcp::endpoint endp , io_context &io____ ,std::vector <std::shared_ptr<connection>>& coneBuf)
{
	//skt = new ip::tcp::socket(io);	
	//this->io = &io;
	this->io = new io_context();
	
	//this->endp = ip::tcp::endpoint(endp.address(),LISNT_PORT);
	this->adress = endp.address();
	
	this->conectionBuf = &coneBuf;
	
	this->ID=getUsebelID();
	
	//this->sk = new ip::tcp::socket(*this->io);
	//this->sk.connect(ip::tcp::endpoint(this->adress,LISNT_PORT));

	//logMsgs("SESTION", "opened @" +this->adress.to_string());
	//sendPong();
	std::thread([&](){
		auto ll = make_work_guard(*this->io);
		this->io->run();
	}).detach();
	
	//post(*io,[](){logMsgs("ERRRRRRRRRRRRRRRRRRRRRRR");});
	return;

}



	
connection::connection(ip::tcp::socket& skt , io_context &io____ , std::vector <std::shared_ptr<connection>>& coneBuf){
	//this->skt = new ip::tcp::socket(io);
	this->io = new io_context();
	auto kk = make_work_guard(*this->io);
	std::thread([&](){
		this->io->run();
	}).detach();
	this->endp = skt.remote_endpoint();
	this->adress = skt.remote_endpoint().address();
	//*this->skt = std::move(skt);
	this->conectionBuf = &coneBuf;
	this->ID=getUsebelID();
	
	//this->sk = new ip::tcp::socket(*this->io);
	//this->sk.connect(ip::tcp::endpoint(this->adress,LISNT_PORT));


	//sendPong();
//ping();//hi , can i know your name?
	//sendPong();//nice , and this is my name
	//logMsgs("SESTION", "opened @" + this->adress.to_string());
}	
	


connection::~connection(){
	//closeSocket(*sk);
	//delete sk;
	io->stop();
	delete io;
}
	




void connection::sendFile(std::string fileP)
{
	if(m_close)return;
	std::thread([=](){
		std::filesystem::path fp(fileP);
		std::ifstream ifl(fileP ,std::ios::in|std::ios::app|std::ios::ate);
		const unsigned int fileS = ifl.tellg();
		//logMsgs("MSFILE SIZE", std::to_string(fileS));
		std::string fileName = fp.filename();
			
		char* mlc = (char*)malloc(fileS);
		memset(mlc, 0, fileS);

					
		if(!ifl.is_open()){
			logMsgs("NON FOUND FILE", fileName);
			ifl.close();
			FREE(mlc);
			return;
		}
		
		ifl.seekg(0);
		ifl.read(mlc, fileS);

		
		error_code ec;
		ip::tcp::socket sk(*io);
		sk.connect(ip::tcp::endpoint(this->adress, LISNT_PORT) , ec);
		if(ec || !sk.is_open()){
			logMsgs("ERROR SENDING FILE", ec.message());
			FREE(mlc);
			return ;
		}
		Packat ms;
		FileMs fms;
		
		memset(&ms , 0, sizeof(ms ));
		memset(&fms, 0, sizeof(fms));
			
		unsigned int packsNeed = (fileS + std::min((unsigned int)fileName.size(),(unsigned int)sizeof(fms.data)) + sizeof(fms.data) -1)/sizeof(fms.data);
		ms.msgL = fileS + packsNeed * (sizeof(ms) - sizeof(fms.data));

		ms.Mgic = MAGIC;
		ms.TYPE = MSFILE; 					//TODO:Remeber to change it to FILE type or somthing!
		
		fms.fileNameL=std::min(fileName.size(),sizeof(fms.data));
		fms.partN = 0;
		fms.dataSize =  fileS + fileName.size();
			
		if(fileName.size() > sizeof(fms.data)){
			logMsgs("WARNING", "file name is more than " + std::to_string(sizeof(fms.data)) + " charcture long , it will be cherenked");
		}

		unsigned int dataWroten = 0;
		memcpy(fms.data, fileName.c_str(), std::min((unsigned int)sizeof(fms.data) , (unsigned int)fileName.size()));
		
		if(fileName.size() < sizeof(fms.data)){
			unsigned int write = std::min((unsigned int)sizeof(fms.data) , (unsigned int)(fileS+fileName.size())) - fileName.size();
			memcpy((char*)(fms.data+fileName.size()), mlc, write);
			dataWroten+=write;
			//ifl.read((char*)(fms.data + fileName.size()), dataWroten);
			
		}
		Packat ready{.TYPE=READY,.Mgic=MAGIC,.msgL=0};


		memcpy(ms.data, &fms, std::min(sizeof(ms.data) , sizeof(fms)));
		bool ret = false;
		sk.write_some(buffer(&ms,PACKAT) , ec );
		if(ec){
			logMsgs("ERROR SENDING FILE", ec.message());
			//closeSocket(sk);
			FREE(mlc);
			return ;
		}
		sk.wait(sk.wait_write);

		for(int i = 1; i < packsNeed ; i++){
			memset(fms.data, 0, sizeof(fms.data));
			memset(ms.data, 0, sizeof(ms.data));

			fms.partN=i;
			fms.fileNameL=0;
			
			unsigned int writ = std::min((unsigned int)(fileS - dataWroten) , (unsigned int)sizeof(fms.data));
				
			memcpy(fms.data, (char*)(mlc+dataWroten), writ);
			memcpy(ms.data, &fms, std::min(sizeof(ms.data) , sizeof(fms)));
			
			//logMsgs("WAITING", std::to_string(writ));
			sk.wait(sk.wait_read);
			sk.read_some(buffer(&ready,PACKAT));
			sk.write_some(buffer(&ms,PACKAT));
			sk.wait(sk.wait_write);
			dataWroten+=writ;
			if(ec){
				logMsgs("ERROR SENDING FILE", ec.message());
				//closeSocket(sk);
				FREE(mlc);
				return ;
			}
		
			//logMsgs(std::to_string(i), "");	
		}
		
		//logMsgs("DONE SENDING","");
		ifl.close();
		FREE(mlc);
		//closeSocket(sk);
		return ;
			
	}).detach();
}


void connection::sendImage(unsigned int hight , unsigned int width , unsigned char* iData){//this and the sound sender , already in ther owne thread, no need for creat a other
	bool cont = false;
	if(m_close)return;
		unsigned int hi = hight, wi= width;
		unsigned int Size = hi*wi*3;//R8G8B8


		char* imgData = (char*)malloc(Size);	
		memcpy(imgData, iData, Size);	
		

		cont=true;
		error_code ec;
		ip::tcp::socket sk(*io);
		sk.connect(ip::tcp::endpoint(this->adress, LISNT_PORT) , ec);
		//sk.connect(ip::tcp::endpoint(this->adress, LISNT_PORT),ec);
		if(ec || !sk.is_open()){
			logMsgs("ERROR SENDING IMAGE", ec.message());
			if(imgData!=nullptr){
				FREE(imgData);
				imgData=nullptr;
			}
			closeSocket(sk);
			return ;
		}
		Packat ms;
		ImageMs ims;
		
		memset(&ms , 0, sizeof(ms ));
		memset(&ims, 0, sizeof(ims));
			
		unsigned int packsNeed = (Size + sizeof(ims.data) -1)/sizeof(ims.data);
		ms.msgL = Size + packsNeed * (sizeof(ms) - sizeof(ims.data));

		ms.Mgic = MAGIC;
		ms.TYPE = IMAGE;
		
		ims.ImgHight=hi;
		ims.ImgWidht=wi;
		ims.packN = 0;	

		unsigned int dataWroten = 0;
		
		Packat ready{.TYPE=READY,.Mgic=MAGIC,.msgL=0};
		//if(imgData==nullptr)return;	
		memcpy(ims.data, imgData, std::min(Size,(unsigned int)sizeof(ims.data)));
		dataWroten+=	std::min(Size,(unsigned int)sizeof(ims.data));
		memcpy(ms.data, &ims, std::min(sizeof(ms.data) , sizeof(ims)));
		bool ret = false;
		sk.write_some(buffer(&ms,PACKAT) , ec);
		if(ec){
			logMsgs("ERROR SENDING IMAGE", ec.message());
			closeSocket(sk);
			if(imgData!=nullptr){
				FREE(imgData);
				imgData=nullptr;
			}
			return ;
		}
		

		for(int i = 1; i < packsNeed  && imgData!=nullptr; i++){
			memset(ims.data, 0, sizeof(ims.data));
			memset(ms.data, 0, sizeof(ms.data));

			ims.packN=i;
			unsigned int writ = std::min((unsigned int)(Size - dataWroten) , (unsigned int)sizeof(ims.data));
				
			memcpy(ims.data, (char*)(imgData+dataWroten), writ);
			memcpy(ms.data, &ims, std::min(sizeof(ms.data) , sizeof(ims)));
			
			//logMsgs("WAITING", std::to_string(writ));
			//usleep(10000);
			//ready.TYPE=0;
			//while(ready.TYPE!=READY){
				/*if(!ec){
					sk.wait(sk.wait_read);
					sk.read_some(buffer(&ready,PACKAT),ec);
					
				}else {
					logMsgs("ERORR SENDING IMAGE", ec.message());
					closeSocket(sk);
					if(imgData!=nullptr){
						FREE(imgData);
						imgData=nullptr;
					}
					return;
				}*/
			//}
			
			sk.write_some(buffer(&ms,PACKAT),ec);
			
			/*if(!ec){sk.wait(sk.wait_write);}
			else{
				logMsgs("ERROR SENDING IMAGE", ec.message());
				if(imgData!=nullptr){
					FREE(imgData);
					imgData=nullptr;
				}
				return ;
			}*/
			dataWroten+=writ;
			
			//logMsgs(std::to_string(i), "");	
		}
		//sk.wait(sk.wait_write);
		//FREE(imgData);
		if(imgData!=nullptr){
			FREE(imgData);
			imgData=nullptr;
		}
		//logMsgs("End Sending");
		
		//closeSocket(sk);
		cont=true;
		return ;
	//	}).detach();

	//while(!cont);

}

void connection::sendSound(float* data__ , unsigned int ln){//NOTE:if somthing break , this is mybe the cuse
	bool conti = false;
	//std::thread([&](){
	//post(*io,[&](){
		if(m_close)return;
		unsigned int Size = ln*4;
		float* data = (float*)malloc(Size);	
		memcpy(data, data__, Size);
		conti = true;	
			
		error_code ec;
		ip::tcp::socket sk(*io);
		
		sk.connect(ip::tcp::endpoint(this->adress, LISNT_PORT) , ec);
		if(ec || !sk.is_open()){
			logMsgs("ERROR SENDING SOUND", ec.message());
			closeSocket(sk);
			if(data!=nullptr){
				FREE(data);
				data=nullptr;
			}
			return ;
		}
		Packat ms;
		SoundMs sms; //some remember a thing rigth now
		
		memset(&ms , 0, sizeof(ms ));
		memset(&sms, 0, sizeof(sms));
			
		unsigned int packsNeed = (Size + sizeof(sms.data) -1)/sizeof(sms.data);
		ms.msgL = Size + packsNeed * (sizeof(ms) - sizeof(sms.data));

		ms.Mgic = MAGIC;
		ms.TYPE = SOUND;
		
		sms.Size = Size;
		sms.packN = 0;	

		unsigned int dataWroten = 0;
		
		Packat ready{.TYPE=READY,.Mgic=MAGIC,.msgL=0};
		if(data==nullptr)return;	
		memcpy(sms.data, data, std::min(Size,(unsigned int)sizeof(sms.data)));
		dataWroten+=	std::min(Size,(unsigned int)sizeof(sms.data));
		memcpy(ms.data, &sms, std::min(sizeof(ms.data) , sizeof(sms)));
		bool ret = false;
		sk.write_some(buffer(&ms,PACKAT) , ec);
		if(ec){
			logMsgs("ERROR SENDING SOUND", ec.message());
			closeSocket(sk);
			if(data!=nullptr){
				FREE(data);
				data=nullptr;
			}
			return ;
		}
		//sk.wait(sk.wait_write);

		for(int i = 1; i < packsNeed  && data!=nullptr; i++){
			memset(sms.data, 0, sizeof(sms.data));
			memset(ms.data, 0, sizeof(ms.data));

			sms.packN=i;
			unsigned int writ = std::min((unsigned int)(Size - dataWroten) , (unsigned int)sizeof(sms.data));
				
			memcpy(sms.data, (char*)(data+dataWroten), writ);
			memcpy(ms.data, &sms, std::min(sizeof(ms.data) , sizeof(sms)));
			
			ready.TYPE=0;
			//while(ready.TYPE!=READY){
				/*if(!ec){
					sk.wait(sk.wait_read);
					sk.read_some(buffer(&ready,PACKAT),ec);
				}
				else {
					logMsgs("ERROR SENDING SOUND", ec.message());
					closeSocket(sk);
					if(data!=nullptr){
						FREE(data);
						data=nullptr;
					}
					return;
				}*/
				
			//}
			sk.write_some(buffer(&ms,PACKAT),ec);
			
			//if(!ec){sk.wait(sk.wait_write);}
			dataWroten+=writ;
			if(ec){
				logMsgs("ERROR SENDING SOUND", ec.message());
				closeSocket(sk);
				if(data!=nullptr){
					FREE(data);
					data=nullptr;
				}
				return ;
			}
		
		}
		if(data!=nullptr){
			FREE(data);
			data=nullptr;
		}
		
		closeSocket(sk);
		return ;
	//});
	//}).detach();
	//while(!conti){}
	return;
}


void connection::sendClose(){
	ip::tcp::socket sk(*io);

	error_code ec;
	Packat msg ;
	msg.TYPE =CLOSE;
	msg.Mgic =MAGIC;
	msg.msgL = 0;
	bool ret = false;
	sk.connect(ip::tcp::endpoint(adress , LISNT_PORT) , ec);
	if(ec){
		logMsgsErr(ec.message());
		ret=true;
	}
	if(ret||ec){
		return ;;
	}
	sk.write_some(buffer(&msg , PACKAT) ,ec);
	if(ec){
		logMsgsErr(ec.message());
		ret=true;
	}
	closeSocket(sk);
	return;
}


void connection::Close(){
	unsigned int pos = getVecPos();
	if(pos!=-1){
		m_close=true;
		io->stop();
		conectionBuf->erase(conectionBuf->begin() + pos);
	}else{
		logMsgsErr("CONCTION OBJECT , nevr found his selfe :(");
	}
	io->stop();
	return;
}

void connection::ping(){
	//if(pingThread.joinable())pingThread.join();
	//std::thread([=](){	
		ip::tcp::socket sk(*io);
	
		error_code ec;
		Packat msg ;
		msg.TYPE =PING;
		msg.Mgic =MAGIC;
		msg.msgL = 0;
		//memset(msg.data, 0, sizeof(msg.data));
		bool ret = false;
		//unsigned int startT = (clock()*1000)/CLOCKS_PER_SEC;
		//unsigned int timeGiveUp = 5000;//give up after 5 ms
		sk.connect(ip::tcp::endpoint(adress , LISNT_PORT) , ec);
		if(ec){
			logMsgsErr(ec.message());
			ret=true;
		}
		if(ret||ec){
			return ;;
		}
		sk.write_some(buffer(&msg , PACKAT) ,ec);
		if(ec){
			logMsgsErr(ec.message());
			ret=true;
		}
		if(ret || ec){
			return ;;
		}
		sk.wait(sk.wait_read);

		sk.read_some(buffer(&msg,PACKAT),ec);
		if(ec){
			logMsgsErr(ec.message());
			ret=true;
		}
		if(ret || ec){
			return ;;
		}

		name.clear();
		for(int i = 0 ; i < msg.msgL ; i++){
			name.push_back(msg.data[i]);
		}
		closeSocket(sk);
	//}).detach();
}

ip::tcp::endpoint connection::getEndpoint(){
	return endp;
}

ip::address connection::getAddress(){
	return this->adress;
}


void connection::sendMSG(std::string send_){
	if(!send_.size())return;
	bool ret = false;
	//post(*io , [&](){ // copy rvry thing we will need, so send will not dei
		Packat msg;

		Message ms ;
		
		std::string send = send_;
		
		error_code ec;
		ip::tcp::socket sk(*io);
		sk.connect(ip::tcp::endpoint(this->adress, LISNT_PORT), ec);
		if(ec){
			logMsgs("ERORR SENDING", ec.message());
			return ;
		}
		
		msg.TYPE=MESSAGE;
		msg.msgL=send.size();
		msg.Mgic=MAGIC;
		ms.msgl=send.size();
		unsigned int packN =(send.size()+sizeof(Message::msg)-1)/sizeof(Message::msg);
		
		for(int i = 0 ; i < packN  && sk.is_open(); i++ ){
			
			ms.packN = i;
			ms.msgl=send.size();
			unsigned int copyL = std::min(send.size() , (i+1)*sizeof(Message::msg));
			//memset(msg.data, 0, sizeof(msg.data));
			
			memcpy(ms.msg, (char*)(send.c_str() + i * sizeof(Message::msg)),  copyL - i*sizeof(Message::msg) );
			memcpy(msg.data, &ms,std::min(sizeof(msg.data),sizeof(ms.msg)));
			//for(int i =0 ; i < sizeof(ms->msg) ; i++)std::cout<<ms->msg[i];
			try{
			
				sk.write_some(buffer(&msg,PACKAT) ,ec);
				if(ec){
					logMsgs("ERORR SENDING", ec.message());
					closeSocket(sk);
					return ;
				}
				//sk.wait(sk.wait_write);
				
			}catch (system_error err){
				logMsgsErr(err.what());
			}
		}
		closeSocket(sk);
		return ;

	//});
}


void connection::sendPong(){
	Packat Pong;
	unsigned int sendS = std::min(sizeof(Packat::data) , user_name.size());
	Pong.msgL = sendS;
	Pong.Mgic=MAGIC;
	memcpy(Pong.data, user_name.c_str(), sendS);//it will be beter , if we send a warning her
	std::error_code ec;
	ip::tcp::socket sk(*io);
	std::error_code e;
	sk.connect(ip::tcp::endpoint(adress,LISNT_PORT), e);
	if(e){
		logMsgs("ERORR SENDING PONG", e.message());
		closeSocket(sk);
		return;
	}
	sk.write_some(buffer(&Pong,PACKAT),e);
	if(e){
		logMsgsErr(e.message());
		closeSocket(sk);
		return;
	}
	sk.wait(sk.wait_write);
	closeSocket(sk);
	return;
	
}
bool connection::is_open(){
	return true;//idk, this is a old one
}


unsigned int connection::getID(){
	return ID;
}
unsigned int connection::getUsebelID(){
	unsigned int nID=0;
	for(int i = 0 ; i < this->conectionBuf->size() ; i++){
		if(this->conectionBuf->at(i)->ID == nID)nID++;
	}
	return nID;
}

unsigned int connection::getVecPos(){
	unsigned int ret = -1;
	for(int i = 0 ; i < this->conectionBuf->size() ; i++){
		unsigned int fID = this->conectionBuf->at(i)->ID;
		if(fID==this->ID){
			ret=fID;
			break;
		}
	}
	return ret;

}





    /***************************************************************************/
   ///// ///// ///// /////////////////////////////////// ///// ///// ///// /////
  ///// ///// ///// ////////////SERVER SIDE//////////// ///// ///// ///// /////
 ///// ///// ///// /////////////////////////////////// ///// ///// ///// /////
/***************************************************************************/









bool server::MsgIsIt(unsigned int a){
	return data->TYPE == a & data->Mgic ==MAGIC;
}

unsigned int server::getVecPos(){
	for(int i = 0 ; i < vecy->size() ; i++ ){
		if(vecy->at(i)->ID == this->ID){
			return i;
		}
	}
	return 0;
}
void server::pingHandler(){
	error_code ec;	
	Packat msg ;

	msg.TYPE =PONG;
	msg.Mgic =MAGIC;
	msg.msgL = user_name.size();
	memset(msg.data, 0, sizeof(msg.data));
	memcpy(msg.data, user_name.c_str(), (size_t)std::min((int)user_name.size() , PACKAT));
	try{
			
		if(skt->is_open() && !ec){
			skt->write_some(buffer(&msg , PACKAT/*user_name.size() +PACK_HS*/),ec);
			if(!ec){
				skt->wait(skt->wait_write);

			}
		}
		else if(ec){
			logMsgs("ERORR IN PING HANDLING", ec.message());
		}else{
			logMsgs("ERORR IN PING", "socket  not open during the writing");
		}
		
	}catch (std::system_error syser){
		logMsgs("ERORR IN PING", syser.what());
	};
	return;
}
void server::FileHandler(){

//TODO: in this fenction, if the sender change the file size, it will be cuse a dangerus buffer overfluw,
//so ; stor the dataSize in a variabel in the class

	FileMs * msfile = (FileMs*)data->data;
	if(!msfile->partN){

		fileName.erase();
		for(unsigned int i = 0 ; i < msfile->fileNameL ; i++){
			fileName.push_back(msfile->data[i]);
			
		}
		
		fileNS = fileName.size();

		/*std::cout<<"\n";
		for(int ii = 0 ; ii < fileName.size()+2 ; ii++){
			std::cout<<"*";
		}
		std::cout<<"\n*"<<fileName<<"*" << " @" << msfile->dataSize-fileNS <<"\n";
		for(int ii = 0 ; ii < fileName.size()+2 ; ii++){
			std::cout<<"*";
		}
		std::cout<<"\n";*/

		/*kbool conti = fileHandlerRequast(fileName,fileNS,this->conction->ID);

		if(!conti){
			//TODO: do a cancel signel and then send it here
			return;
		}*/

		fileName.insert(0,donwloadF+"/");
		mlc = (char*)malloc(msfile->dataSize);
		file.open(fileName );
		
		//file.write((char*)(msfile->data+msfile->fileNameL), std::min((unsigned int)(sizeof(FileMs::data)-msfile->fileNameL) , msfile->dataSize-msfile->fileNameL));
		
		ptr=0;
		for(int i = msfile->fileNameL ; i < std::min((unsigned int)sizeof(FileMs::data) , msfile->dataSize);i++,ptr++){
			mlc[ptr] = msfile->data[i];
		}

		//file.close();
	}
	
	else {
		//file.open(fileName , std::ios::app );
		//file.write(msfile->data, std::min(msfile->dataSize , (unsigned int)((msfile->partN+1)*sizeof(FileMs::data) - msfile->partN*sizeof(FileMs::data))) );
		
		for(int i = 0 ; 
			i <  std::min(msfile->dataSize  , (unsigned int)((msfile->partN+1)*sizeof(FileMs::data))) -msfile->partN*sizeof(FileMs::data);
			i++,ptr++){
			mlc[ptr] = msfile->data[i];
		}
		//logMsgs(std::to_string(ptr), "");
		//file.close();
									
		
		
	}
	if(ptr >= msfile->dataSize - fileNS ){
		//logMsgs("WRITING FILE ...", "");
		file.seekp(0);
		file.write(mlc, ptr);
		fileHandlerRequastComplite(fileName,fileNS,this->conction->getID());		
		
		fileName="";
		if(mlc!=nullptr){
			FREE(mlc);
			mlc=nullptr;
		}
		ptr=0;
		fileNS=0;
		file.close();
		return;
	}

	else{//sending ready
		Packat ready{
			.TYPE=READY,
			.Mgic=MAGIC,
			.msgL=0
		};
		error_code e;
		skt->write_some(buffer(&ready ,PACK_HS /*it is just a ready , no data,to simlify things, we dont want multi packats*/),e );
		if(e){
			logMsgs("ERROR IN SENDING READY", e.message());
		}else{
			skt->wait(skt->wait_write);
		}
	}
	return;


}



void server::ImageHandler(){
	ImageMs * msImg = (ImageMs*)data->data;
	if(!msImg->packN){
		ISize = msImg->ImgWidht * msImg->ImgHight * 3;
		ImgB = (char*) malloc(ISize);
		imageWidth=msImg->ImgWidht;
		imageHeight=msImg->ImgHight;
		Iptr=0;
		for(int i = 0 ; i < std::min((unsigned int)sizeof(ImageMs::data) , ISize);i++,Iptr++){
			ImgB[Iptr] = msImg->data[Iptr];
		}
	}
	
	else {
		
		for(int i = 0 ; 
			i <  std::min( ISize  , (unsigned int)( (msImg->packN+1)*sizeof(ImageMs::data) ) ) - msImg->packN*sizeof(ImageMs::data);
			i++,Iptr++){
			ImgB[Iptr] = msImg->data[i];
		}
		
	}
	if(Iptr >= ISize){
	
			Image img;
			img.ImgHight = this->imageHeight;
			img.ImgWidht = this->imageWidth;
			img.imgBitmap = this->ImgB;

			imageHandlingReq(img,conction->ID);
		
		if(ImgB!=nullptr){
			FREE(ImgB);
			ImgB=nullptr;
		}
		ISize=0;
		imageWidth=0;
		imageHeight=0;
		Iptr=0;
		return;

	}

	else{//sending ready
		Packat ready{
			.TYPE=READY,
			.Mgic=MAGIC,
			.msgL=0
		};
		error_code e;
		//skt->write_some(buffer(&ready ,PACK_HS /*it is just a ready , no data,to simlify things, we dont want multi packats*/),e );
		if(e){
			logMsgs("ERROR IN SENDING READY", e.message());
		}else{
			//skt->wait(skt->wait_write);
		}
	}
	return;
}
void server::SondeHandler(){
	SoundMs * msSound = (SoundMs*)data->data;
	if(!msSound->packN){
		SSize = msSound->Size / 4;
		SondB = (float*) malloc(SSize*4);
		Sptr=0;
		for(int i = 0 ; i < std::min((unsigned int)sizeof(SoundMs::data)/4 , SSize);i++,Sptr++){
			SondB[Sptr] = msSound->data[Sptr];
		}
	}
	
	else {
		
		for(int i = 0 ; 
			i <  std::min( SSize, (unsigned int)( (msSound->packN+1)*sizeof(SoundMs::data)/4 ) ) - msSound->packN*sizeof(SoundMs::data)/4;
			i++,Sptr++){
			SondB[Sptr] = msSound->data[i];
		}
		
	}
	if(Sptr >= SSize){
	
		soundHandlerRequast(SondB,SSize,conction->ID);
		if(SondB!=nullptr){
			FREE(SondB);
			SondB=nullptr;
		}
		SSize=0;
		Sptr=0;

	}

	else{//sending ready
		Packat ready{
			.TYPE=READY,
			.Mgic=MAGIC,
			.msgL=0
		};
		error_code e;
		//skt->write_some(buffer(&ready ,PACK_HS /*it is just a ready , no data,to simlify things, we dont want multi packats*/),e );
		if(e){
			logMsgs("ERROR IN SENDING READY", e.message());
		}else {
			//skt->wait(skt->wait_write);

		}
	}
	return;
}

void server::MessageHandler(){
	
	Message * msg = (Message*)data->data;
	if(!msg->packN){
		msSize = msg->msgl;
		msgBuf = (char*) malloc(msSize);
		msPtr=0;
		for(int i = 0 ; i < std::min((unsigned int)sizeof(Message::msg) , msSize);i++,msPtr++){
			msgBuf[msPtr] = msg->msg[msPtr];
		}
	}
	
	else {
		
		for(int i = 0 ; 
			i <  std::min( msSize, (unsigned int)( (msg->packN+1)*sizeof(Message::msg) ) ) - msg->packN*sizeof(Message::msg);
			i++,msPtr++){
			msgBuf[msPtr] = msg->msg[i];
		}
		
	}
	if(msPtr >= msSize){
		readMsgHandlerRequast(msgBuf,msSize,this->conction->ID);		

		if(msgBuf!=nullptr){
			FREE(msgBuf);
			msgBuf=nullptr;
		}
		msSize=0;
		msPtr=0;

	}
	return;

}



void server::close(){
	if(isOpen){
	isOpen=false;
	delete skt;
	unsigned int entry = getVecPos();

	if(SondB!=nullptr){
	FREE(SondB);
	SondB=nullptr;
	}
	if(mlc!=nullptr){
	FREE(mlc);
	mlc=nullptr;
	}
	if(ImgB!=nullptr){
	FREE(ImgB);
	ImgB=nullptr;
	}
	if(Messag_.size())Messag_.erase();
	if(this->data!=nullptr){
	FREE(this->data);
	this->data=nullptr;
	}
	if(this->resevedData!=nullptr){
	FREE(this->resevedData);
	this->resevedData=nullptr;
	}
	this->conction->serverOpnedFromeDestny--;
	vecy->erase(vecy->begin()+entry);
	}

	return;
}

void server::CloseHandler(){
	closeHandlerRequast(conction->ID);
	
	for(int i = this->vecy->size() -1 /*we are her, so vecy cant be -1*/ ; i >=0 ; i--){
		if(this->vecy->at(i)->skt->remote_endpoint() == this->skt->remote_endpoint() 
				&& this->vecy->at(i)->ID != this->ID)
		{
			this->vecy->at(i)->close();//kill evry one
		}

	}
	
	this->conction->Close();//kill your master
	
	this->close();//and then , kill your selfe	
	return;	
}


void server::readHandler(){
	try{
		
		asio::async_read(*skt,buffer(data,PACKAT) , [=](error_code ec, size_t leng){
			if(!ec){
				
				if(skt->is_open()){
					if(MsgIsIt(PING)) { // is it ping? if is it send pong
						logMsgs("PING");
						pingHandler();
					}else if(MsgIsIt(PONG)){
						logMsgs("PONG");
						conction->name.clear();
						for(int i =0 ; i < data->msgL ; i++ ){
							conction->name.push_back(data->data[i]);
						}
					}

					else if (MsgIsIt(MESSAGE)){
						MessageHandler();
					}
					else if(MsgIsIt(MSFILE)){
						FileHandler();
					}else if (MsgIsIt(IMAGE)) {
						//logMsgs("GET A IMAGE");
						ImageHandler();
					}else if (MsgIsIt(SOUND)) {
						SondeHandler();
					}else if(MsgIsIt(CLOSE)){
						//logMsgs("Dead for your father", "");
						CloseHandler();//kill your famely
						return ;
					}					
				}
				
			}else{
				if(ec.value() == asio::error::eof){
					//logMsgs("END-DELETING", "Entry #"+std::to_string(entry));
					
					closeSocket(*skt);
					
					
				}else if(ec.value() == asio::error::operation_aborted){
					logMsgs("SERVER FATEL ERROR --OA--", ec.message());
				}
				else {
					logMsgs("SERVER FATEL ERROR", ec.message());
					closeSocket(*skt);	
				}
				if(isOpen){	
					delete skt;
					isOpen=false;
					unsigned int entry = getVecPos();
					
					if(SondB!=nullptr){
						FREE(SondB);
						SondB=nullptr;
					}
					if(mlc!=nullptr){
						FREE(mlc);
						mlc=nullptr;
					}
					if(ImgB!=nullptr){
						FREE(ImgB);
						ImgB=nullptr;
					}
					if(Messag_.size())Messag_.erase();
					if(this->data!=nullptr){
						FREE(this->data);
						this->data=nullptr;
					}
					if(this->resevedData!=nullptr){
						FREE(this->resevedData);
						this->resevedData=nullptr;
					}
					this->conction->serverOpnedFromeDestny--;
					//io->stop();
					vecy->erase(vecy->begin()+entry);
				}
				return ;
			}
			dataPtr=0;
			readHandler();

		});
	}catch (std::system_error ec){
		logMsgs("SYSTEM ERROR", ec.what());
	};
}



server::server(ip::tcp::socket &skt , std::shared_ptr<connection>& con , io_context& io , std::vector<std::shared_ptr<server>>& vec , unsigned int entry){

	this->conction = con;
	vecy = &vec;
	isOpen=true;
	data=(Packat*)malloc(PACKAT);
	//resevedData = (Packat*)malloc(PACKAT);
	unsigned int ID = 0;
	for(int i = 0 ; i < vecy->size() ;i++ ){
		if(vecy->at(i)->ID == ID){
			ID++;
		}
	}
	this->ID = ID;
	this->conction->serverOpnedFromeDestny++;
	
	unsigned int fd = skt.release();
	
	this->skt=new ip::tcp::socket(*this->conction->io);
	this->skt->assign(ip::tcp::v4() , fd);
	readHandler();
}

server::~server(){
	if(isOpen){
		
		if(SondB!=nullptr){
			FREE(SondB);
			SondB=nullptr;
		}
		if(mlc!=nullptr){
			FREE(mlc);
			mlc=nullptr;
		}
		if(ImgB!=nullptr){
			FREE(ImgB);
			ImgB=nullptr;
		}
		if(Messag_.size())Messag_.erase();
		if(this->data!=nullptr){
			FREE(this->data);
			this->data=nullptr;
		}
		if(this->resevedData!=nullptr){
			FREE(this->resevedData);
			this->resevedData=nullptr;
		}
		closeSocket(*skt);
		dataPtr=0;
		Messag_.erase();
		this->conction->serverOpnedFromeDestny--;
		//FREE(skt);
		delete skt;
		isOpen=false;
		//io->stop();
	}
	return;
}










    /******************************************************************************************/
   ////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////Conection & SeverMangment///////////////////////////////////
 ////////////////////////////////////////////////////////////////////////////////////////////
/******************************************************************************************/







std::vector<std::shared_ptr<connection>> cone;
std::vector<std::shared_ptr<server>> servers;
std::vector<std::shared_ptr<DevInNetwork>> dess;







DevInNetwork::DevInNetwork(asio::io_context & io , ip::tcp::endpoint& ep)
{
	ping = (Packat*)malloc(PACKAT);
	skt =new ip::tcp::socket(io);
	//endp = ep;	
	endp = ip::tcp::endpoint(ep.address() , LISNT_PORT);
	this->io = &io;
	
	connect();
	//readHandler();
}

DevInNetwork::~DevInNetwork(){
	FREE(ping);
	delete skt;
}

void DevInNetwork::connect(){

	//asio::post(*io,[&](){
	skt->async_connect(endp,[&](auto){
		is_open=false;
		//name=false;

		ping->Mgic = MAGIC;
		ping->TYPE = PING;
		//logMsgs(endp.address().to_string(), "");
		if(skt->is_open()){
			//IsTargitRuning(*skt);
			skt->async_write_some(buffer((char*)ping,PACKAT),[&](std::error_code ec , size_t lg){
				if(!ec){
					skt->async_read_some(buffer((char*)ping, PACKAT) , [&](std::error_code ecc, size_t llg){
						if(!ecc && ping->TYPE==PONG){
							
							is_open=true;
							name="";
							for(int i = 0 ; i < ping->msgL ; i++){
								name.push_back(ping->data[i]);
							}
							logMsgs("FIND", name);
							FindHandler();
							//connect();
							//closeSocket(*skt);
						}
					});
				}
			});

		}

	});
}

void DevInNetwork::FindHandler(){
	unsigned int con = addConection(endp,*io);
	cone[con]->name = this->name;
}





unsigned int addConection(ip::tcp::endpoint ep, io_context &io){
	unsigned int ret = -1;
	for(int i = 0 ; i < cone.size() ; i++){
		if(cone[i]->getAddress() == ep.address()){
			ret=i;
			break;
		}
	}
	if(ret==-1){
		int ID=0;
		for(int i = 0 ; i < cone.size() ; i++){
			if(cone[i]->getID() == ID)ID++;
		}
		ret = cone.size();
	//	std::shared_ptr<connection> conc = std::make_shared<connection>(ep,io,ID);
		cone.emplace_back( std::make_shared<connection>(ep,io,cone));
		//cone.push_back(conc);
		//conc->ping();
		//conc->readHandler();

	}
	return ret;
}



unsigned int addConection(ip::tcp::socket &skt, io_context &io){
	unsigned int ret = -1;
	for(int i = 0 ; i < cone.size() ; i++){
		if(cone[i]->getAddress() == skt.remote_endpoint().address()){
			ret=i;
			break;
		}
	}
	if(ret==-1){
		int ID=0;
		for(int i = 0 ; i < cone.size() ; i++){
			if(cone[i]->getID() == ID)ID++;
		}
		ret = cone.size();
		//std::shared_ptr<connection> conc = std::make_shared<connection>(skt,io,ID);
		//cone.push_back(conc);
		cone.emplace_back(std::make_shared<connection>(skt.remote_endpoint(),io,cone));
		//conc->ping();
		//conc->readHandler();

	}
	return ret;
}

void addServer(ip::tcp::socket &skt, io_context &Oio , io_context& Iio){
	unsigned int c = addConection(skt , Oio);
	//std::shared_ptr<server> svr = std::make_shared<server>(skt , cone[c],io , servers , servers.size());
	unsigned int emP = servers.size();
	servers.emplace_back(std::make_shared<server>(skt , cone[c],Iio , servers , servers.size()));
	return ;
}


void ClientHandl(asio::ip::tcp::acceptor &accept , io_context& Oio , io_context& Iio){
	accept.async_accept(
			[&](std::error_code ec , asio::ip::tcp::socket socket)
			{
				if(!ec){
					//std::cout<<"Cleint connect! @" << servers.size() << "\n";
					addServer(socket,Oio, Iio); 


				}
				ClientHandl(accept,Oio,Iio);
			}
	);

	return;
}



bool IsTargitRuning(ip::tcp::socket & skt){
	std::error_code ec;
	Packat* ping = (Packat*)malloc( PACKAT );
	ping->Mgic=MAGIC;
	ping->TYPE=PING;

	skt.write_some(buffer((char*)ping , sizeof(Packat) ),ec);
	std::cout << "\n---Wating for pong---\n";
	//skt.wait(skt.wait_read , 100);
	usleep(10000);
	std::cout << "\nDone";
	skt.read_some(buffer((char*)ping  , sizeof(Packat)) , ec);
	if(ping->TYPE != PONG || ec){
		FREE(ping);
		return false;
	}
	std::string targetName = ping->data;
	std::cout << "\n" << "Device Name : "  ;
	for(int i = 0 ; i < 20 ; i++)std::cout<<ping->data[i];;
	FREE(ping);
	return true;
}



void dicover(io_context &io , std::string Astart , std::string Aend ){

	try{
		ip::tcp::endpoint endp(ip::make_address_v4(Astart) , LISNT_PORT);

		unsigned int start = ip::make_address_v4(Astart).to_uint();
		unsigned int end = ip::make_address_v4(Aend).to_uint();
		if(end<start) {
			logMsgsErr("netdiscovering end is less than netdicovring start");
			return;
		}
		dess.reserve(end-start);

		for(int i = start  ; i <  end ; i++){
			endp.address(ip::make_address_v4(i));
			//std::cout << "\n"<< endp.address() << "--" << endp.port();
			try{
				//std::shared_ptr<DevInNetwork> d = std::make_shared<DevInNetwork>(io,endp);
				//dess.push_back(d);
				dess.emplace_back(std::make_shared<DevInNetwork>(io,endp));//never make the chorno angree,i main, if he see this code, i dont think that this will make any change

			}catch(std::system_error ec){
				//probaly main that we cant connect to
			}


		}
	}catch(std::error_code ec){
		logMsgsErr(ec.message());
	}
	return;
}





unsigned int getConPos(unsigned int ID){
	unsigned int ret = -1;
	for(int i = 0 ; i < cone.size() ; i++){
		if(cone[i]->getID() == ID){
			ret=i;
			break;
		}
	}
	return ret;
}



/*
 asio::io_context io;
ip::tcp::resolver resolver(io);

// get your machine hostname
char hostname[256];
gethostname(hostname, sizeof(hostname));

// resolve hostname to IP
auto results = resolver.resolve(hostname, "");
for(auto& result : results) {
    std::cout << "IP: " << result.endpoint().address() << "\n";
}*/

#include "server.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace EpochLabsTest {

std::mutex mu;

Server::Server(const std::string& listen_address, int listen_port)
    : listen_fd(-1)
{
    std::cout << "creating server" << std::endl;

    sockaddr_in listen_sockaddr_in;
    std::memset(&listen_sockaddr_in, 0, sizeof(listen_sockaddr_in));
    listen_sockaddr_in.sin_family = AF_INET;
    inet_aton(listen_address.c_str(), &listen_sockaddr_in.sin_addr);
    listen_sockaddr_in.sin_port = htons(listen_port);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0) {
        throw_error("could not create socket", errno);
    }

    int t = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t))) {
        throw_error("could not set SO_REUSEADDR", errno);
    }

    if(bind(listen_fd, (struct sockaddr*) &listen_sockaddr_in, sizeof(listen_sockaddr_in))) {
        throw_error("could not bind listen socket", errno);
    }

    if(listen(listen_fd, 48)) {
        throw_error("could not listen on socket", errno);
    }

    //picked up by test_server.py to know server is ready
    //this line must be output after listen returns successfully 
    std::cout << "listening on " << listen_address << ":" << listen_port << std::endl;
}

int Server::accept_new_connection() {
    std::cout<<"Accepting connection";
    sockaddr_in peer_addr;
    socklen_t peer_addr_size = sizeof(peer_addr);
    std::memset(&peer_addr, 0, peer_addr_size);
    
    //peer_fd is the file descriptor for the socket of the newly connected client
    int peer_fd = accept4(listen_fd, (struct sockaddr*) &peer_addr, &peer_addr_size, SOCK_CLOEXEC);
  
    if (peer_fd < 0) {
        throw_error("error accepting connection", errno);
    }

    std::cout << "accepted connection, peer_fd=" << peer_fd << std::endl;

    return peer_fd;
}

void Server::run() {
    std::cout << "running ..." << std::endl;
    
    int noOfThreads = 50;
    std::thread thr[50];
    int currNoOfThreads = 0;
    while(currNoOfThreads <= noOfThreads ){
    	int peer_fd = accept_new_connection();
	if(peer_fd <0)
		throw_error("Server::run() is not not implemented", 0);	
	else
		std::cout<<"Connected"<<std::endl;

	thr[currNoOfThreads]=std::thread(Server::serverTask, peer_fd, &mapOfVal);
	currNoOfThreads++;	
    }
    
}

void Server::serverTask(int peer_fd, std::map<std::string,std::string> *mapOfVal){
	
	char test[1024];
	char req[1024];
	bool loop = false;
	int rfd = peer_fd;
	while(!loop)
	{    
		bzero(test, 301);
		read(rfd, test, 300);
		std::string tester (test);
		
		for(int i=0; i<(signed)strlen(test); i++)
		{	
			//Checking for new line.
			if(test[i] != '\n')
				req[strlen(req)] = test[i];
			else{
				if (strcmp(req, "quit") == 0)
					goto end;
				else{
					mu.lock();
					
					//Splitting the request using ' ' as delimiter					
					std::string delimiter=" ";
					char* tempChar;
					std::vector<std::string> requestValues;
					tempChar=strtok(req,delimiter.c_str());
					while(tempChar!=NULL){
						requestValues.push_back(tempChar);
						tempChar=strtok(NULL,delimiter.c_str());
					}
					
					//processing get request
					if(requestValues[0]=="get")
					{
						
						std::map<std::string,std::string>::iterator it = mapOfVal->find(requestValues[1]); 
						std::string value="";
						if (it != mapOfVal->end())
							value=requestValues[1]+"="+(*mapOfVal)[requestValues[1]]+"\n";
						else
							value=requestValues[1]+"=null\n";
		
						char val1[1024];
						strcpy(val1,value.c_str());
						write(rfd,val1,value.size());
						memset(&val1[0], 0, sizeof(val1));	

					}

					//processing set request
					else if(requestValues[0]=="set")
					{						
						std::map<std::string,std::string>::iterator it = mapOfVal->find(requestValues[1]); 
						if (it != mapOfVal->end())
							(*mapOfVal)[requestValues[1]] = requestValues[2];		
						else 
							mapOfVal->insert(std::map<std::string,std::string>::value_type(requestValues[1],requestValues[2]));
									
						std::string value=requestValues[1]+"="+(*mapOfVal)[requestValues[1]]+"\n";
						char val1[1024];
						strcpy(val1,value.c_str());		
						write(rfd,val1,value.size());
						memset(&val1[0], 0, sizeof(val1));
					}
					memset(&req[0], 0, sizeof(req));
					mu.unlock();
				}
			}
		}
	}
end:
close(rfd);
}

void Server::throw_error(const char* msg_, int errno_) {
    std::string msg = msg_ + std::string(" errno=") + std::to_string(errno_);
    throw std::runtime_error(msg);
}

}

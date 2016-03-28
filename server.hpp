#ifndef EPOCHLABS_TEST_SERVER_HPP
#define EPOCHLABS_TEST_SERVER_HPP

#include <string>
#include <map>


namespace EpochLabsTest {

class Server {
public:
    Server(const std::string& listen_address, int listen_port);
    void run();
  
private:
    int listen_fd;
    std::map<std::string,std::string> mapOfVal;
    


    int accept_new_connection();
    void throw_error(const char* msg_, int errno_);
    static void serverTask(int peer_fd, std::map<std::string,std::string> *mapOfVal );
};

}

#endif

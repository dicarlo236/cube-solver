//
// Created by jared on 11/25/15.
//

#ifndef CUBEWIZARD_SOLVERCONNECTION_H
#define CUBEWIZARD_SOLVERCONNECTION_H


#include "PracticalSocket.h"

class SolverConnection {
    TCPSocket* sock;
public:
    //establish connection with solve server.
    void initConnection(int port);
    //send cube string and return solution string
    char* solve(std::string cube);
private:
    char return_value[300];
};


#endif //CUBEWIZARD_SOLVERCONNECTION_H

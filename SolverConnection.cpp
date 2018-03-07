//
// Created by jared on 11/25/15.
//
#include <string>
#include <iostream>
#include <stdio.h>
#include "SolverConnection.h"
#include "Timer.h"

void SolverConnection::initConnection(int port) {
    //solver running locally - use loopback address.
    std::string servAddress = "127.0.0.1";

    unsigned short servPort = port;
    //open socket connection to server.
    try {
        sock = new TCPSocket(servAddress, servPort);
        std::cout<<"connected"<<std::endl;
    } catch(SocketException &e) {
        std::cout<<e.what()<<std::endl;
    }

}

char* SolverConnection::solve(std::string cube) {
    //socket server starts by sending the size of the message.
    try {
        //Timer timer;
        //timer.start();
        //std::string result;
        //append lower case "a" at end to mark end of cube string
        std::string cubemod = cube + "a";
        //send string
        sock->send(cubemod.c_str(), 55);
        //solver first sends two chars indicating the size of the solution
        char sizeBuffer[2];
        //store these two size chars in sizeBuffer
        sock->recv(sizeBuffer, 2);
        printf("size: %c%c\n",sizeBuffer[0],sizeBuffer[1]);
        //calculate the size of the solution string
        int size_tens = sizeBuffer[0] - '0';
        int size_ones = sizeBuffer[1] - '0';
        int size = 10 * size_tens + size_ones;
        std::cout << "CUBE STRING SIZE: " << size << std::endl;
        //receive solution string
        //char resultBuffer[size];
        sock->recv(return_value, size);
        return_value[size] = '\0';
        //for (int i = 0; i < size; i++) {
            //result += resultBuffer[i];
        //}
        //std::cout<<result<<std::endl;
        //std::cout<<"Time: "<<timer.stop()<<std::endl;
        return return_value;
    } catch(SocketException &e) {
        std::cout<<e.what()<<std::endl;
        return NULL; //lol that's why it crashes.
    }
}


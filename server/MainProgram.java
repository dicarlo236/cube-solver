package ui;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import cs.min2phase.Search;

/**
 * Created by jared on 11/25/15.
 * updated 1/13/18 with less printing
 */
public class MainProgram {
    static String cubeString = "";
    static Search search = new Search();
    static int maxDepth = 24;
    public static void main(String[] args) throws IOException {
        System.out.println("starting listener...");
        int clientNumber = 0;
        ServerSocket listener = new ServerSocket(9090);
        try {
            while(true) {
                new Solver(listener.accept(), clientNumber++).start();
            }
        } finally {
            listener.close();
        }
    }

    private static String addToCubeString(int value, String cubeString){
        String addition = "";
        switch (value){
            case 66: addition = "B";
                break;
            case 70: addition = "F";
                break;
            case 85: addition = "U";
                break;
            case 68: addition = "D";
                break;
            case 76: addition = "L";
                break;
            case 82: addition = "R";
                break;
            default: addition = "A";
                break;

        }
        return cubeString + addition;
    }

    private static void smartSend(String data, PrintWriter output){
        int size = data.length();
        String size_str;
        if(size < 10){
           size_str = "0" + Integer.toString(size);
        }else{
            size_str = Integer.toString(size);
        }
        output.println(size_str + data);
    }

    private static String solveCube(String cubeString){
        int mask = 0;
        mask |= 0;
        mask |= 0;
        mask |= 0;
        long t = System.nanoTime();
        String result = search.solution(cubeString, maxDepth, 100, 0, mask);;
        long n_probe = search.numberOfProbes();
        // ++++++++++++++++++++++++ Call Search.solution method from package org.kociemba.twophase ++++++++++++++++++++++++

        while (result.startsWith("Error 8") && ((System.nanoTime() - t) < 2 * 1.0e9)) {
            result = search.next(100, 0, mask);
            n_probe += search.numberOfProbes();
        }
        t = System.nanoTime() - t;
        System.out.println("Length of result: " + result);
        return result;
    }

    private static class Solver extends Thread {
        private Socket socket;
        private int clientNumber;

        public Solver(Socket socket, int clientNumber){
            this.socket = socket;
            this.clientNumber = clientNumber;
            System.out.println("Connection: " + clientNumber);
        }

        public void run(){
            try {
                BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                PrintWriter out = new PrintWriter(socket.getOutputStream(), true);

                //out.println("SUCCESS");

                while(true){
                    //System.out.println(in.read());
                    int inVal = in.read();
                    //System.out.println(inVal);
                    if(inVal == 97){
                        //System.out.println(cubeString);
                        String result = solveCube(cubeString);
                        System.out.println(result);
                        smartSend(result, out);
			System.out.println("Sent solution.");
                        cubeString = "";
                        break;


                    }
                    cubeString = addToCubeString(inVal, cubeString);
//                    String input = in.readLine();
//                    System.out.println(input);
//                    if(input == null || input.equals(".")){
//                        break;
//                    }
                    //out.println("SOLN");
                   // System.out.println(input);
                }
            } catch (IOException e){
                e.printStackTrace();
            } finally {
                try {
                    socket.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                System.out.println("Closed connection " + clientNumber);
            }
        }
    }
}

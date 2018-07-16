#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

//to show the error message and exit the program
void error(string msg){
	char *msg1;
	const int len = msg.length();
	msg1 = new char[len+1];
	strcpy(msg1,msg.c_str());
	perror(msg1);
	exit(1);
}

int main(int argc, char *argv[]){
	//for tcp to send the file (including the 2D array and the number of lines of the file)
	typedef struct{
		int iArraylen;       //the real length of the file
		int iMessarray[100][3];    // first column:operator; second column(and/or=16/17): the first value; third column: the second value;
	}FMESSAGE, *RFMESSAGE;

	//the final result struct(from server->edge server->client)
	typedef struct{
		int total;        //length
		int results[100];
	}CRESULT, *CFRESULT;

	char *filename;
	stringstream ss; 
	string str,str0,str1,str2;
	int stri0;           //int,correspond to the operational rule("and" or "or" and=16, or=17)
	int message[100][3];   //store the file content temporarily
	int i=0;         //array (message)
	int length=0;     //the real length in the file

	int tcp_CliScockdes, n;      			//n used for send
	struct sockaddr_in edgeserv_addr;       //family, port number, internet address and how large
	char buffer[2048] = {0};     			//store the received message
	const int tcpportno = 23145;    		//client tcp port number

	//socket()
	tcp_CliScockdes = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_CliScockdes < 0){
		error("Error opening socket");
	}

	//connect()
	memset(&edgeserv_addr, 0, sizeof(edgeserv_addr));      //set to all zero
	edgeserv_addr.sin_family = AF_INET;
	edgeserv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	edgeserv_addr.sin_port = htons(tcpportno);  		//network byte order

	if(connect(tcp_CliScockdes, (struct sockaddr *)&edgeserv_addr,sizeof(struct sockaddr)) < 0){    //similar to the youtube video(link) given in the instruction pdf
		close(tcp_CliScockdes);
		error("Error connecting");
	}else{
		cout << "The client is up and running." <<endl;
	}

	filename = argv[1];	//get the name of the file ./client ***.txt
	ifstream ifs(filename);
	while(getline(ifs, str)){			//read every line
		int pos = str.find(',');     //change the comma into space
		while(pos != string::npos){       //whether find ,
			str = str.replace(pos,1,1,' ');
			pos = str.find(',');
		}
		//seperate the number   
		ss.clear();
		ss.str(str);
		ss >> str0 >> str1 >> str2;

		//and = 16, or = 17 (in order to put it into a integer array(message))
		if(str0 == "and"){
			stri0 = 16;
		}
		if(str0 == "or"){
			stri0 = 17;
		}
		message[i][0] = stri0;
		message[i][1] = atoi(str1.c_str());
		message[i][2] = atoi(str2.c_str());
		i++;
	}

	//get the length
	for(int i=0;i<100;i++){
		if(message[i][0] == 16 || message[i][0] == 17){
			length++;
		}
	}

	//put into the array of the struct
	FMESSAGE strMessage = {0};
	for(int i = 0; i < length; i++){
		strMessage.iMessarray[i][0] = message[i][0];
		strMessage.iMessarray[i][1] = message[i][1];
		strMessage.iMessarray[i][2] = message[i][2];
	}

	//send()
	strMessage.iArraylen = length;
	n = send(tcp_CliScockdes, &strMessage, sizeof(FMESSAGE),0);
	if(n<0){
		close(tcp_CliScockdes);
		error("Error on sending");
	}
	else{
		length = 0;       
	}
	
	cout << "The client has successfully finished sending " << strMessage.iArraylen <<" lines to the edge server" <<endl;

	//recv()
	int m = recv(tcp_CliScockdes, buffer, sizeof(buffer), 0); 
	if(m<0){       //unsuccessful
		close(tcp_CliScockdes);
		error("Error receiving from edge.");
	}else{
		cout << "The client has successfully finished receiving all computation results from the edge server." <<endl;
		cout << "The final computation result are:" <<endl;

		CFRESULT fresult = (CFRESULT)buffer;
		for(int i=0; i<(fresult->total);i++){
			cout<< fresult->results[i] << endl;     //print the result
		}
		close(tcp_CliScockdes);       				//close the socket

	}
	return 0;
}
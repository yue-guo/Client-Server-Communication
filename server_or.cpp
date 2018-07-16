#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
using namespace std;

void error(string msg){
	char *msg1;
	const int len = msg.length();
	msg1 = new char[len+1];
	strcpy(msg1,msg.c_str());
	perror(msg1);
	exit(1);
}

int main(){
	//udp read struct
	typedef struct{
		int eOrlen;
		int eOrarray[100][3];
	}ORMESSAGE, *RORMESSAGE;

	//udp write struct
	typedef struct{
		int results[100][4];      //line no. result num1 num2
		int servername;
		int orlength;
	}RESULT, *PRESULT;

	const int orServPortNo = 21145;
	struct sockaddr_in oraddr, cliaddr;
	oraddr.sin_family = AF_INET;
	oraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	oraddr.sin_port = htons(orServPortNo);

	int udpSockDes;
	int orlength = 0;
	char buffer[2048] = {0};
	int iRecvarray[100][3];
	int computationresult;
	socklen_t size = sizeof(cliaddr);

	//socket()
	udpSockDes = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpSockDes < 0){
		error("Error opening socket");
	}

	//bind
	if(bind(udpSockDes, (struct sockaddr*)&oraddr, sizeof(oraddr)) < 0){
		error("Error binding");
	}

	cout << "The Server OR is up and running using UDP on port " << orServPortNo <<endl;

	//recv()
	while(1){
		int n = recvfrom(udpSockDes, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &size);			
		if(n > 0){
			cout << "The Server OR start receiving lines from the edge server for OR computation. The computation results are." <<endl;
			RORMESSAGE ormess = (RORMESSAGE)buffer;
			RESULT result;

			int m;
			for(int i=0; i<(ormess->eOrlen); i++){
				iRecvarray[i][0] = ormess -> eOrarray[i][0];      //line no.
				iRecvarray[i][1] = ormess -> eOrarray[i][1];
				iRecvarray[i][2] = ormess -> eOrarray[i][2];
				orlength++;
			}
			for(int i=0; i<orlength; i++){
				result.results[i][0] = iRecvarray[i][0];        //line no.
				computationresult = iRecvarray[i][1] | iRecvarray[i][2];
				result.results[i][1] = computationresult;
				result.results[i][2] = iRecvarray[i][1];
				result.results[i][3] = iRecvarray[i][2];
				cout << iRecvarray[i][1] << " or " << iRecvarray[i][2] << " = " << computationresult << endl;
			}
			cout << "The Server OR has successfully received " << orlength << " lines from the edge server and finish all OR computation" <<endl;
			result.servername = 17;
			result.orlength = orlength;
			int p = sendto(udpSockDes, &result, sizeof(RESULT),0,(struct sockaddr *)&cliaddr, sizeof(cliaddr));
			if(p < 0){
				error("Error OR server sending");
			}else{
				cout << "The Server OR has successfully finished sending all computation results to the edge server" <<endl;
			}
		}else{
			error("Error receiving");
		}
		//
	}
	if(udpSockDes >= 0){
		close(udpSockDes);
	}
}
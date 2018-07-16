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
		int eAndlen;
		int eAndarray[100][3];
	}ANDMESSAGE, *RANDMESSAGE;

	//udp write struct
	typedef struct{
		int results[100][4];       //line no. and result and num1 and num2
		int servername;
		int andlength;           //how many lines
	}RESULT, *PRESULT;

	const int andServPortNo = 22145;
	struct sockaddr_in andaddr, cliaddr;
	andaddr.sin_family = AF_INET;
	andaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	andaddr.sin_port = htons(andServPortNo);

	int udpSockDes;
	int andlength = 0;
	char buffer[2048] = {0};
	int iRecvarray[100][3];       //save line no.
	int computationresult;
	socklen_t size = sizeof(cliaddr);
	RESULT result;

	//socket()
	udpSockDes = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpSockDes < 0){
		error("Error opening socket");
	}

	//bind
	if(bind(udpSockDes, (struct sockaddr*)&andaddr, sizeof(andaddr)) < 0){
		error("Error binding");
	}

	cout << "The Server AND is up and running using UDP on port " << andServPortNo <<endl;

	//recv()
	while(1){
		int n = recvfrom(udpSockDes, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &size);
		if(n > 0){
			cout << "The Server AND start receiving lines from the edge server for AND computation. The computation results are." << endl;
			RANDMESSAGE andmess = (RANDMESSAGE)buffer;
			int m;
			for(int i=0; i<(andmess->eAndlen); i++){
				iRecvarray[i][0] = andmess -> eAndarray[i][0]; //line no.
				iRecvarray[i][1] = andmess -> eAndarray[i][1];
				iRecvarray[i][2] = andmess -> eAndarray[i][2];
				andlength++;
			}

			for(int i=0; i<andlength; i++){
				result.results[i][0] = iRecvarray[i][0];        //line no.
				computationresult = iRecvarray[i][1] & iRecvarray[i][2];
				result.results[i][1] = computationresult;         //result
				result.results[i][2] = iRecvarray[i][1];
				result.results[i][3] = iRecvarray[i][2];
				cout << iRecvarray[i][1] << " and " << iRecvarray[i][2] << " = " << computationresult <<endl;  
			}
			cout <<"The Server AND has successfully received " << andlength << " lines from the edge server and finished all AND computations" <<endl;
			result.servername = 16;
			result.andlength = andlength;
			int p = sendto(udpSockDes, &result, sizeof(RESULT),0,(struct sockaddr *)&cliaddr, sizeof(cliaddr));
			if(p < 0){
				error("Error And server sending");
			}else{
				cout << "The Server AND has successfully finished sending all computation results to the edge server" <<endl;
			}
		}
		else{
			error("Error receiving");
		}
	}
	if(udpSockDes >= 0){
		close(udpSockDes);      //close()
	}

}
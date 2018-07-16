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

void sigchld_handler(int s){         //from Beej's, used in main()
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void error(string msg){    //from the youtube video
	char *msg1;
	const int len = msg.length();
	msg1 = new char[len+1];
	strcpy(msg1,msg.c_str());
	perror(msg1);
	exit(1);
}

int main(){
	int tcp_ServSockdes, tcp_ChildSockdes; 
	int udp_ServSockdes;
	fd_set fdrset, fdrmid, fdwset, fdwmid; //socket description set,2pair read 2, write 2(use for select, get from Beej's 9.19(select))
	int  maxsockdes;
	int filelength;
	char buffer[2048] = {0};
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	struct sigaction sa;
	struct timeval timeout;      //time out
	int iRecvarray[100][3];
	int iAndRecvarray[100][2];         //(for and operator) temporary save the result from the backend
	int iOrRecvarray[100][2];           //(for or operator)
	unsigned int len;
	int alllength = 0;
	int n;
	const int tcpportno = 23145;
	const int udpandportno = 22145;
	const int udporportno = 21145;
	const int udpedgeportno = 24145;
	int count = 0;      //to check the finish send to back server 
	int recvcount = 0;        //check which back end server the data from
	int resultnumber = 0; 
	int andlenno = 0;      //and result length
	int orlenno = 0;       //or result length

//correspond to client
	typedef struct{
		int iArraylen;
		int iMessarray[100][3];	
	}FMESSAGE, *RFMESSAGE;

//udp struct and/or
	typedef struct{
		int eAndlen;
		int eAndarray[100][3];
	}ANDMESSAGE, *RANDMESSAGE;

	typedef struct{
		int eOrlen;
		int eOrarray[100][3];
	}ORMESSAGE, *RORMESSAGE;

//the result get from and/or server
	typedef struct{
		int results[100][4];
		int servername;
		int length;
	}RESULT, *PRESULT;

//final result send to client
	typedef struct{
		int total;
		int results[100];
	}CRESULT, *CFRESULT;

	//sock()
	tcp_ServSockdes = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_ServSockdes < 0){
		error("Error opening socket");
	}

	memset(&cli_addr, 0, sizeof(cli_addr));
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(tcpportno);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//to make sure the "address already in use" not happen, avoid TIME_WAIT(search and get from the Internet)
	int on = 1;
	if(setsockopt(tcp_ServSockdes,SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0){
		error("Error on setsockopt");
	}

	//bind()
	if(bind(tcp_ServSockdes, (struct sockaddr *) &serv_addr,sizeof(serv_addr))<0){
		error("Error binding");
	}

	//listen()
	if(listen(tcp_ServSockdes, 1024)<0){
		error("Error listening");
	}

	//deal with the dead process(get it from the Beej's)
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1){
		error("Error sigaction");
	}
	maxsockdes = tcp_ServSockdes;
	cout << "The edge server is up and running" <<endl;

	//initialize
	FD_ZERO(&fdrmid);    //set to empty
	FD_ZERO(&fdwmid);
	FD_SET(tcp_ServSockdes,&fdrmid);   //add new socket

	while(1){
		FD_ZERO(&fdrset);
		fdrset = fdrmid;          //everytime in the loop add the new change to fdrset
		fdwset = fdwmid;    
		timeout.tv_sec = 10; 			//10 sec time out
		timeout.tv_usec = 500000;
		int remainsock = select((maxsockdes+1), &fdrset, &fdwset, NULL, &timeout); //Beej's //exception set to NULL 
		if(remainsock < 0){      //error
			if(tcp_ServSockdes > 0){
				close(tcp_ServSockdes);
				error("Error selecting");
			}
		}
		else if(remainsock == 0){      //time out
			cout << "time out" <<endl;
			continue;
		}
		else{
			//tcp connection read from client exist
			if(FD_ISSET(tcp_ServSockdes, &fdrset) )     //whether there is link from client in the set
			{
				len = sizeof(cli_addr);
				//create child socket using accept()
				tcp_ChildSockdes = accept(tcp_ServSockdes, (struct  sockaddr*)&cli_addr, &len);
				if(tcp_ChildSockdes < 0){
					error("Error accepting");
				}
				else{
					FD_ZERO(&fdrmid);
					FD_SET(tcp_ChildSockdes, &fdrmid);     //add childsocket
					if(tcp_ChildSockdes > tcp_ServSockdes){
						maxsockdes = tcp_ChildSockdes;
					}
				}
			}
			//the second check in () to make sure there isn't any udp link, and last two re cvcount is to make run 
			//not to run into this loop again after alreayd finished received
			//receive from client
			else if(FD_ISSET(tcp_ChildSockdes, &fdrset) && (!FD_ISSET(udp_ServSockdes, &fdrset)) && (recvcount !=1) && (recvcount!=2) && (recvcount != 3)){     //已经accept()
				int length = recv(tcp_ChildSockdes, buffer, sizeof(buffer),0);   //return the real byte number in buffer
				if(length <= 0){
					if(tcp_ChildSockdes > 0){
						close(tcp_ChildSockdes);
						error("Error receiveing");
					}
				}
				else{ 
					RFMESSAGE message = (RFMESSAGE)buffer;
					//deal with received message
					for(int i = 0; i<(message->iArraylen); i++){
						iRecvarray[i][0] = message->iMessarray[i][0];
						iRecvarray[i][1] = message->iMessarray[i][1];
						iRecvarray[i][2] = message->iMessarray[i][2];
						alllength++;
					}

					filelength = message->iArraylen;
					cout << "The edge server has received " << message->iArraylen << " lines from the client using TCP over port 23145" <<endl;

				    //deal with the back end, set connection(udp)
					struct sockaddr_in addr;
					addr.sin_family = AF_INET;
					addr.sin_port = htons(udpedgeportno);
					addr.sin_addr.s_addr = inet_addr("127.0.0.1");

					//socket()
					udp_ServSockdes = socket(AF_INET,SOCK_DGRAM, 0);
					if(udp_ServSockdes < 0){
						error("error opening udp socket");
					}
					//bind()
					if(bind(udp_ServSockdes, (struct sockaddr *)&addr, sizeof(addr)) < 0){
						if(udp_ServSockdes > 0){
							close(udp_ServSockdes);
						}
						error("Error binding");
					}

					// add udp to write
					FD_ZERO(&fdwmid);     //Beej's
					FD_ZERO(&fdrmid);

					FD_SET(udp_ServSockdes,&fdwmid);   //prepare to send
					FD_SET(tcp_ChildSockdes,&fdrmid);    

					if(udp_ServSockdes > maxsockdes){   //new
						maxsockdes = udp_ServSockdes;
					}
				}
			}
			else if(FD_ISSET(udp_ServSockdes,&fdwset)){       //send to back end
				struct sockaddr_in and_addr, or_addr;
				and_addr.sin_family = AF_INET;
				or_addr.sin_family = AF_INET;
				and_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
				or_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
				and_addr.sin_port = htons(udpandportno);
				or_addr.sin_port = htons(udporportno);
				ANDMESSAGE strAndInfo = {0};
				ORMESSAGE strOrInfo = {0};
				int andlength = 0;
				int orlength = 0;
				for(int i=0; i<alllength;i++ ){
					if(iRecvarray[i][0] == 16){         //and
						strAndInfo.eAndarray[andlength][0] = i;
						strAndInfo.eAndarray[andlength][1] = iRecvarray[i][1];
						strAndInfo.eAndarray[andlength][2] = iRecvarray[i][2];
						andlength++;
					}
					if(iRecvarray[i][0] == 17){                 //or
						strOrInfo.eOrarray[orlength][0] = i;           //line no.
						strOrInfo.eOrarray[orlength][1] = iRecvarray[i][1];
						strOrInfo.eOrarray[orlength][2] = iRecvarray[i][2];
						orlength++;
					}
				}
				strAndInfo.eAndlen = andlength;          //length of and
				strOrInfo.eOrlen = orlength;             //length of or

				//sendto()
				int a = sendto(udp_ServSockdes, (char *)&strAndInfo,sizeof(ANDMESSAGE),0, (struct sockaddr *)&and_addr, sizeof(and_addr));
				int o = sendto(udp_ServSockdes, (char *)&strOrInfo,sizeof(ORMESSAGE),0, (struct sockaddr *)&or_addr, sizeof(or_addr));
				if(a < 0){
					error("Error sending to and-server");
				}else{
					count++;
					cout << "The edge has successfully sent " << andlength << " lines to Backend_Server AND." << endl;
				}
				if(o < 0){
					error("Error sending to or-server");
				}else{
					count++;
					cout << "The edge has successfully sent " << orlength << " lines to Backend_Server OR." << endl;
				}
				if(count == 2){
					FD_ZERO(&fdrmid);
					FD_SET(udp_ServSockdes,&fdrmid);
					FD_CLR(udp_ServSockdes,&fdwmid);
				}
			}
			else if(FD_ISSET(udp_ServSockdes, &fdrset)){     //receive from back end
				char buffer[2048] = {0};
				struct sockaddr_in backaddr;
				int n;
				socklen_t backlength = sizeof(backlength);

				//recvfrom()
				n = recvfrom(udp_ServSockdes, buffer, sizeof(buffer), 0, (struct sockaddr*)&backaddr, &backlength);
				recvcount ++;	
				if(n>0){
					if(recvcount == 1){
						cout << "The edge server start receiving the computation results from Backend-Server OR and Backend-Server AND using UDP over port " << udpedgeportno <<endl;
						cout << "The computation results are." <<endl;
						PRESULT presult = (PRESULT)buffer;
						int servername = presult->servername;
						if(servername == 16){
							for(int i=0;i<(presult->length); i++){
								iAndRecvarray[i][0] = presult->results[i][0];     //line no.
								iAndRecvarray[i][1] = presult->results[i][1];     //result
								andlenno = presult->length;
								cout << presult->results[i][2] << " and " << presult->results[i][3] << " = " << presult->results[i][1] <<endl;
							}
						}
						if(servername == 17){
							for(int i=0;i<(presult->length); i++){
								iOrRecvarray[i][0] = presult->results[i][0];     
								iOrRecvarray[i][1] = presult->results[i][1];    
								orlenno = presult->length;
								cout << presult->results[i][2] << " or " << presult->results[i][3] << " = " << presult->results[i][1] <<endl;
							}
						}
					}
					if(recvcount == 2){
						PRESULT presult1 = (PRESULT)buffer;
						int servername = presult1->servername;
						if(servername == 17){
							for(int i=0;i<(presult1->length); i++){
								iOrRecvarray[i][0] = presult1->results[i][0];     //line no.
								iOrRecvarray[i][1] = presult1->results[i][1];     //result
								orlenno = presult1->length;
								cout << presult1->results[i][2] << " or " << presult1->results[i][3] << " = " << presult1->results[i][1] << endl;
							}
						}
						if(servername == 16){
							for(int i=0;i<(presult1->length); i++){
								iAndRecvarray[i][0] = presult1->results[i][0];     
								iAndRecvarray[i][1] = presult1->results[i][1];     
								andlenno = presult1->length;
								cout << presult1->results[i][2] << " and " << presult1->results[i][3] << " = " << presult1->results[i][1] <<endl;
							}
						}
					}
				}
				if(recvcount == 2){       //in order to get the result from both backend
					cout << "The edge server has successfully finished receiving all computation results from Backend-Server OR and Backend-Server AND" <<endl;
						CRESULT cresult;
						int andflength = sizeof(iAndRecvarray)/sizeof(int);
						int andslength = sizeof(iAndRecvarray[0])/sizeof(int);
						int fflength = andflength/andslength;        //and line's length
						int orflength = sizeof(iOrRecvarray)/sizeof(int);
						int orslength = sizeof(iOrRecvarray)/sizeof(int);
						int sslength = orflength/orslength;         //or line's length
						int total = fflength + sslength;
						int totallength = andlenno+orlenno;     //equal to the real length in the file
						for(int totalcount=0; totalcount<totallength;totalcount++){
							for(int i=0; i<andlenno;i++){
								if(totalcount == iAndRecvarray[i][0]){
									cresult.results[totalcount] = iAndRecvarray[i][1];
								}
							}
						}
						for(int totalcount=0; totalcount<totallength;totalcount++){
							for(int i=0; i<orlenno;i++){
								if(totalcount == iOrRecvarray[i][0]){
									cresult.results[totalcount] = iOrRecvarray[i][1];
								}
							}
						}

						cresult.total = totallength;
						int final = send(tcp_ChildSockdes, (char *)&cresult, sizeof(CRESULT),0);
						if(final < 0){
							error("Error sending to client");
							if(tcp_ServSockdes > 0){
								close(tcp_ServSockdes);
							}
						}
						else{
							cout << "The edge server has successfully finished sending all computation results to the client." <<endl;
							FD_ZERO(&fdwset);
							FD_ZERO(&fdrset);
							break;
						}
				}
			}	
		}
	}

}




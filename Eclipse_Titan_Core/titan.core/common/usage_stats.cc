/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Ormandi, Matyas
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include "usage_stats.hh"

#ifdef MINGW
# include <windows.h>
# include <lmcons.h>
#else
# include <pwd.h>
#endif

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <time.h>

#define SA      struct sockaddr
#define MAXLINE 4096
#define MAXSUB  2000

#define LISTENQ         1024

std::string UsageData::id;
std::string UsageData::host;
std::string UsageData::platform;

#ifdef sun
extern int gethostname( char *name, int namelen );
#endif

static int countDots (const char *name )
{
    int n;
    for (n=0; *name; ++name) if (*name=='.') ++n;
    return(n);
}

std::string gethostnameFullyQualified ( void )
{
  char host[774];
  struct hostent *hp;
  char *fqname=NULL;
  int nd;
  host[0] = 0;

  if (-1 == gethostname(host, sizeof(host))) {
    //perror("warning - getting local hostname");
    strcpy(host,"localhost");
  }
  fqname = host;
  nd = countDots(fqname);

  hp = gethostbyname(host);
  if (!hp) {
    //fprintf(stderr, "warning - can't gethostbyname(%s): %s\n",host, hstrerror(h_errno));
  }
  else {
    char **nm;
    if (nd <= countDots(hp->h_name)) {
      fqname = const_cast<char*>(hp->h_name);
      nd = countDots(fqname);
    }
	
    for (nm = hp->h_aliases; *nm; ++nm) {
      int d = countDots(*nm);
      if (d > nd) {fqname = *nm; nd=d;}
    }
  }
  
  if (2 > nd) {
   /* still need to find a domain, look through the usual suspects:
    *    LOCALDOMAIN env variable
    *    domain defn from /etc/resolv.conf
    *    /etc/defaultdomain  (sun only?)
    */
    FILE *fp = NULL;
    char domain[1024];
    char *e = getenv("LOCALDOMAIN");
    if (e) strcpy(domain, e);
    else domain[0] = 0;

    if( !domain[0] && NULL != (fp=fopen("/etc/resolv.conf","r")) ) {
      nd = 0;
      while( fgets(domain, sizeof(domain), fp) ) {
        if( 0==strncmp("domain ",domain,7) ) {
          nd = strlen(domain) - 7;
          memmove(domain, domain+7, nd);
        }
      }
      domain[nd] = 0;  /* nul terminate (or reset empty) */
      fclose(fp);
    }

    if( !domain[0] && NULL != (fp=fopen("/etc/defaultdomain","r")) ) {
      (void)fgets(domain, sizeof(domain), fp);
      fclose(fp);
    }

    if( domain[0] ) {
     /* trim blanks */
      int first = 0;
      nd = strlen(domain) - 1;
      while (first <= nd && isspace(domain[first])) ++first;
      while (nd > first && isspace(domain[nd])) domain[nd--] = 0;

      if (domain[first]) {
        if (fqname != host) strcpy(host,fqname);
        if ('.'!=domain[first]) strcat(host,".");
        strcat(host,domain+first);
        fqname = host;
      }
    }
  }
  return(std::string(fqname));
}

UsageData::UsageData() {

#ifdef MINGW
  TCHAR user_name[UNLEN + 1], computer_name[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD buffer_size = sizeof(user_name);
  if (GetUserName(user_name, &buffer_size)) id=user_name;
  else id="unknown";

  buffer_size = sizeof(computer_name);
  if (GetComputerName(computer_name, &buffer_size))
    host=computer_name;
  else host="unknown";
#else
  struct passwd *p;
  setpwent();
  p = getpwuid(getuid());
  if (p != NULL)
    id = p->pw_name;
  else
    id="unknown";
  endpwent();

  host = gethostnameFullyQualified();

  struct utsname name;

  int result = uname(&name);
  if (result >= 0)
    platform= std::string(name.sysname) +  " " + std::string(name.release) + " " + std::string(name.machine);
  else
    platform="unknown";

#ifdef LINUX
  std::string dist;
  FILE *fp;
  char path[128];

  fp = popen("lsb_release -a 2>/dev/null", "r");
  if (fp != NULL) {
    while (fgets(path, sizeof(path), fp) != NULL)
	  if (strncmp ("Description",path,11) == 0) dist.append(path);
    //printf("\n'%s'\n", dist.c_str());
    pclose(fp);
  }
  dist.erase(0,12);
  platform.append(dist);
#endif

#endif
}

UsageData::~UsageData() {

}


pthread_t UsageData::sendDataThreaded(std::string msg, thread_data* data) {
  data->msg = "id="+ id + "&host=" + host + "&platform=" + platform +
    "&gccv=" + C_COMPILER_VERSION + "&titanv=" + PRODUCT_NUMBER + "&msg="+ msg + "\r";

  pthread_t thread;
  pthread_create(&thread, NULL, sendData, data);
  return thread;
}

void* UsageData::sendData(void* m) {
  // make sure the thread is cancelable if the main thread finishes first
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  
  thread_data* my_data;
  my_data = (thread_data*)m;

  if(my_data->sndr) {
    my_data->sndr->send(my_data->msg.c_str());
  }
  
  return NULL;
}


//**************** HttpSender *******************

ssize_t process_http(int sockfd, const char *host, const char *page, const char *poststr)
{
  char sendline[MAXLINE + 1];//, recvline[MAXLINE + 1];
  ssize_t n = 0;
  //printf("\n ##### \n%s\n ##### \n", poststr);
  snprintf(sendline, MAXSUB,
     "POST %s HTTP/1.0\r\n"
     "Host: %s\r\n"
     "Content-type: application/x-www-form-urlencoded\r\n"
     "Content-length: %lu\r\n\r\n"
     "%s", page, host, (unsigned long)strlen(poststr), poststr);

  (void)write(sockfd, sendline, strlen(sendline));
  /*while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
    recvline[n] = '\0';
    printf("%s", recvline);
  }*/
  return n;
}

// msg must be in the right format to process!
// id=x&host=y&platform=z&gccv=v&titanv=t&msg=m\r
void HttpSender::send(const char* msg) {
  int sockfd;
  struct sockaddr_in servaddr, clientaddr;

  char **pptr;
  const char *hname = "ttcn.ericsson.se";
  const char *page = "/download/usage_stats/usage_stats.php";
  const char *poststr = msg;//.c_str();
  //*******************************************************

  char str[50];
  struct hostent *hptr;
  if ((hptr = gethostbyname(hname)) == NULL) {
    /*fprintf(stderr, " gethostbyname error for host: %s: %s",
            hname, hstrerror(h_errno));*/
    return;
  }
  //printf("hostname: %s\n", hptr->h_name);
  if (hptr->h_addrtype == AF_INET
      && (pptr = hptr->h_addr_list) != NULL) {
    //printf("address: %s\n",
           inet_ntop(hptr->h_addrtype, *pptr, str,sizeof(str));
  } else {
    //fprintf(stderr, "Error call inet_ntop \n");
    return;
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  memset(&clientaddr, 0, sizeof(clientaddr));
  clientaddr.sin_family = AF_INET;
  clientaddr.sin_port = htons(49555);
  inet_pton(AF_INET, "0.0.0.0", &clientaddr.sin_addr);
  if (bind(sockfd, (SA *) & clientaddr, sizeof(clientaddr)) < 0) {
    clientaddr.sin_port = htons(59555);
    if (bind(sockfd, (SA *) & clientaddr, sizeof(clientaddr)) < 0) {
      clientaddr.sin_port = htons(61555);
      if (bind(sockfd, (SA *) & clientaddr, sizeof(clientaddr)) < 0) {
        // last ditch effort, use an automatically generated port
        clientaddr.sin_port = htons(0);
        bind(sockfd, (SA *) & clientaddr, sizeof(clientaddr));
      }
    }
  }
          
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(80);
  inet_pton(AF_INET, str, &servaddr.sin_addr);

  connect(sockfd, (SA *) & servaddr, sizeof(servaddr));
  
  process_http(sockfd, hname, page, poststr);
  close(sockfd);
}

//**************** DNSSender *******************

DNSSender::DNSSender() : nameserver("172.31.21.9"), domain("domain.net") { }

void DNSSender::send(const char* /* msg */) {
  //send_over_dns(msg, "TXT", "pass", nameserver, domain);
}


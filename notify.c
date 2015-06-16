#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <jansson.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

// #define __DEBUG__
#define BUFFER    1024

//
// command line arguments
//
static struct option long_options[] = {
	{"host",    required_argument, 0, 'h'},
	{"port",    required_argument, 0, 'p'},
	{"source",  required_argument, 0, 's'},
	{"level",   required_argument, 0, 'l'},
	{"title",   required_argument, 0, 't'},
	{"message", required_argument, 0, 'm'},
	{"tag",     required_argument, 0, 'g'},
	{"fork",    no_argument,       0, 'f'},
	{0, 0, 0, 0}
};

//
// notification data
//
typedef struct notification_t {
	char *host;
	int port;
	char *source;
	char *level;
	char *title;
	char *message;
	char *tag;
	
} notification_t;

//
// levels
//
static char *__levels[] = {"low", "normal", "critical"};

//
// utilities
//
void diep(char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

void dief(char *str) {
	fprintf(stderr, "[-] %s\n", str);
	exit(EXIT_FAILURE);
}

static char *parent(pid_t pid) {
	char *name;
	FILE *fp;
	
	if(!(name = (char *) calloc(BUFFER, sizeof(char))))
		diep("[-] malloc");
	
	sprintf(name, "/proc/%d/cmdline", pid);
	
	if(!(fp = fopen(name, "r"))) {
		perror(name);
		sprintf(name, "default");
		return name;
	}
	
	if(!fread(name, sizeof(char), BUFFER, fp))
		diep("[-] fread");
		
	fclose(fp);
	
	return name;
}

//
// help
//
void usage() {
	printf("notify client options:\n");
	printf(" --host     notified server host\n");
	printf(" --port     notified server port (default 5050)\n");
	printf(" --source   notification source name\n");
	printf(" --level    importance (low, normal, critical)\n");
	printf(" --title    notification title (optional)\n");
	printf(" --message  notification message\n");
	printf(" --tag      internal tag of the message\n");
	printf(" --fork     send message in background (non blocking)\n");
	
	exit(EXIT_FAILURE);
}

//
// socket transfert
//
int transfert(notification_t *notification, char *payload) {
	int fd = -1, connresult;
	struct sockaddr_in host;
	struct hostent *he;
	
	// resolving
	if((he = gethostbyname(notification->host)) == NULL)
		diep("[-] gethostbyname");
	
	bcopy(he->h_addr, &host.sin_addr, he->h_length);

	host.sin_family = AF_INET; 
	host.sin_port = htons(notification->port);

	// socket
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		diep("[-] socket");

	// connection
	if((connresult = connect(fd, (struct sockaddr *) &host, sizeof(host))) < 0)
		diep("[-] connect");
	
	// sending payload
	if(send(fd, payload, strlen(payload), 0) < 0)
		diep("[-] send");
	
	close(fd);
	
	return 0;
}

//
// encoder
//
char *encode(notification_t *notification) {
	json_t *root;
	char *json;
	
	root = json_object();
	json_object_set_new(root, "source", json_string(notification->source));
	json_object_set_new(root, "title", json_string(notification->title));
	json_object_set_new(root, "message", json_string(notification->message));
	json_object_set_new(root, "level", json_string(notification->level));
	
	if(notification->tag)
		json_object_set_new(root, "tag", json_string(notification->tag));
	
	json = json_dumps(root, JSON_INDENT(2));
	json_decref(root);
	
	return json;
}

//
// notification
//
int notifiy(notification_t *notification) {
	unsigned int i;
	char *level = NULL, *payload;
	
	//
	// checking levels
	//
	for(i = 0; i < (sizeof(__levels) / sizeof(char *)); i++) {
		if(!strcmp(__levels[i], notification->level))
			level = __levels[i];
	}
	
	if(!level)
		dief("invalid level");
	
	//
	// checking required fields
	//
	if(!notification->host)
		dief("missing host");
	
	if(!notification->message)
		dief("missing message");
	
	//
	// print summary
	//
	#ifdef __DEBUG__
	printf("[+] host   : %s:%d\n", notification->host, notification->port);
	printf("[+] level  : %s\n", notification->level);
	printf("[+] title  : %s\n", notification->title);
	printf("[+] message: %s\n", notification->message);
	printf("[+] source : %s\n", notification->source);
	printf("[+] tag    : %s\n", notification->tag);
	#endif
	
	//
	// encoding request
	//
	if(!(payload = encode(notification)))
		dief("encoding failed");
	
	#ifdef __DEBUG__
	printf("[+] -------- request --------\n");
	puts(payload);
	printf("[+] -------------------------\n");
	#endif
	
	return transfert(notification, payload);
}

int main(int argc, char *argv[]) {
	int option_index = 0;
	int i;
	int background = 0;
	
	notification_t notification = {
		.host    = NULL,
		.port    = 5050,
		.source  = "default",
		.level   = "normal",
		.title   = "",
		.message = NULL,
		.tag     = NULL
	};
	
	// default source
	notification.source = parent(getppid());
	
	while(1) {
		i = getopt_long(argc, argv, "h:p:s:l:t:m:g", long_options, &option_index);
		
		if(i == -1)
			break;

		switch(i) {
			case 'h': notification.host    = optarg;       break;
			case 'p': notification.port    = atoi(optarg); break;
			case 's': notification.source  = optarg;       break;
			case 'l': notification.level   = optarg;       break;
			case 't': notification.title   = optarg;       break;
			case 'm': notification.message = optarg;       break;
			case 'g': notification.tag     = optarg;       break;
			case 'f': background = 1;                      break;
			
			case '?':
				usage();
			break;

			default:
				abort();
		}
	}
	
	if(background) {
		if(fork()) {
			#ifdef __DEBUG__
			printf("[+] forked, closing parent...\n");
			#endif
			exit(EXIT_SUCCESS);
		}
	}
	
	return notifiy(&notification);
}

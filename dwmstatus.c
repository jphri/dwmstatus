#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

typedef struct Process Process;
struct Process {
	pid_t pid;
	int fd;
	Process *next;
	char *stext;
	int   stextresv, stexti;

	char *ctext;
	unsigned char buffer[256];
	int bufferi, buffersi;
};
static Process *list = NULL;

static const char *leftpad   = "";
static const char *rightpad  = "";
static const char *separator = " | ";

int
max(int a, int b)
{
	return (a > b ? a : b);
}

Process *
newproc(const char *command)
{
	Process *p;
	int pipes[2];
	pid_t pid;
	if(pipe(pipes) < 0) {
		perror("pipe()");
		exit(EXIT_FAILURE);
	}

	int flags = fcntl(pipes[0], F_GETFL, 0);
	if(flags < 0) {
		perror("fcntl()");
		exit(EXIT_FAILURE);
	}

	if(fcntl(pipes[0], F_SETFL, flags | O_NONBLOCK) < 0) {
		perror("fcntl()");
		exit(EXIT_FAILURE);
	}

	switch((pid = fork())) {
	case -1:
		perror("fork()");
		exit(EXIT_FAILURE);
	case 0:
		close(0);
		close(1);
		dup2(pipes[1], 1);
		close(pipes[0]);
		close(pipes[1]);
		execl("/bin/sh", "/bin/sh", "-c", command, NULL);
		exit(EXIT_SUCCESS);
		return NULL;
	default:
		close(pipes[1]);
		p = malloc(sizeof *p);
		if(!p) {
			perror("malloc()");
			exit(EXIT_FAILURE);
		}
		memset(p, 0, sizeof *p);
		p->pid = pid;
		p->fd = pipes[0];
		p->stext = malloc(1);
		p->stextresv = 1;
		return p;
	}
}

static void
sigtrap(int sig)
{
	Process *next;

	next = NULL;
	for(Process *p = list; p; p = next) {
		next = p->next;
		kill(p->pid, sig);
		if(p->ctext)
			free(p->ctext);
		free(p->stext);
		free(p);
	}
	while(wait(NULL) != -1 || errno == EINTR);

	exit(EXIT_SUCCESS);
}

void
print_status()
{
	printf("%s", leftpad);
	for(Process *p = list; p; p = p->next)
		printf("%s%s", p->ctext != NULL ? p->ctext : "(null)" , p->next ? separator : "");
	printf("%s\n", rightpad);
	fflush(stdout);
}

int
procnextchar(Process *p)
{
	if(p->bufferi >= p->buffersi) {
		p->buffersi = read(p->fd, p->buffer, sizeof p->buffer);
		if(p->buffersi < 0)
			return -1;
		p->bufferi = 0;
	}
	return p->buffer[p->bufferi++];
}

int
linerd(Process *p)
{
	int i, n;
	int c;

	for(i = 0; i < 32; i++) {
		if((c = procnextchar(p)) < 0) {
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else {
				return -1;
			}
		}

		if(p->stexti >= p->stextresv) {
			p->stextresv *= 2;
			p->stext = realloc(p->stext, p->stextresv);
			if(!p->stext) {
				perror("realloc()");
				exit(EXIT_FAILURE);
			}
		}

		if(c == '\n') {
			if(p->ctext != NULL)
				free(p->ctext);
			p->stext[p->stexti] = 0;

			p->ctext = p->stext;
			p->stext = malloc(1);
			p->stextresv = 1;
			p->stexti = 0;

			print_status();
			break;
		}

		p->stext[p->stexti++] = c;
	}

	return i;
}

int
main(int argc, char *argv[])
{
	int maxfd = 0, i, c;
	fd_set rdset;
	Process *p;

	signal(SIGINT, sigtrap);
	signal(SIGTERM, sigtrap);
	signal(SIGKILL, sigtrap);

	FD_ZERO(&rdset);

	while((c = getopt(argc, argv, "c:s:l:r:")) > 0) {
		switch(c) {
		case 's':
			separator = optarg;
			break;
		case 'l':
			leftpad = optarg;
			break;
		case 'r':
			rightpad = optarg;
			break;
		case 'c':
			p = newproc(optarg);
			FD_SET(p->fd, &rdset);
			maxfd = max(maxfd, p->fd);
			p->next = list;
			list = p;
			break;
		default:
			fprintf(stderr, "Unknown option: %c\n", c);
			exit(EXIT_FAILURE);
		}
	}

	if(!list) {
		fprintf(stderr, "usage: %s -c command [-c command2 ]...\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	for(;;) {
		int n;
		fd_set copy;
		
		FD_ZERO(&copy);
		memcpy(&copy, &rdset, sizeof rdset);
		n = select(maxfd + 1, &copy, NULL, NULL, NULL);
		if(n < 0) {
			perror("select()");
			continue;
		}

		for(Process *p = list; p; p = p->next)
			if(FD_ISSET(p->fd, &copy)) {
				if(linerd(p) < 0) {
					FD_CLR(p->fd, &rdset);
					close(p->fd);
				}
			}
	}
	
	return 0;
}

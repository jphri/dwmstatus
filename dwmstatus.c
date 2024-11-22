#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

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
	int   stextresv;
};
static Process *list = NULL;

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
	if(sig != SIGINT)
		return;

	for(Process *p = list; p; p = p->next) {
		killpg(p->pid, sig);
	}
	while(wait(NULL) != -1 || errno == EINTR);

	/* let the operating take care of the memory leak, fuck it */
	exit(EXIT_SUCCESS);
}

int
linerd(Process *p)
{
	int i;
	char c;

	for(i = 0;; i++) {
		if(read(p->fd, &c, sizeof c) <= 0) {
			break;
		}

		if(i >= p->stextresv) {
			p->stextresv *= 2;
			p->stext = realloc(p->stext, p->stextresv);
			if(!p->stext) {
				perror("realloc()");
				exit(EXIT_FAILURE);
			}
		}

		if(c == '\n') {
			p->stext[i] = 0;
			break;
		}
		p->stext[i] = c;
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

	while((c = getopt(argc, argv, "c:")) > 0) {
		switch(c) {
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
				if(linerd(p) <= 0) {
					FD_CLR(p->fd, &rdset);
					close(p->fd);
				}
			}

		for(Process *p = list; p; p = p->next)
			printf("%s%s", p->stext, p->next ? " | " : "");

		printf("\n");
		fflush(stdout);
	}
	
	return 0;
}

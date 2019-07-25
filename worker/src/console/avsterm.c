#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>

static void print_welcome (FILE* stream)
{
    fprintf (stream, "Type help <enter> to see a list of commands, Ctrl-D for console exit\n");
}


char USH_PS1[16] = "avsterm> ";

static struct termios old_mode;
static char command[64*1024];

static void make_term_mode()
{
    struct termios new_mode;

    tcgetattr(fileno(stdin), &old_mode);

    new_mode = old_mode;
    memcpy(&new_mode, &old_mode, sizeof(struct termios));

    if(new_mode.c_lflag & ICANON)
    {
        new_mode.c_cc[VERASE] = 'H' & ~0140;
        new_mode.c_lflag &= ~ISIG;
        tcsetattr(fileno(stdin), TCSADRAIN, &new_mode);
    }
}

void recover_term_mode()
{
    tcsetattr(fileno(stdin), TCSADRAIN, &old_mode);
}

static int open_unix_socket(const char *usock_name)
{
    int     sockfd;
    struct sockaddr_un  servaddr;

    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);

    unlink(usock_name);            /* OK if this fails */

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sun_family = AF_LOCAL;
    strncpy(servaddr.sun_path, usock_name, sizeof(servaddr.sun_path) - 1);

    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    return sockfd;
}

static void unescapeCommand(char *command, int len)
{
    char *pCursor = command;
    int i;
    for (i=0; i<len; )
    {
        if ((command[i] == '\\') && ((i+1) < len) &&
          strchr("nrtx", (int)command[i+1]))
        {
            if ((int)command[i+1] == 'n')
            {
                *pCursor++ = '\n';
                i += 2;
            }
            else if ((int)command[i+1] == 'x')
            {
                int x, ret;
                ret = sscanf(command + i, "\\x%2x", &x);
                if (ret)
                {
                    *pCursor++ = (char)(x & 0xFF);
                    if ((i + 3 < len) &&
                      strchr("0123456789abcdefABCDEF", (int)command[i+3]))
                    {
                        i += 4;
                    }
                    else
                    {
                        i += 3;
                    }
                }
                else
                {
                    *pCursor++ = '\\';
                    *pCursor++ = 'x';
                    i += 2;
                }
            }
            else if ((int)command[i+1] == 't')
            {
                *pCursor++ = '\t';
                i += 2;
            }
            else
            {
                *pCursor++ = '\r';
                i += 2;
            }
        }
        else
        {
            *pCursor++ = command[i++];
        }
    }
    *pCursor = '\0';
}

static int proc_stdin_to_usock(int usock, struct sockaddr_un servaddr)
{
    int len;

    memset(command, 0, sizeof(command));
    if (!fgets(command, sizeof(command), stdin))
    {
        return -1;
    }

    /* remove trailing newline */
    len = strlen(command) - 1;
    len = (len < 0)? 0 : len;
#if 1
    unescapeCommand(command, len);
//    command[len] = '\0';
#endif
//    printf("command %s", command);
//    return 0;

	return sendto(usock , command, len, 0, \
	             (struct sockaddr *)&servaddr, sizeof(servaddr));
}

static int proc_usock_to_stdout(int usock, struct sockaddr_un servaddr)
{
    int ret;

    memset(command, 0, sizeof(command));
    ret = recv(usock, command, sizeof(command), 0);
	if (ret < 0)
	{
		perror("log fifo not useable\n");
		return -1;
	}
	else if (0 != ret)
	{
//	    printf("ret = %d/%d\n", ret, sizeof(command));
		command[ret] = '\0';
		printf("%s\n", command);
	}

	return ret;
}

int main(int argc, char *argv[])
{
    char locname[16] = "/tmp/pntServer";
    //char locname[16] = "/tmp/ushXXXXXX";

    struct sockaddr_un servaddr;

    fd_set watchset;       /* fds to read from */
    fd_set inset;          /* updated by select() */

    int ret;
    int maxfd;
    int len;
    int fd_log;
	int next_option;
    int input_enable = 1;
    //mkstemp(locname);
    //printf("local name %s\n", locname);
    print_welcome(stdout);

    fd_log = open_unix_socket(locname);
    if (-1 == fd_log) {
        printf("can't open locale part socket, program aborted\n");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sun_family = AF_UNIX;
    snprintf(servaddr.sun_path, sizeof(servaddr.sun_path), "%s", "/tmp/avsterm");//modify socket path

    {
        /* don't pay any attention to this signal; it just confuses
           things and isn't really meant for shells anyway */
        signal(SIGTTOU, SIG_IGN);

        make_term_mode();


        /* start off reading from both file descriptors */
        FD_ZERO(&watchset);
        if (input_enable)
            FD_SET(0, &watchset);	// stdin fd
        FD_SET(fd_log, &watchset);

        maxfd = fd_log + 1;

        printf("%s", USH_PS1);
        fflush(stdout);

        while (1)
        {
//            printf("%s", USH_PS1);
//            fflush(stdout);

            inset = watchset;
            if (select(maxfd, &inset, NULL, NULL, NULL) < 0) {
                if (EINTR == errno) {
                    continue;
                }

                perror("select");
                return 1;
            }

    		if (FD_ISSET(0, &inset)) {
                ret = proc_stdin_to_usock(fd_log, servaddr);
                if (0 == ret) {
                    printf("%s", USH_PS1);
                    fflush(stdout);
                } else if (-1 == ret) {
                    printf("\n");
                    break;
                }
    		}

    		if (FD_ISSET(fd_log, &inset)) {
                proc_usock_to_stdout(fd_log, servaddr);
    		}
        }

        recover_term_mode();
        printf("\n");
    }

    unlink(locname);
    close(fd_log);

    exit(0);

}

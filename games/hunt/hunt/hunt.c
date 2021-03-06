/*	$NetBSD: hunt.c,v 1.24 2006/05/09 20:18:06 mrg Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hunt.c,v 1.24 2006/05/09 20:18:06 mrg Exp $");
#endif /* not lint */

# include	<sys/param.h>
# include	<sys/stat.h>
# include	<sys/time.h>
# include	<sys/poll.h>
# include	<ctype.h>
# include	<err.h>
# include	<errno.h>
# include	<curses.h>
# include	<signal.h>
# include	<stdlib.h>
# include	<string.h>
# if !defined(USE_CURSES) && defined(BSD_RELEASE) && BSD_RELEASE >= 44
# include	<termios.h>
static struct termios saved_tty;
# endif
# include	<unistd.h>
# include	<ifaddrs.h>

# include	"hunt.h"

/*
 * Some old versions of curses don't have these defined
 */
# if !defined(cbreak) && (!defined(BSD_RELEASE) || BSD_RELEASE < 44)
# define	cbreak()	crmode()
# endif

# if !defined(USE_CURSES) || !defined(TERMINFO)
# define	beep()		(void) putchar(CTRL('G'))
# endif
# if !defined(USE_CURSES)
# undef		refresh
# define	refresh()	(void) fflush(stdout);
# endif
# ifdef USE_CURSES
# define	clear_eol()	clrtoeol()
# define	put_ch		addch
# define	put_str		addstr
# endif

FLAG	Last_player = FALSE;
# ifdef MONITOR
FLAG	Am_monitor = FALSE;
# endif

char	Buf[BUFSIZ];

int	Socket;
# ifdef INTERNET
char	*Sock_host;
char	*use_port;
FLAG	Query_driver = FALSE;
char	*Send_message = NULL;
FLAG	Show_scores = FALSE;
# endif

SOCKET	Daemon;
# ifdef	INTERNET
# define	DAEMON_SIZE	(sizeof Daemon)
# else
# define	DAEMON_SIZE	(sizeof Daemon - 1)
# endif

char	map_key[256];			/* what to map keys to */
FLAG	no_beep;

static char	name[NAMELEN];
static char	team = ' ';

static int	in_visual;

extern int	cur_row, cur_col;

void	dump_scores(SOCKET);
long	env_init(long);
void	fill_in_blanks(void);
void	leave(int, char *) __attribute__((__noreturn__));
void	leavex(int, char *) __attribute__((__noreturn__));
void	fincurs(void);
int	main(int, char *[]);
# ifdef INTERNET
SOCKET *list_drivers(void);
# endif

extern int	Otto_mode;
/*
 * main:
 *	Main program for local process
 */
int
main(ac, av)
	int	ac;
	char	**av;
{
	char		*term;
	int		c;
	long		enter_status;

	enter_status = env_init((long) Q_CLOAK);
	while ((c = getopt(ac, av, "Sbcfh:l:mn:op:qst:w:")) != -1) {
		switch (c) {
		case 'l':	/* rsh compatibility */
		case 'n':
			(void) strncpy(name, optarg, NAMELEN);
			break;
		case 't':
			team = *optarg;
			if (!isdigit((unsigned char)team)) {
				warnx("Team names must be numeric");
				team = ' ';
			}
			break;
		case 'o':
# ifndef OTTO
			warnx("The -o flag is reserved for future use.");
			goto usage;
# else
			Otto_mode = TRUE;
			break;
# endif
		case 'm':
# ifdef MONITOR
			Am_monitor = TRUE;
# else
			warnx("The monitor was not compiled in.");
# endif
			break;
# ifdef INTERNET
		case 'S':
			Show_scores = TRUE;
			break;
		case 'q':	/* query whether hunt is running */
			Query_driver = TRUE;
			break;
		case 'w':
			Send_message = optarg;
			break;
		case 'h':
			Sock_host = optarg;
			break;
		case 'p':
			use_port = optarg;
			Test_port = atoi(use_port);
			break;
# else
		case 'S':
		case 'q':
		case 'w':
		case 'h':
		case 'p':
			wanrx("Need TCP/IP for S, q, w, h, and p options.");
			break;
# endif
		case 'c':
			enter_status = Q_CLOAK;
			break;
		case 'f':
# ifdef FLY
			enter_status = Q_FLY;
# else
			warnx("The flying code was not compiled in.");
# endif
			break;
		case 's':
			enter_status = Q_SCAN;
			break;
		case 'b':
			no_beep = !no_beep;
			break;
		default:
		usage:
			fputs(
"usage:\thunt [-qmcsfS] [-n name] [-t team] [-p port] [-w message] [host]\n",
			stderr);
			exit(1);
		}
	}
# ifdef INTERNET
	if (optind + 1 < ac)
		goto usage;
	else if (optind + 1 == ac)
		Sock_host = av[ac - 1];
# else
	if (optind > ac)
		goto usage;
# endif

# ifdef INTERNET
	if (Show_scores) {
		SOCKET	*hosts;

		for (hosts = list_drivers(); hosts->sin_port != 0; hosts += 1)
			dump_scores(*hosts);
		exit(0);
	}
	if (Query_driver) {
		SOCKET	*hosts;

		for (hosts = list_drivers(); hosts->sin_port != 0; hosts += 1) {
			struct	hostent	*hp;
			int	num_players;

			hp = gethostbyaddr((char *) &hosts->sin_addr,
					sizeof hosts->sin_addr, AF_INET);
			num_players = ntohs(hosts->sin_port);
			printf("%d player%s hunting on %s!\n",
				num_players, (num_players == 1) ? "" : "s",
				hp != NULL ? hp->h_name :
				inet_ntoa(hosts->sin_addr));
		}
		exit(0);
	}
# endif
# ifdef OTTO
	if (Otto_mode)
		(void) strncpy(name, "otto", NAMELEN);
	else
# endif
	fill_in_blanks();

	(void) fflush(stdout);
	if (!isatty(0) || (term = getenv("TERM")) == NULL)
		errx(1, "no terminal type");
# ifdef USE_CURSES
	initscr();
	(void) noecho();
	(void) cbreak();
# else /* !USE_CURSES */
# if !defined(BSD_RELEASE) || BSD_RELEASE < 44
	_tty_ch = 0;
# endif
	gettmode();
	(void) setterm(term);
	(void) noecho();
	(void) cbreak();
# if defined(BSD_RELEASE) && BSD_RELEASE >= 44
	tcgetattr(0, &saved_tty);
# endif
	_puts(TI);
	_puts(VS);
# endif /* !USE_CURSES */
	in_visual = TRUE;
	if (LINES < SCREEN_HEIGHT || COLS < SCREEN_WIDTH)
		leavex(1, "Need a larger window");
	clear_the_screen();
	(void) signal(SIGINT, intr);
	(void) signal(SIGTERM, sigterm);
	(void) signal(SIGUSR1, sigusr1);
	(void) signal(SIGPIPE, SIG_IGN);
#if !defined(USE_CURSES) && defined(SIGTSTP)
	(void) signal(SIGTSTP, tstp);
#endif

	for (;;) {
# ifdef	INTERNET
		find_driver(TRUE);

		if (Daemon.sin_port == 0)
			leavex(1, "Game not found, try again");

	jump_in:
		do {
			int	option;

			Socket = socket(SOCK_FAMILY, SOCK_STREAM, 0);
			if (Socket < 0)
				err(1, "socket");
			option = 1;
			if (setsockopt(Socket, SOL_SOCKET, SO_USELOOPBACK,
			    &option, sizeof option) < 0)
				warn("setsockopt loopback");
			errno = 0;
			if (connect(Socket, (struct sockaddr *) &Daemon,
			    DAEMON_SIZE) < 0) {
				if (errno != ECONNREFUSED) {
					leave(1, "connect");
				}
			}
			else
				break;
			sleep(1);
		} while (close(Socket) == 0);
# else /* !INTERNET */
		/*
		 * set up a socket
		 */

		if ((Socket = socket(SOCK_FAMILY, SOCK_STREAM, 0)) < 0)
			err(1, "socket");

		/*
		 * attempt to connect the socket to a name; if it fails that
		 * usually means that the driver isn't running, so we start
		 * up the driver.
		 */

		Daemon.sun_family = SOCK_FAMILY;
		(void) strcpy(Daemon.sun_path, Sock_name);
		if (connect(Socket, &Daemon, DAEMON_SIZE) < 0) {
			if (errno != ENOENT) {
				leavex(1, "connect2");
			}
			start_driver();

			do {
				(void) close(Socket);
				if ((Socket = socket(SOCK_FAMILY, SOCK_STREAM,
				    0)) < 0)
					err(1, "socket");
				sleep(2);
			} while (connect(Socket, &Daemon, DAEMON_SIZE) < 0);
		}
# endif

		do_connect(name, team, enter_status);
# ifdef INTERNET
		if (Send_message != NULL) {
			do_message();
			if (enter_status == Q_MESSAGE)
				break;
			Send_message = NULL;
			/* don't continue as that will call find_driver */
			goto jump_in;
		}
# endif
		playit();
		if ((enter_status = quit(enter_status)) == Q_QUIT)
			break;
	}
	leavex(0, (char *) NULL);
	/* NOTREACHED */
	return(0);
}

# ifdef INTERNET
# ifdef BROADCAST
int
broadcast_vec(s, vector)
	int			s;		/* socket */
	struct	sockaddr	**vector;
{
	int			vec_cnt;
	struct ifaddrs		*ifp, *ip;

	*vector = NULL;
	if (getifaddrs(&ifp) < 0)
		return 0;

	vec_cnt = 0;
	for (ip = ifp; ip; ip = ip->ifa_next)
		if ((ip->ifa_addr->sa_family == AF_INET) &&
		    (ip->ifa_flags & IFF_BROADCAST))
			vec_cnt++;

	*vector = (struct sockaddr *)
		malloc(vec_cnt * sizeof(struct sockaddr_in));

	vec_cnt = 0;
	for (ip = ifp; ip; ip = ip->ifa_next)
		if ((ip->ifa_addr->sa_family == AF_INET) &&
		    (ip->ifa_flags & IFF_BROADCAST))
			memcpy(&(*vector)[vec_cnt++], ip->ifa_broadaddr,
			       sizeof(struct sockaddr_in));

	freeifaddrs(ifp);
	return vec_cnt;
}
# endif

SOCKET	*
list_drivers()
{
	int			option;
	u_short			msg;
	u_short			port_num;
	static SOCKET		test;
	int			test_socket;
	socklen_t		namelen;
	char			local_name[MAXHOSTNAMELEN + 1];
	static int		initial = TRUE;
	static struct in_addr	local_address;
	struct hostent		*hp;
# ifdef BROADCAST
	static	int		brdc;
	static	SOCKET		*brdv;
# else
	u_long			local_net;
# endif
	int			i;
	static	SOCKET		*listv;
	static	unsigned int	listmax;
	unsigned int		listc;
	struct pollfd		set[1];

	if (initial) {			/* do one time initialization */
# ifndef BROADCAST
		sethostent(1);		/* don't bother to close host file */
# endif
		if (gethostname(local_name, sizeof local_name) < 0) {
			leavex(1, "Sorry, I have no name.");
			/* NOTREACHED */
		}
		local_name[sizeof(local_name) - 1] = '\0';
		if ((hp = gethostbyname(local_name)) == NULL) {
			leavex(1, "Can't find myself.");
			/* NOTREACHED */
		}
		local_address = * ((struct in_addr *) hp->h_addr);

		listmax = 20;
		listv = (SOCKET *) malloc(listmax * sizeof (SOCKET));
	} else if (Sock_host != NULL)
		return listv;		/* address already valid */

	test_socket = socket(SOCK_FAMILY, SOCK_DGRAM, 0);
	if (test_socket < 0) {
		leave(1, "socket system call failed");
		/* NOTREACHED */
	}
	test.sin_family = SOCK_FAMILY;
	test.sin_port = htons(Test_port);
	listc = 0;

	if (Sock_host != NULL) {	/* explicit host given */
		if ((hp = gethostbyname(Sock_host)) == NULL) {
			leavex(1, "Unknown host");
			/* NOTREACHED */
		}
		test.sin_addr = *((struct in_addr *) hp->h_addr);
		goto test_one_host;
	}

	if (!initial) {
		/* favor host of previous session by broadcasting to it first */
		test.sin_addr = Daemon.sin_addr;
		msg = htons(C_PLAYER);		/* Must be playing! */
		(void) sendto(test_socket, (char *) &msg, sizeof msg, 0,
		    (struct sockaddr *) &test, DAEMON_SIZE);
	}

# ifdef BROADCAST
	if (initial)
		brdc = broadcast_vec(test_socket, (void *) &brdv);

# ifdef SO_BROADCAST
	/* Sun's will broadcast even though this option can't be set */
	option = 1;
	if (setsockopt(test_socket, SOL_SOCKET, SO_BROADCAST,
	    &option, sizeof option) < 0) {
		leave(1, "setsockopt broadcast");
		/* NOTREACHED */
	}
# endif

	/* send broadcast packets on all interfaces */
	msg = htons(C_TESTMSG());
	for (i = 0; i < brdc; i++) {
		test.sin_addr = brdv[i].sin_addr;
		if (sendto(test_socket, (char *) &msg, sizeof msg, 0,
		    (struct sockaddr *) &test, DAEMON_SIZE) < 0) {
			leave(1, "sendto");
			/* NOTREACHED */
		}
	}
	test.sin_addr = local_address;
	if (sendto(test_socket, (char *) &msg, sizeof msg, 0,
	    (struct sockaddr *) &test, DAEMON_SIZE) < 0) {
		leave(1, "sendto");
		/* NOTREACHED */
	}
# else /* !BROADCAST */
	/* loop thru all hosts on local net and send msg to them. */
	msg = htons(C_TESTMSG());
	local_net = inet_netof(local_address);
	sethostent(0);		/* rewind host file */
	while (hp = gethostent()) {
		if (local_net == inet_netof(* ((struct in_addr *) hp->h_addr))){
			test.sin_addr = * ((struct in_addr *) hp->h_addr);
			(void) sendto(test_socket, (char *) &msg, sizeof msg, 0,
			    (struct sockaddr *) &test, DAEMON_SIZE);
		}
	}
# endif

get_response:
	namelen = DAEMON_SIZE;
	errno = 0;
	set[0].fd = test_socket;
	set[0].events = POLLIN;
	for (;;) {
		if (listc + 1 >= listmax) {
			listmax += 20;
			listv = (SOCKET *) realloc((char *) listv,
						listmax * sizeof(SOCKET));
		}

		if (poll(set, 1, 1000) == 1 &&
		    recvfrom(test_socket, (char *) &port_num, sizeof(port_num),
		    0, (struct sockaddr *) &listv[listc], &namelen) > 0) {
			/*
			 * Note that we do *not* convert from network to host
			 * order since the port number *should* be in network
			 * order:
			 */
			for (i = 0; i < listc; i += 1)
				if (listv[listc].sin_addr.s_addr
				== listv[i].sin_addr.s_addr)
					break;
			if (i == listc)
				listv[listc++].sin_port = port_num;
			continue;
		}

		if (errno != 0 && errno != EINTR) {
			leave(1, "poll/recvfrom");
			/* NOTREACHED */
		}

		/* terminate list with local address */
		listv[listc].sin_family = SOCK_FAMILY;
		listv[listc].sin_addr = local_address;
		listv[listc].sin_port = htons(0);

		(void) close(test_socket);
		initial = FALSE;
		return listv;
	}

test_one_host:
	msg = htons(C_TESTMSG());
	(void) sendto(test_socket, (char *) &msg, sizeof msg, 0,
	    (struct sockaddr *) &test, DAEMON_SIZE);
	goto get_response;
}

void
find_driver(do_startup)
	FLAG	do_startup;
{
	SOCKET	*hosts;

	hosts = list_drivers();
	if (hosts[0].sin_port != htons(0)) {
		int	i, c;

		if (hosts[1].sin_port == htons(0)) {
			Daemon = hosts[0];
			return;
		}
		/* go thru list and return host that matches daemon */
		clear_the_screen();
# ifdef USE_CURSES
		move(1, 0);
# else
		mvcur(cur_row, cur_col, 1, 0);
		cur_row = 1;
		cur_col = 0;
# endif
		put_str("Pick one:");
		for (i = 0; i < HEIGHT - 4 && hosts[i].sin_port != htons(0);
								i += 1) {
			struct	hostent	*hp;
			char	buf[80];

# ifdef USE_CURSES
			move(3 + i, 0);
# else
			mvcur(cur_row, cur_col, 3 + i, 0);
			cur_row = 3 + i;
			cur_col = 0;
# endif
			hp = gethostbyaddr((char *) &hosts[i].sin_addr,
					sizeof hosts[i].sin_addr, AF_INET);
			(void) sprintf(buf, "%8c    %.64s", 'a' + i,
				hp != NULL ? hp->h_name
				: inet_ntoa(hosts->sin_addr));
			put_str(buf);
		}
# ifdef USE_CURSES
		move(4 + i, 0);
# else
		mvcur(cur_row, cur_col, 4 + i, 0);
		cur_row = 4 + i;
		cur_col = 0;
# endif
		put_str("Enter letter: ");
		refresh();
		while (!islower(c = getchar()) || (c -= 'a') >= i) {
			beep();
			refresh();
		}
		Daemon = hosts[c];
		clear_the_screen();
		return;
	}
	if (!do_startup)
		return;

	start_driver();
	sleep(2);
	find_driver(FALSE);
}

void
dump_scores(host)
	SOCKET	host;
{
	struct	hostent	*hp;
	int	s;
	char	buf[BUFSIZ];
	int	cnt;

	hp = gethostbyaddr((char *) &host.sin_addr, sizeof host.sin_addr,
								AF_INET);
	printf("\n%s:\n", hp != NULL ? hp->h_name : inet_ntoa(host.sin_addr));
	fflush(stdout);

	s = socket(SOCK_FAMILY, SOCK_STREAM, 0);
	if (s < 0)
		err(1, "socket");
	if (connect(s, (struct sockaddr *) &host, sizeof host) < 0)
		err(1, "connect");
	while ((cnt = read(s, buf, BUFSIZ)) > 0)
		write(fileno(stdout), buf, cnt);
	(void) close(s);
}

# endif

void
start_driver()
{
	int	procid;

# ifdef MONITOR
	if (Am_monitor) {
		leavex(1, "No one playing.");
		/* NOTREACHED */
	}
# endif

# ifdef INTERNET
	if (Sock_host != NULL) {
		sleep(3);
		return;
	}
# endif

# ifdef USE_CURSES
	move(HEIGHT, 0);
# else
	mvcur(cur_row, cur_col, HEIGHT, 0);
	cur_row = HEIGHT;
	cur_col = 0;
# endif
	put_str("Starting...");
	refresh();
	procid = fork();
	if (procid == -1) {
		leave(1, "fork failed.");
	}
	if (procid == 0) {
		(void) signal(SIGINT, SIG_IGN);
# ifndef INTERNET
		(void) close(Socket);
# else
		if (use_port == NULL)
# endif
			execl(Driver, "HUNT", (char *) NULL);
# ifdef INTERNET
		else 
			execl(Driver, "HUNT", "-p", use_port, (char *) NULL);
# endif
		/* only get here if exec failed */
		(void) kill(getppid(), SIGUSR1);	/* tell mom */
		_exit(1);
	}
# ifdef USE_CURSES
	move(HEIGHT, 0);
# else
	mvcur(cur_row, cur_col, HEIGHT, 0);
	cur_row = HEIGHT;
	cur_col = 0;
# endif
	put_str("Connecting...");
	refresh();
}

/*
 * bad_con:
 *	We had a bad connection.  For the moment we assume that this
 *	means the game is full.
 */
void
bad_con()
{
	leavex(1, "The game is full.  Sorry.");
	/* NOTREACHED */
}

/*
 * bad_ver:
 *	version number mismatch.
 */
void
bad_ver()
{
	leavex(1, "Version number mismatch. No go.");
	/* NOTREACHED */
}

/*
 * sigterm:
 *	Handle a terminate signal
 */
SIGNAL_TYPE
sigterm(dummy)
	int dummy __attribute__((__unused__));
{
	leavex(0, (char *) NULL);
	/* NOTREACHED */
}


/*
 * sigusr1:
 *	Handle a usr1 signal
 */
SIGNAL_TYPE
sigusr1(dummy)
	int dummy __attribute__((__unused__));
{
	leavex(1, "Unable to start driver.  Try again.");
	/* NOTREACHED */
}

# ifdef INTERNET
/*
 * sigalrm:
 *	Handle an alarm signal
 */
SIGNAL_TYPE
sigalrm(dummy)
	int dummy __attribute__((__unused__));
{
	return;
}
# endif

/*
 * rmnl:
 *	Remove a '\n' at the end of a string if there is one
 */
void
rmnl(s)
	char	*s;
{
	char	*cp;

	cp = strrchr(s, '\n');
	if (cp != NULL)
		*cp = '\0';
}

/*
 * intr:
 *	Handle a interrupt signal
 */
SIGNAL_TYPE
intr(dummy)
	int dummy __attribute__((__unused__));
{
	int	ch;
	int	explained;
	int	y, x;

	(void) signal(SIGINT, SIG_IGN);
# ifdef USE_CURSES
	getyx(stdscr, y, x);
	move(HEIGHT, 0);
# else
	y = cur_row;
	x = cur_col;
	mvcur(cur_row, cur_col, HEIGHT, 0);
	cur_row = HEIGHT;
	cur_col = 0;
# endif
	put_str("Really quit? ");
	clear_eol();
	refresh();
	explained = FALSE;
	for (;;) {
		ch = getchar();
		if (isupper(ch))
			ch = tolower(ch);
		if (ch == 'y') {
			if (Socket != 0) {
				(void) write(Socket, "q", 1);
				(void) close(Socket);
			}
			leavex(0, (char *) NULL);
		}
		else if (ch == 'n') {
			(void) signal(SIGINT, intr);
# ifdef USE_CURSES
			move(y, x);
# else
			mvcur(cur_row, cur_col, y, x);
			cur_row = y;
			cur_col = x;
# endif
			refresh();
			return;
		}
		if (!explained) {
			put_str("(Yes or No) ");
			refresh();
			explained = TRUE;
		}
		beep();
		refresh();
	}
}

void fincurs()
{
	if (in_visual) {
# ifdef USE_CURSES
		move(HEIGHT, 0);
		refresh();
		endwin();
# else /* !USE_CURSES */
		mvcur(cur_row, cur_col, HEIGHT, 0);
		(void) fflush(stdout);	/* flush in case VE changes pages */
# if defined(BSD_RELEASE) && BSD_RELEASE >= 44
		tcsetattr(0, TCSADRAIN, &__orig_termios);
# else
		resetty();
# endif
		_puts(VE);
		_puts(TE);
# endif /* !USE_CURSES */
	}
}

/*
 * leave:
 *	Leave the game somewhat gracefully, restoring all current
 *	tty stats.
 */
void
leave(eval, mesg)
	int	eval;
	char	*mesg;
{
	int serrno = errno;
	fincurs();
	errno = serrno;
	err(eval, mesg ? mesg : "");
}

/*
 * leave:
 *	Leave the game somewhat gracefully, restoring all current
 *	tty stats.
 */
void
leavex(eval, mesg)
	int	eval;
	char	*mesg;
{
	fincurs();
	errx(eval, mesg ? mesg : "");
}

#if !defined(USE_CURSES) && defined(SIGTSTP)
/*
 * tstp:
 *	Handle stop and start signals
 */
SIGNAL_TYPE
tstp(dummy)
	int dummy;
{
# if BSD_RELEASE < 44
	static struct sgttyb	tty;
# endif
	int	y, x;

	y = cur_row;
	x = cur_col;
	mvcur(cur_row, cur_col, HEIGHT, 0);
	cur_row = HEIGHT;
	cur_col = 0;
# if !defined(BSD_RELEASE) || BSD_RELEASE < 44
	tty = _tty;
# endif
	_puts(VE);
	_puts(TE);
	(void) fflush(stdout);
# if defined(BSD_RELEASE) && BSD_RELEASE >= 44
	tcsetattr(0, TCSADRAIN, &__orig_termios);
# else
	resetty();
# endif
	(void) kill(getpid(), SIGSTOP);
	(void) signal(SIGTSTP, tstp);
# if defined(BSD_RELEASE) && BSD_RELEASE >= 44
	tcsetattr(0, TCSADRAIN, &saved_tty);
# else
	_tty = tty;
	ioctl(_tty_ch, TIOCSETP, &_tty);
# endif
	_puts(TI);
	_puts(VS);
	cur_row = y;
	cur_col = x;
	_puts(tgoto(CM, cur_row, cur_col));
	redraw_screen();
	(void) fflush(stdout);
}
#endif /* !defined(USE_CURSES) && defined(SIGTSTP) */

# if defined(BSD_RELEASE) && BSD_RELEASE < 43
char *
strpbrk(s, brk)
	char *s, *brk;
{
	char *p;
	c;

	while (c = *s) {
		for (p = brk; *p; p++)
			if (c == *p)
				return (s);
		s++;
	}
	return (0);
}
# endif

long
env_init(enter_status)
	long	enter_status;
{
	int	i;
	char	*envp, *envname, *s;

	for (i = 0; i < 256; i++)
		map_key[i] = (char) i;

	envname = NULL;
	if ((envp = getenv("HUNT")) != NULL) {
		while ((s = strpbrk(envp, "=,")) != NULL) {
			if (strncmp(envp, "cloak,", s - envp + 1) == 0) {
				enter_status = Q_CLOAK;
				envp = s + 1;
			}
			else if (strncmp(envp, "scan,", s - envp + 1) == 0) {
				enter_status = Q_SCAN;
				envp = s + 1;
			}
			else if (strncmp(envp, "fly,", s - envp + 1) == 0) {
				enter_status = Q_FLY;
				envp = s + 1;
			}
			else if (strncmp(envp, "nobeep,", s - envp + 1) == 0) {
				no_beep = TRUE;
				envp = s + 1;
			}
			else if (strncmp(envp, "name=", s - envp + 1) == 0) {
				envname = s + 1;
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					strncpy(name, envname, NAMELEN);
					break;
				}
				*s = '\0';
				strncpy(name, envname, NAMELEN);
				envp = s + 1;
			}
# ifdef INTERNET
			else if (strncmp(envp, "port=", s - envp + 1) == 0) {
				use_port = s + 1;
				Test_port = atoi(use_port);
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}
			else if (strncmp(envp, "host=", s - envp + 1) == 0) {
				Sock_host = s + 1;
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}
			else if (strncmp(envp, "message=", s - envp + 1) == 0) {
				Send_message = s + 1;
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}
# endif
			else if (strncmp(envp, "team=", s - envp + 1) == 0) {
				team = *(s + 1);
				if (!isdigit((unsigned char)team))
					team = ' ';
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				*s = '\0';
				envp = s + 1;
			}			/* must be last option */
			else if (strncmp(envp, "mapkey=", s - envp + 1) == 0) {
				for (s = s + 1; *s != '\0'; s += 2) {
					map_key[(unsigned int) *s] = *(s + 1);
					if (*(s + 1) == '\0') {
						break;
					}
				}
				*envp = '\0';
				break;
			} else {
				*s = '\0';
				printf("unknown option %s\n", envp);
				if ((s = strchr(envp, ',')) == NULL) {
					*envp = '\0';
					break;
				}
				envp = s + 1;
			}
		}
		if (*envp != '\0') {
			if (envname == NULL)
				strncpy(name, envp, NAMELEN);
			else
				printf("unknown option %s\n", envp);
		}
	}
	return enter_status;
}

void
fill_in_blanks()
{
	int	i;
	char	*cp;

again:
	if (name[0] != '\0') {
		printf("Entering as '%s'", name);
		if (team != ' ')
			printf(" on team %c.\n", team);
		else
			putchar('\n');
	} else {
		printf("Enter your code name: ");
		if (fgets(name, NAMELEN, stdin) == NULL)
			exit(1);
	}
	rmnl(name);
	if (name[0] == '\0') {
		name[0] = '\0';
		printf("You have to have a code name!\n");
		goto again;
	}
	for (cp = name; *cp != '\0'; cp++)
		if (!isprint((unsigned char)*cp)) {
			name[0] = '\0';
			printf("Illegal character in your code name.\n");
			goto again;
		}
	if (team == ' ') {
		printf("Enter your team (0-9 or nothing): ");
		i = getchar();
		if (isdigit(i))
			team = i;
		while (i != '\n' && i != EOF)
			i = getchar();
	}
}

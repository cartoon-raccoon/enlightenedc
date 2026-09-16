// A translation of suckless' sinit (suckless init) to EnlightenedC.
// Original source code can be found at https://git.suckless.org/sinit/file/sinit.c.html.

// Original includes. Once we have a working stdlib with these headers (as .HH),
// we can uncomment these.

// #include <sys/types.h>
// #include <sys/wait.h>

// #include <signal.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>

#define LEN(x)	(sizeof (x) / sizeof *(x))
#define TIMEO	30

// nonsensical defines just so it compiles, for now^tm.
#define SIGUSR1 1
#define SIGCHLD 2
#define SIGALRM 3
#define SIGINT  4

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1

#define WNOHANG 42

#define SigSetT U32 // todo: turn this into a typedef

static U0 SigPoweroff();
static U0 SigReap();
static U0 SigReboot();
static U0 Spawn(I8 *const []);

// Declare prototypes here since we don't have the headers.
I32 GetPid();
U0 ChDir(I8 *);
I32 SigFillSet(SigSetT *);
I32 SigProcMask(I32, SigSetT *, SigSetT *);
I32 SigWait(SigSetT *, I32);

I32 Alarm(I32);

I32 Fork();
I32 WaitPID(I32, I32 *, I32);
I32 SetSID();

I32 ExecVP(I8 *const, I8 *const []);

I32 PError(I8 *const);

I32 Exit(I32);

static class {
    I32 sig;
    Void (*Handler)();
} sigmap[] = {
	{ SIGUSR1, SigPoweroff },
	{ SIGCHLD, SigReap     },
	{ SIGALRM, SigReap     },
	{ SIGINT,  SigReboot   },
};

// This was initially the config file for sinit, but we don't have it here, so
// just define the configs inline.

// #include "config.h"

static SigSetT set;

static I8 *const rcinitcmd[]     = { "/bin/rc.init", nullptr };
static I8 *const rcrebootcmd[]   = { "/bin/rc.shutdown", "reboot", nullptr };
static I8 *const rcpoweroffcmd[] = { "/bin/rc.shutdown", "poweroff", nullptr };

@[main]
I32 Main() {
    I32 sig;
    U64 i;

    if (GetPid() != 1)
    	return 1;

    ChDir("/");
    SigFillSet(&set);
    SigProcMask(SIG_BLOCK, &set, nullptr);
    Spawn(rcinitcmd);
    while (1) {
    	Alarm(TIMEO);
    	SigWait(&set, sig);
    	for (i = 0; i < LEN(sigmap); i++) {
        	if (sigmap[i].sig == sig) {
    			sigmap[i].Handler();
    			break;
    		}
    	}
	}
    /* not reachable */
	return 0;
}

static U0 SigPoweroff() {
	Spawn(rcpoweroffcmd);
}

static U0 SigReap() {
	while (WaitPID(-1, nullptr, WNOHANG) > 0);
	Alarm(TIMEO);
}

static U0 SigReboot() {
	Spawn(rcrebootcmd);
}

static U0 Spawn(I8 *const argv[]) {
	switch (Fork()) {
	case 0:
		SigProcMask(SIG_UNBLOCK, &set, nullptr);
		SetSID();
		ExecVP(argv[0], argv);
		PError("execvp");
		Exit(1);
	case -1:
		PError("fork");
	}
}

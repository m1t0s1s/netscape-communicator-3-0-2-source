/*
** Copyright (c) 1995.  Netscape Communications Corporation.  All rights
** reserved.  This use of this Secure Sockets Layer Reference
** Implementation (the "Software") is governed by the terms of the SSL
** Reference Implementation License Agreement.  Please read the
** accompanying "License" file for a description of the rights granted.
** Any other third party materials you use with this Software may be
** subject to additional license restrictions from the licensors of such
** third party software and/or additional export restrictions.  The SSL
** Implementation License Agreement grants you no rights to any such
** third party material.
*/

/*
** Functions for parsing configuration file 'ssld.conf' for ssld.
**
** This should be included in the Makefile for ssld
**
** Last updated: 12/01/95	By: dkarlton
*/

#include "ssld.h"
#include <ctype.h>
#include <netdb.h>

static char **EmitWord(char **av, int nav, char *start, int len)
{
    char *s = (char*) malloc(len+1);
    memcpy(s, start, len);
    s[len] = 0;

    if (av) {
	av = (char**) realloc(av, (nav + 2) * sizeof(char*));
	av[nav] = s;
	av[nav+1] = 0;
    } else {
	av = (char**) malloc(2 * sizeof(char*));
	av[0] = s;
	av[1] = 0;
    }
    return av;
}

char **BreakLine(int *navp, char *cp)
{
    char **av;
    char c, *s;
    int nav;

    av = 0;
    s = cp;
    nav = 0;
    for (; (c = *cp) != 0; cp++) {
	if (c == '\n') {
	    /* Stop and don't increment cp */
	    break;
	}
	if ((c == ' ') || (c == '\t')) {
	    if (cp - s >= 1) {
		/* Emit word into av */
		av = EmitWord(av, nav, s, cp - s);
		nav++;
	    }
	    s = cp + 1;
	}
    }
    if (cp - s >= 1) {
	/* Emit word into av */
	av = EmitWord(av, nav, s, cp - s);
	nav++;
    }
    *navp = nav;
    return av;
}

void
dumpACL(Service *s)
{
    ACLNode *acl;

    acl = s->acl;
    if ( acl ) {
	DebugPrint("using Access Control List for port %d:", s->port);
    
	while ( acl ) {
	    DebugPrint("\t%s", acl->name);
	    acl = acl->next;
	}
    } else {
	DebugPrint("not using any Access Control List for port %d", s->port);
    }
    return;
}

/*
** Read in ssld configuration file, returning a list of Service objects
** that the daemon will service. Leaks memory, but who cares.
*/
Service *ReadConfFile(char *file)
{
    FILE *fp, *aclfp;
    Service *list, *s;
    Service **next;
    char line[2000], *cp, **av;
    int num, nav, port, client, auth, acl;
    struct servent *sp;
    struct hostent *hp;
    ACLNode *node, *aclhead;
    int linelen;
    
    fp = fopen(file, "r");
    if (!fp) return 0;

    num = 0;
    list = 0;
    next = &list;
    for (;;) {
	num++;
	cp = fgets(line, sizeof(line), fp);
	if (!cp) break;
	if (line[0] == '#') continue;
	if (strcmp(line, "\n") == 0) continue;

	/* Parse line */
	av = BreakLine(&nav, line);
	if (av == 0) continue;
	if (nav < 7) {
	    Notice("%s:%d: syntax error", file, num);
	    continue;
	}

	/* Process port field */
	if (isdigit(av[0][0])) {
	    port = htons(atoi(av[0]));
	} else {
	    sp = getservbyname(av[0], "tcp");
	    if (!sp) {
		Notice("%s:%d: no service named \"%s\"", file, num, av[0]);
		continue;
	    }
	    port = sp->s_port;
	}

	/* Process mode field */
	if (strcasecmp(av[1], "client") == 0) {
	    client = 1;
	    auth = 0;
	} else if (strcasecmp(av[1], "auth-client") == 0) {
	    client = 1;
	    auth = 1;
	} else if (strcasecmp(av[1], "server") == 0) {
	    client = 0;
	    auth = 0;
	} else if (strcasecmp(av[1], "auth-server") == 0) {
	    client = 0;
	    auth = 1;
	} else {
	    Notice("%s:%d: no mode called \"%s\"", file, num, av[1]);
	    continue;
	}

	/* Process acl field */
	/* WARNING - acl reading reuses the line buffer,
	 *   we had better be done with it by now.
	 */
	aclhead = 0;
	if (!client) {
	    if (strcmp(av[2], "-") == 0) {
		acl = 0;
	    } else {
		acl = 1;
		aclfp = fopen(av[2], "r");
		if (!aclfp) return 0;
		
		for (;;) {
		    /* read a name from the file */
		    cp = fgets(line, sizeof(line), aclfp);
		    if (!cp) {
			/* done */
			break;
		    }
		    
		    if ( ( line[0] == '#' ) || ( line[0] == 0 ) ||
			( line[0] == '\n' ) ) {
			/* comment or blank line */
			continue;
		    }

		    linelen = strlen(line);

		    if ( line[linelen-1] == '\n' ) {
			line[linelen-1] = 0;
		    }

		    node = (ACLNode *)calloc(1, sizeof(ACLNode));
		    if ( !node ) {
			return 0;
		    }
		    
		    node->name = (char *)calloc(1, linelen + 1);
		    if ( !node->name ) {
			return 0;
		    }
		    
		    strcpy(node->name, line);
		
		    node->next = aclhead;
		    aclhead = node;
		}
	    }
	}

	DebugPrint("config file %s:%d: ", file, num);

	DebugPrint("\tport=%d client=%d auth=%d key=%s cert=%s action=%s",
	      port, client, auth, av[3], av[4], av[5]);

	/* Process action field */
	s = (Service*) calloc(1, sizeof(Service));
	s->port = port;
	s->client = client;
	s->auth = auth;
	s->listen = -1;
	s->acl = aclhead;
	s->keyname = av[3];
	s->certname = av[4];

	dumpACL(s);
	
	if (strcasecmp(av[5], "forward") == 0) {
	    cp = strchr(av[6], ':');
	    if (!cp) {
		Notice("%s:%d: no port on forward address", file, num);
		continue;
	    }
	    *cp++ = 0;
	    hp = gethostbyname(av[6]);
	    if (!hp) {
		Notice("%s:%d: no host found called \"%s\"", file, num, av[6]);
		continue;
	    }
	    if (isdigit(*cp)) {
		port = htons(atoi(cp));
	    } else {
		sp = getservbyname(cp, "tcp");
		if (!sp) {
		    Notice("%s:%d: no service named \"%s\"", file, num, av[0]);
		    continue;
		}
		port = sp->s_port;
	    }
	    s->action = FORWARDER;
	    s->forward.sin_family = AF_INET;
	    s->forward.sin_port = port;
	    s->forward.sin_addr = *(struct in_addr*) hp->h_addr_list[0];
	} else if (strcasecmp(av[5], "exec") == 0) {
	    if (client) {
		Notice("%s:%d: client mode can't use exec action", file, num);
		continue;
	    }
	    s->action = EXECER;
	    s->file = av[6];
	    s->avp = &av[7];
	}
	*next = s;
	next = &s->next;
    }
    return list;
}


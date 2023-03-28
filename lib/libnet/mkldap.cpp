// 
// mkldap.cpp -- Handles "ldap:" and "ldaps:" URLs for core Navigator, without
//               requiring libmsg. mkldap is therefore intended for  
//               non-address-book searching such as CRLs for security.
//
// Copyright (c) 1996 by Netscape Communications Corp
//

#include "mkutils.h"
#include "mkgeturl.h"
#include "mkldap.h"

#ifdef WIN32
#define LIBNET_LDAP
#endif

#ifdef LIBNET_LDAP
	#include "dirprefs.h"

	#define NEEDPROTOS
	#define LDAP_REFERRALS
	#include "lber.h"
	#include "ldap.h"
	#include "disptmpl.h"
#endif

extern "C"
{
	extern int MK_OUT_OF_MEMORY;
};


//
// NET_LdapConnectionData is the closure object for LDAP connections
// 

class NET_LdapConnectionData
{
public:
	NET_LdapConnectionData ();
	~NET_LdapConnectionData ();

#ifdef LIBNET_LDAP
	LDAP *m_ldap;
#endif
};


NET_LdapConnectionData::NET_LdapConnectionData ()
{
	m_ldap = NULL;
}


NET_LdapConnectionData::~NET_LdapConnectionData ()
{
	XP_ASSERT(m_ldap == NULL); // must have unbound
}


//
// Callbacks from NET_GetURL 
//


int NET_LoadLdap (ActiveEntry *ce)
{
	NET_LdapConnectionData *cd = new NET_LdapConnectionData ();
	if (!cd)
		return MK_OUT_OF_MEMORY;
	ce->con_data = cd;

	return 0;
}


int NET_ProcessLdap (ActiveEntry *ce)
{
	return 0;
}


int NET_InterruptLdap (ActiveEntry * ce)
{
	NET_LdapConnectionData *cd = (NET_LdapConnectionData*) ce->con_data;
	delete cd;

	return 0;
}


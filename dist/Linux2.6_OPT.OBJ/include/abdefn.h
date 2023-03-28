#ifndef _ABDefn_H_
#define	_ABDefn_H_

/*  not final numbers */
const int kMaxFullNameLength 		= 256;	/* common name */
const int kMaxNameLength 			= 64;	/* given, middle, family */
const int kMaxCompanyLength			= 128;	/* company name */
const int kMaxLocalityLength		= 128;	/* city */
const int kMaxRegionLength			= 128;	/* state */
const int kMaxEmailAddressLength 	= 256;
const int kMaxInfo					= 1024;
const int kMaxAddressEntry			= 1000;

typedef uint32 ABID;

typedef struct PersonEntryTag {
	char * pNickName;
	char * pGivenName;
	char * pMiddleName;
	char * pFamilyName;
	char * pCompanyName;
	char * pLocality;
	char * pRegion;
	char * pEmailAddress;
	char * pInfo;
	XP_Bool	HTMLmail;

#ifdef XP_CPLUSPLUS
public:
        void Initialize();
#endif
} PersonEntry; 


typedef struct MailingListEntryTag {
	char * pFullName;
	char * pNickName;
	char * pInfo;

#ifdef XP_CPLUSPLUS

public: 
	void Initialize();
#endif
} MailingListEntry;  

const ABID	ABTypeAll		= 35;
const ABID	ABTypePerson	= 36;
const ABID  ABTypeList		= 37;

const unsigned long	ABTypeEntry			= 0x70634944;	/* ASCII - 'pcID' */
const unsigned long	ABFullName			= 0x636E2020;	/* ASCII - 'cn  ' */
const unsigned long	ABNickname			= 0x6E69636B;	/* ASCII - 'nick' */
const unsigned long	ABEmailAddress		= 0x6D61696C;	/* ASCII - 'mail' */
const unsigned long	ABLocality			= 0x6C6F6320;	/* ASCII - 'loc ' */
const unsigned long	ABCompany			= 0x6F726720;	/* ASCII - 'org ' */

/* Flags for updating the address book */

#define AB_FLAG_ADD			0x0001		/* has been added */
#define AB_FLAG_REMOVE		0x0002		/* has been deleted */
#define AB_FLAG_MODIFY		0x0008		/* has been modified */
#define AB_FLAG_RESORT		0x0010		/* has been resorted */
#define AB_FLAG_OPEN		0x0020		/* Database has been opened */
#define AB_FLAG_CLOSE		0x0080		/* Database has been closed */
#define AB_FLAG_REMOVEFROMML	0x0100	/* mailing list entry has been deleted */

typedef enum
{
 
  /* FILE MENU
	 =========
   */
  AB_NewMessageCmd,				/* Send a new message to the selected entries */
  AB_NewCCMessageCmd,			/* Send a new message and cc the selected entries */
  AB_NewBCCMessageCmd,			/* Send a new message and bcc the selected entries */

  AB_ImportCmd,					/* import a file into the address book */

  AB_SaveCmd,					/* export to a file */

  AB_CloseCmd,					/* close the address book window */

  /* EDIT MENU
	 =========
   */

  AB_UndoCmd,					/* Undoes the last operation. */

  AB_RedoCmd,					/* Redoes the last undone operation. */

  AB_DeleteCmd,					/* Causes the given entries to be
								   deleted. */

  AB_FindCmd,					/* Find the given string in the address
								   book */

  AB_FindAgainCmd,				/* Find the string from the previous find
									again */

  /* VIEW/SORT MENUS
	 ===============
   */

  AB_SortByTypeCmd,				/* Sort alphabetized by type. */
  AB_SortByFullNameCmd,			/* Sort alphabetizedby full name. */
  AB_SortByLocality,			/* Sort by state */
  AB_SortByNickname,			/* Sort by nickname */
  AB_SortByEmailAddress,		/* Sort by email address */
  AB_SortByCompanyName,			/* Sort by email address */
  AB_SortAscending,				/* Sort current column ascending */
  AB_SortDescending,			/* Sort current column descending */

  /* ITEM MENU
	 ============
   */

   AB_AddUserCmd,				/* Add a user to the address book */
   AB_AddMailingListCmd,		/* Add a mailing list to the address book */
   AB_PropertiesCmd				/* Get the properties of an entry */

  /* Tools MENU
	 =======
   */
  
} AB_CommandType;

#endif

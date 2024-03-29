/* @(#)cryptint.h	1.26\t10 Nov 1995 */
/*****************************************************************************
  Definitive Fortezza header file.
  Application Level Interface to Fortezza CI Library.

  Version for CI Library 1.52
    November 8, 1995


  NOTICE: Fortezza Export Policy

    The Fortezza Cryptologic Interface (CI) Library (both source and 
    object) and Fortezza CI Library based applications are defense 
    articles, as defined in the International Traffic In Arms 
    Regulations (ITAR), and are subject to export controls under the 
    ITAR and the Arms Export Control Act. Any export to any country 
    of (a) the Fortezza CI Library, related documentation, and 
    technical data, or (b) your cryptographic application, process, 
    or service that is the direct product of, or contains the 
    Fortezza CI Library must comply with the requirements of the ITAR. 
    If you or your customer intends to engage in such export, contact 
    the United States Department of State, Office of Defense Trade 
    Controls for specific guidance.


 ****************************************************************************/
#ifndef __CRYPTINT_H
#define __CRYPTINT_H

#if __cplusplus__ || __cplusplus
extern "C"
{
#endif /* C++ */

#ifndef PROTO_LIST
#ifdef _K_AND_R_
#define PROTO_LIST(list)  ()
#else
#define PROTO_LIST(list)  list
#endif /*_K_AND_R_ */
#endif /* PROTO_LIST */


#ifndef RETURN_TYPE
#if defined( _WIN32 ) || defined( __WIN32__ )
#define RETURN_TYPE  __declspec(dllimport) int
#elif defined( _WINDOWS ) || defined( _Windows )
#define RETURN_TYPE  extern int _far _pascal
#else
#define RETURN_TYPE  extern int
#endif /* Windows */
#endif /* RETURN_TYPE */

/* MS Visual C++ defines _MSDOS and _WINDOWS    */
/* Borland C/C++ defines __MSDOS__ and _Windows */
#if defined( _WIN32 ) ||  defined ( __WIN32__ )
#define CI_FAR
#elif defined( _WINDOWS ) || defined( _Windows )
#define CI_FAR  _far
#else
#define CI_FAR
#endif /* MS DOS or Windows */


/*****************************************************************************
  Constants
 ****************************************************************************/
#define CI_LIB_VERSION_VAL        0x0152    /* Version 1.52 */

#define CI_CERT_SIZE              2048
#define CI_CERT_FLAGS_SIZE        16
#define CI_CERT_NAME_SIZE         32
#define CI_CHALLENGE_SIZE         20

#define CI_G_SIZE                 128

#define CI_HASHVALUE_SIZE         20

#define CI_IV_SIZE                24

#define CI_KEY_SIZE               12
#define CI_KS_SIZE                10

#define CI_NAME_SIZE              32

#define CI_PASSWORD_SIZE          24
#define CI_PIN_SIZE               12
#define CI_P_SIZE                 128

#define CI_Q_SIZE                 20

#define CI_R_SIZE                 40
#define CI_RANDOM_NO_SIZE         20
#define CI_RANDOM_SEED_SIZE       8
#define CI_RA_SIZE                128
#define CI_RB_SIZE                128
#define CI_REG_FLAGS_SIZE         4

#define CI_S_SIZE                 40
#define CI_SAVE_DATA_SIZE         28
#define CI_SERIAL_NUMBER_SIZE     8
#define CI_SIGNATURE_SIZE         40
#define CI_STATUS_FLAGS_SIZE      4

#define CI_TIME_SIZE              16
#define CI_TIMESTAMP_SIZE         16

#define CI_WRAPPED_X_SIZE         24

#define CI_Y_SIZE                 128

#define CI_X_SIZE                 20


/* Miscellaneous */
#define CI_NULL_FLAG              0 
#define CI_POWER_DOWN_FLAG        2
#define CI_NO_LOG_OFF_FLAG        4
#define CI_INITIATOR_FLAG         0
#define CI_RECIPIENT_FLAG         1

#define CI_BLOCK_LOCK_FLAG        1
#define CI_SSO_LOGGED_ON          0x40
#define CI_USER_LOGGED_ON         0x00
#define CI_FAST_MODE              0x10
#define CI_SLOW_MODE              0x00
#define CI_WORST_CASE_MODE        0x40
#define CI_TYPICAL_CASE_MODE      0x00

/* Card Public Key Algorithms Types */
#define CI_DSA_TYPE               0xA
#define CI_KEA_TYPE               0x5
#define CI_DSA_KEA_TYPE           0xF

/* Fortezza Pin Types */
#define CI_SSO_PIN                0x25
#define CI_USER_PIN               0x2A

/* Crypto Types */
#define CI_ENCRYPT_TYPE           0
#define CI_DECRYPT_TYPE           1
#define CI_HASH_TYPE              2

/* Save and Restore Types */
#define CI_ENCRYPT_INT_TYPE       0x00      /* Internal Encryption */
#define CI_ENCRYPT_EXT_TYPE       0x10      /* External Encryption */
#define CI_DECRYPT_INT_TYPE       0x01      /* Internal Decryption */
#define CI_DECRYPT_EXT_TYPE       0x11      /* External Decryption */
#define CI_HASH_INT_TYPE          0x02      /* Internal Hash */
#define CI_HASH_EXT_TYPE          0x12      /* External Hash */
#define CI_TYPE_EXT_FLAG          0x10      /* Used to differentiate */

/* Configuration types */
#define CI_SET_SPEED_TYPE         1
#define CI_SET_TIMING_TYPE        2

/* Lock States */
#define CI_SOCKET_UNLOCKED        0
#define CI_HOLD_LOCK              1
#define CI_SOCKET_LOCKED          2

/* Fortezza Crypto Types Modes */
#define CI_ECB64_MODE             0
#define CI_CBC64_MODE             1
#define CI_OFB64_MODE             2
#define CI_CFB64_MODE             3
#define CI_CFB32_MODE             4
#define CI_CFB16_MODE             5
#define CI_CFB8_MODE              6

/* Card States */
#define CI_POWER_UP               0
#define CI_UNINITIALIZED          1
#define CI_INITIALIZED            2
#define CI_SSO_INITIALIZED        3
#define CI_LAW_INITIALIZED        4
#define CI_USER_INITIALIZED       5
#define CI_STANDBY                6
#define CI_READY                  7
#define CI_ZEROIZE                8
#define CI_INTERNAL_FAILURE       (-1)

/* Flags for Firmware Update. */
#define CI_NOT_LAST_BLOCK_FLAG    0x00000000UL
#define CI_LAST_BLOCK_FLAG        0x80000000UL
#define CI_DESTRUCTIVE_FLAG       0x000000FFUL
#define CI_NONDESTRUCTIVE_FLAG    0x0000FF00UL


/****************************************************************************
  Fortezza Library Return Codes
 ***************************************************************************/

/* Card Responses */
#define CI_OK                     0
#define CI_FAIL                   1
#define CI_CHECKWORD_FAIL         2
#define CI_INV_TYPE               3
#define CI_INV_MODE               4
#define CI_INV_KEY_INDEX          5
#define CI_INV_CERT_INDEX         6
#define CI_INV_SIZE               7
#define CI_INV_HEADER             8
#define CI_INV_STATE              9
#define CI_EXEC_FAIL              10
#define CI_NO_KEY                 11
#define CI_NO_IV                  12
#define CI_NO_X                   13

#define CI_NO_SAVE                15
#define CI_REG_IN_USE             16
#define CI_INV_COMMAND            17
#define CI_INV_POINTER            18
#define CI_BAD_CLOCK              19
#define CI_NO_DSA_PARMS           20

/* Library Errors */
#define CI_ERROR                  (-1)
#define CI_LIB_NOT_INIT           (-2)
#define CI_CARD_NOT_READY         (-3)
#define CI_CARD_IN_USE            (-4)
#define CI_TIME_OUT               (-5)
#define CI_OUT_OF_MEMORY          (-6)
#define CI_NULL_PTR               (-7)
#define CI_BAD_SIZE               (-8)
#define CI_NO_DECRYPT             (-9)
#define CI_NO_ENCRYPT             (-10)
#define CI_NO_EXECUTE             (-11)
#define CI_BAD_PARAMETER          (-12)
#define CI_OUT_OF_RESOURCES       (-13)

#define CI_NO_CARD                (-20)
#define CI_NO_DRIVER              (-21)
#define CI_NO_CRDSRV              (-22)
#define CI_NO_SCTSRV              (-23)

#define CI_BAD_CARD               (-30)
#define CI_BAD_IOCTL              (-31)
#define CI_BAD_READ               (-32)
#define CI_BAD_SEEK               (-33)
#define CI_BAD_WRITE              (-34)
#define CI_BAD_FLUSH              (-35)
#define CI_BAD_IOSEEK             (-36)
#define CI_BAD_ADDR               (-37)

#define CI_INV_SOCKET_INDEX       (-40)
#define CI_SOCKET_IN_USE          (-41)
#define CI_NO_SOCKET              (-42)
#define CI_SOCKET_NOT_OPENED      (-43)
#define CI_BAD_TUPLES             (-44)
#define CI_NOT_A_CRYPTO_CARD      (-45)

#define CI_INVALID_FUNCTION       (-50)
#define CI_LIB_ALRDY_INIT         (-51)
#define CI_SRVR_ERROR             (-52)


/*****************************************************************************
  Data Structures
 ****************************************************************************/

typedef unsigned char   CI_CERTIFICATE[CI_CERT_SIZE];

typedef unsigned char   CI_CERT_FLAGS[CI_CERT_FLAGS_SIZE];

typedef unsigned char   CI_CERT_STR[CI_CERT_NAME_SIZE+4];

typedef unsigned char   CI_FAR *CI_DATA;

typedef unsigned char   CI_G[CI_G_SIZE];

typedef unsigned char   CI_HASHVALUE[CI_HASHVALUE_SIZE];

typedef unsigned char   CI_IV[CI_IV_SIZE];

typedef unsigned char   CI_KEY[CI_KEY_SIZE];

typedef unsigned char   CI_KS[CI_KS_SIZE];

typedef unsigned char   CI_P[CI_P_SIZE];

typedef unsigned char   CI_PASSWORD[CI_PASSWORD_SIZE + 4];

typedef unsigned char   CI_PIN[CI_PIN_SIZE + 4];

typedef unsigned char   CI_Q[CI_Q_SIZE];

typedef unsigned char   CI_RA[CI_RA_SIZE];

typedef unsigned char   CI_RB[CI_RB_SIZE];

typedef unsigned char   CI_RANDOM[CI_RANDOM_NO_SIZE];

typedef unsigned char   CI_RANDSEED[CI_RANDOM_SEED_SIZE];

typedef unsigned char   CI_REG_FLAGS[CI_REG_FLAGS_SIZE];

typedef unsigned char   CI_SIGNATURE[CI_SIGNATURE_SIZE];

typedef unsigned char   CI_SAVE_DATA[CI_SAVE_DATA_SIZE];

typedef unsigned char   CI_SERIAL_NUMBER[CI_SERIAL_NUMBER_SIZE];

typedef unsigned int    CI_STATE, CI_FAR *CI_STATE_PTR;

typedef unsigned char   CI_TIME[CI_TIME_SIZE];

typedef unsigned char   CI_TIMESTAMP[CI_TIMESTAMP_SIZE];

typedef unsigned char   CI_WRAPPED_X[CI_WRAPPED_X_SIZE];

typedef unsigned char   CI_Y[CI_Y_SIZE];

typedef unsigned char   CI_X[CI_X_SIZE];

typedef struct {
    int     LibraryVersion;                 /* CI Library version */
    int     ManufacturerVersion;            /* Card's hardware version */
    char    ManufacturerName[CI_NAME_SIZE+4]; /* Card manufacturer's name*/
    char    ProductName[CI_NAME_SIZE+4];    /* Card's product name */
    char    ProcessorType[CI_NAME_SIZE+4];  /* Card's processor type */
    unsigned long  UserRAMSize;             /* Amount of User RAM in bytes */
    unsigned long  LargestBlockSize;        /* Largest block of data to pass in */
    int     KeyRegisterCount;               /* Number of key registers */
    int     CertificateCount;               /* Maximum number of personalities (# certs-1) */
    int     CryptoCardFlag;                 /* A flag that if non-zero indicates that there is
                                               a Crypto-Card in the socket. If this value is
                                               zero then there is NOT a Crypto-Card in the
                                               sockets. */
    int     ICDVersion;                     /* The ICD compliance level */
    int     ManufacturerSWVer;              /* The Manufacturer's Software Version */
    int     DriverVersion;                  /* Driver Version */
} CI_CONFIG, CI_FAR *CI_CONFIG_PTR;

typedef struct {
    int          CertificateIndex;          /* Index from 1 to CertificateCount */
    CI_CERT_STR  CertLabel;                 /* The certificate label */
} CI_PERSON, CI_FAR *CI_PERSON_PTR;

typedef struct {
     int    CurrentSocket;                  /* The currently selected socket */
     int    LockState;                      /* Lock status of the current socket */
     CI_SERIAL_NUMBER  SerialNumber;        /* Serial number of the Crypto Engine chip */
     CI_STATE  CurrentState;                /* State of The Card */
     int    DecryptionMode;                 /* Decryption mode of The Card */
     int    EncryptionMode;                 /* Encryption mode of The Card */
     int    CurrentPersonality;             /* Index of the current personality */
     int    KeyRegisterCount;               /* No. of Key Register on The Card */
     CI_REG_FLAGS   KeyRegisterFlags;       /* Bit Masks indicating Key Register use */
     int    CertificateCount;               /* No. of Certificates on The Card */
     CI_CERT_FLAGS  CertificateFlags;       /* Bit Mask indicating certificate use */
     unsigned char  Flags[CI_STATUS_FLAGS_SIZE];  
                                            /* Flag[0] : bit 6 for Condition mode */
                                            /*           bit 4 for Clock mode */
} CI_STATUS, CI_FAR *CI_STATUS_PTR;


/*****************************************************************************
  Function Call Prototypes
 ****************************************************************************/

RETURN_TYPE
CI_ChangePIN PROTO_LIST( (
    int            PINType,
    CI_PIN CI_FAR  pOldPIN,
    CI_PIN CI_FAR  pNewPIN ) );

RETURN_TYPE
CI_CheckPIN PROTO_LIST( (
    int            PINType,
    CI_PIN CI_FAR  pPIN ) );

RETURN_TYPE
CI_Close PROTO_LIST( (
    unsigned int  Flags,
    int           SocketIndex ) );

RETURN_TYPE
CI_Decrypt PROTO_LIST( (
    unsigned int  CipherSize,
    CI_DATA       pCipher,
    CI_DATA       pPlain ) );

RETURN_TYPE
CI_DeleteCertificate PROTO_LIST( (
    int  CertificateIndex ) );

RETURN_TYPE
CI_DeleteKey PROTO_LIST( (
    int  RegisterIndex ) );

RETURN_TYPE
CI_Encrypt PROTO_LIST( (
    unsigned int  PlainSize,
    CI_DATA       pPlain,
    CI_DATA       pCipher ) );

RETURN_TYPE
CI_ExtractX PROTO_LIST( (
    int                  CertificateIndex,
    int                  AlgorithmType,
    CI_PASSWORD CI_FAR   pPassword,
    unsigned int         YSize,
    CI_Y CI_FAR          pY,
    CI_WRAPPED_X CI_FAR  pX,
    CI_RA CI_FAR         pRa,
    unsigned int         PandGSize,
    unsigned int         QSize,
    CI_P CI_FAR          pP,
    CI_Q CI_FAR          pQ,
    CI_G CI_FAR          pG ) );

RETURN_TYPE
CI_FirmwareUpdate PROTO_LIST( (
    unsigned long  Flags,
    long           Cksum,
    unsigned int   CksumLength,
    unsigned int   DataSize,
    CI_DATA        pData ) );

RETURN_TYPE
CI_GenerateIV PROTO_LIST( (
    CI_IV CI_FAR  pIV ) );

RETURN_TYPE
CI_GenerateMEK PROTO_LIST( (
    int  RegisterIndex,
    int  Reserved ) );

RETURN_TYPE
CI_GenerateRa PROTO_LIST( (
    CI_RA CI_FAR  pRa ) );

RETURN_TYPE
CI_GenerateRandom PROTO_LIST( (
    CI_RANDOM CI_FAR  pRandom ) );

RETURN_TYPE
CI_GenerateTEK PROTO_LIST( (
    int           Flags,
    int           RegisterIndex,
    CI_RA CI_FAR  pRa,
    CI_RB CI_FAR  pRb,
    unsigned int  YSize,
    CI_Y CI_FAR   pY ) );

RETURN_TYPE
CI_GenerateX PROTO_LIST( (
    int           CertificateIndex,
    int           AlgorithmType,
    unsigned int  PandGSize,
    unsigned int  QSize,
    CI_P CI_FAR   pP,
    CI_Q CI_FAR   pQ,
    CI_G CI_FAR   pG,
    unsigned int  YSize,
    CI_Y CI_FAR   pY ) );

RETURN_TYPE
CI_GetCertificate PROTO_LIST( (
    int                    CertificateIndex,
    CI_CERTIFICATE CI_FAR  pCertificate ) );

RETURN_TYPE
CI_GetConfiguration PROTO_LIST( (
    CI_CONFIG_PTR  pConfiguration ) );

RETURN_TYPE
CI_GetHash PROTO_LIST( (
    unsigned int         DataSize,
    CI_DATA              pData,
    CI_HASHVALUE CI_FAR  pHashValue ) );

RETURN_TYPE
CI_GetPersonalityList PROTO_LIST( (
    int               EntryCount,
    CI_PERSON CI_FAR  pPersonalityList[] ) );

RETURN_TYPE
CI_GetState PROTO_LIST( (
    CI_STATE_PTR  pState ) );

RETURN_TYPE
CI_GetStatus PROTO_LIST( (
    CI_STATUS_PTR  pStatus ) );

RETURN_TYPE
CI_GetTime PROTO_LIST( (
    CI_TIME CI_FAR  pTime ) );

RETURN_TYPE
CI_Hash PROTO_LIST( (
    unsigned int  DataSize,
    CI_DATA       pData ) );

RETURN_TYPE
CI_Initialize PROTO_LIST( (
    int CI_FAR  *SocketCount ) );

RETURN_TYPE
CI_InitializeHash PROTO_LIST( (
    void ) );

RETURN_TYPE
CI_InstallX PROTO_LIST( (
    int                  CertificateIndex,
    int                  AlgorithmType,
    CI_PASSWORD CI_FAR   pPassword,
    unsigned int         YSize,
    CI_Y CI_FAR          pY,
    CI_WRAPPED_X CI_FAR  pWrappedX,
    CI_RA CI_FAR         pRa,
    unsigned int         PandGSize,
    unsigned int         QSize,
    CI_P CI_FAR          pP,
    CI_Q CI_FAR          pQ,
    CI_G CI_FAR          pG ) );

RETURN_TYPE
CI_LoadCertificate PROTO_LIST( (
    int                    CertificateIndex,
    CI_CERT_STR CI_FAR     pCertLabel,
    CI_CERTIFICATE CI_FAR  pCertificate,
    long                   Reserved ) );

RETURN_TYPE
CI_LoadDSAParameters PROTO_LIST( (
    unsigned int  PandGSize,
    unsigned int  QSize,
    CI_P CI_FAR   pP,
    CI_Q CI_FAR   pQ,
    CI_G CI_FAR   pG ) );

RETURN_TYPE
CI_LoadInitValues PROTO_LIST( (
    CI_RANDSEED CI_FAR  pRandSeed,
    CI_KS CI_FAR        pKs ) );

RETURN_TYPE
CI_LoadIV PROTO_LIST( (
    CI_IV CI_FAR  pIV ) );

RETURN_TYPE
CI_LoadX PROTO_LIST( (
    int           CertificateIndex,
    int           AlgorithmType,
    unsigned int  PandGSize,
    unsigned int  QSize,
    CI_P CI_FAR   pP,
    CI_Q CI_FAR   pQ,
    CI_G CI_FAR   pG,
    CI_X CI_FAR   pX,
    unsigned int  YSize,
    CI_Y CI_FAR   pY ) );

RETURN_TYPE
CI_Lock PROTO_LIST( (
    int  Flags ) );

RETURN_TYPE
CI_Open PROTO_LIST( (
    unsigned int  Flags,
    int           SocketIndex ) );

RETURN_TYPE
CI_RelayX PROTO_LIST( (
    CI_PASSWORD CI_FAR   pOldPassword,
    unsigned int         OldYSize,
    CI_Y CI_FAR          pOldY,
    CI_RA CI_FAR         pOldRa,
    CI_WRAPPED_X CI_FAR  pOldWrappedX,
    CI_PASSWORD CI_FAR   pNewPassword,
    unsigned int         NewYSize,
    CI_Y CI_FAR          pNewY,
    CI_RA CI_FAR         pNewRa,
    CI_WRAPPED_X CI_FAR  pNewWrappedX ) );

RETURN_TYPE
CI_Reset PROTO_LIST( (
    void ) );

RETURN_TYPE
CI_Restore PROTO_LIST( (
    int                  CryptoType,
    CI_SAVE_DATA CI_FAR  pData ) );

RETURN_TYPE
CI_Save PROTO_LIST( (
    int                  CryptoType,
    CI_SAVE_DATA CI_FAR  pData ) );

RETURN_TYPE
CI_Select PROTO_LIST( (
    int  SocketIndex ) );

RETURN_TYPE
CI_SetConfiguration PROTO_LIST( (
    int           Type,
    unsigned int  DataSize,
    CI_DATA       pData ) );

RETURN_TYPE
CI_SetKey PROTO_LIST( (
    int  RegisterIndex ) );

RETURN_TYPE
CI_SetMode PROTO_LIST( (
    int  CryptoType,
    int  CryptoMode ) );

RETURN_TYPE
CI_SetPersonality PROTO_LIST( (
    int  CertificateIndex ) );

RETURN_TYPE
CI_SetTime PROTO_LIST( (
    CI_TIME CI_FAR  pTime ) );

RETURN_TYPE
CI_Sign PROTO_LIST( (
    CI_HASHVALUE CI_FAR  pHashValue,
    CI_SIGNATURE CI_FAR  pSignature ) );

RETURN_TYPE
CI_Terminate PROTO_LIST( (
    void ) );

RETURN_TYPE
CI_TimeStamp PROTO_LIST( (
    CI_HASHVALUE CI_FAR  pHashValue,
    CI_SIGNATURE CI_FAR  pSignature,
    CI_TIMESTAMP CI_FAR  pTimeStamp ) );

RETURN_TYPE
CI_Unlock PROTO_LIST( (
    void ) );

RETURN_TYPE
CI_UnwrapKey PROTO_LIST( (
    int            UnwrapIndex,
    int            KeyIndex,
    CI_KEY CI_FAR  pKey ) );

RETURN_TYPE
CI_VerifySignature PROTO_LIST( (
    CI_HASHVALUE CI_FAR  pHashValue,
    unsigned int         YSize,
    CI_Y CI_FAR          pY,
    CI_SIGNATURE CI_FAR  pSignature ) );

RETURN_TYPE
CI_VerifyTimeStamp PROTO_LIST( (
    CI_HASHVALUE CI_FAR  pHashValue,
    CI_SIGNATURE CI_FAR  pSignature,
    CI_TIMESTAMP CI_FAR  pTimeStamp ) );

RETURN_TYPE
CI_WrapKey PROTO_LIST( (
    int            WrapIndex,
    int            KeyIndex,
    CI_KEY CI_FAR  pKey ) );

RETURN_TYPE
CI_Zeroize PROTO_LIST( (
    void ) );

#if __cplusplus__ || __cplusplus
}
#endif /* C++ */

#endif /* CRYPTINT_H */

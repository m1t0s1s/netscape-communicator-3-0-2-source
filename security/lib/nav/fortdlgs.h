/*
 * Fortezza Security stuff specific to the client (mozilla).
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: fortdlgs.h,v 1.4 1996/10/14 11:35:47 jsw Exp $
 */

SECStatus FortezzaClientAuthData(SSLSocket *ss, CERTCertificate **pRetCert,
			 SECKEYPrivateKey **pRetKey);
SECStatus SECMOZ_GetPin(void *arg,void *psock);
SECStatus SECMOZ_CertificateSelect(void *arg, int fd, char **prompt, 
					int certCount, int *cert);
SECStatus SECMOZ_CardSelect(void *arg, int fd, void *, int cardmask, 
								int *psockId);
SECStatus SECMOZ_FortezzaAlert(void *arg,PRBool onOpen,char *string,
						    int value1,void *value2);

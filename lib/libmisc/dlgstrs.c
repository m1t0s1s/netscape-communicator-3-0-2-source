/* this file is not built any more.  ifdef'd out to make sure that we don't
 * break the build before people remove it from their makefiles
 */

#ifdef NOTDEF
#include "net.h"
#include "sec.h"
#include "htmldlgs.h"

static char *dialogHeaderStrings[] = {
    "<head><title>",
    0,
    "</title></head>"
    "<body bgcolor=\"#bbbbbb\">"
    "<form action=internal-dialog-handler method=post>"
    "<input type=\"hidden\" name=\"handle\" value=\"",
    0,
    "\">"
    "<table border=0 cellspacing=0 cellpadding=0 width=\"100%\" height=\"90%\">"
    "<tr width=\"100%\" height=\"75%\" valign=top>"
    "<td>"
    "<font size=2>",
    NULL
};

static char *dialogMiddleStrings[] = {
    "</font></td></tr><tr width=\"100%\" height=\"15%\" valign=\"bottom\">"
    "<td>",
    NULL
};

static char *dialogFooterStrings[] = {
    "</td></tr></table></form></body>",
    NULL
};

static char *dialogCancelButtonStrings[] = {
    "<center><input type=\"submit\" name=\"button\" "
    "value=\"Cancel\"></center>",
    NULL
};

static char *dialogOkButtonStrings[] = {
    "<center><input type=\"submit\" name=\"button\" "
    "value=\"OK\"></center>",
    NULL
};

static char *dialogContinueButtonStrings[] = {
    "<center><input type=\"submit\" name=\"button\" "
    "value=\"Continue\"></center>",
    NULL
};

static char *dialogCancelOKButtonStrings[] = {
    "<center><input type=\"submit\" name=\"button\" value=\"OK\">"
    "<input type=\"submit\" name=\"button\" value=\"Cancel\"></center>",
    NULL
};

static char *dialogCancelContinueButtonStrings[] = {
    "<center><input type=\"submit\" name=\"button\" value=\"Cancel\">"
    "<input type=\"submit\" name=\"button\" value=\"Continue\"></center>",
    NULL
};

static char *nullStrings[] = {
    NULL
};

static char *panelHeaderStrings[] = {
    "<head><title>",
    0,
    "</title></head>"
    "<body bgcolor=\"#bbbbbb\">"
    "<form action=internal-panel-handler method=post>"
    "<input type=\"hidden\" name=\"handle\" value=\"",
    0,
    "\">"
    "<table border=0 cellspacing=0 cellpadding = 0 width=\"100%\" height=\"90%\">"
    "<tr width=\"100%\" height=\"75%\" valign=top>"
    "<td colspan=3>"
    "<font size=2>",
    NULL
};

static char *panelMiddleStrings[] = {
    "</font></td></tr><tr width=\"100%\" height=\"15%\" valign=\"bottom\">"
    "<td width=\"33%\"></td>",
    NULL
};

static char *panelFooterStrings[] = {
    "</tr></table></form></body>",
    NULL
};

static char *panelFirstButtonStrings[] = {
    "<td width=\"34%\" align=center>"
    "<input type=\"submit\" name=\"button\" value=\"Cancel\">"
    "</td>"
    "<td width=\"33%\" align=right>"
    "<input type=\"submit\" name=\"button\" value=\"Next&gt\">"
    "</td>",
    NULL
};

static char *panelMiddleButtonStrings[] = {
    "<td width=\"34%\" align=center>"
    "<input type=\"submit\" name=\"button\" value=\"Cancel\">"
    "</td>"
    "<td width=\"33%\" align=right>"
    "<input type=\"submit\" name=\"button\" value=\"&ltBack\">"
    "<input type=\"submit\" name=\"button\" value=\"Next&gt\">"
    "</td>",
    NULL
};

static char *panelLastButtonStrings[] = {
    "<td width=\"34%\" align=center>"
    "<input type=\"submit\" name=\"button\" value=\"Cancel\">"
    "</td>"
    "<td width=\"33%\" align=right>"
    "<input type=\"submit\" name=\"button\" value=\"&ltBack\">"
    "<input type=\"submit\" name=\"button\" value=\"Finished\">"
    "</td>",
    NULL
};

static char *panelOnlyButtonStrings[] = {
    "<td width=\"34%\" align=center>"
    "<input type=\"submit\" name=\"button\" value=\"Cancel\">"
    "</td>"
    "<td width=\"33%\" align=right>"
    "<input type=\"submit\" name=\"button\" value=\"Finished\">"
    "</td>",
    NULL
};

static char *plainCertDialogStrings[] = {
    NULL
};

static char *certConfirmStrings[] = {
    "<title>",
    0,
    "</title><b>",
    0,
    "</b><hr>",
    0,
    "<hr>",
    0
};

#ifdef NOTDEF
static char *certPageStrings[] = {
    "<title>Certificate: \n",
    0,
    "</title>",
    0,
    0,
    0,
};
#endif

static char *certPageStrings[] = {
    0,
    0,
    0,
};

static char *sslServerCert1Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "<b><font size=4>",
    0,
    " is a site that uses encryption to protect transmitted information. "
    "However, Netscape does not recognize the "
    "authority who signed its Certificate."
    "</font></b><p>"
    "Although Netscape does not recognize the signer of this Certificate, "
    "you may decide to accept it anyway so that you can connect to and "
    "exchange information with this site."
    "<p>"
    "This assistant will help you decide whether or not you wish to accept "
    "this Certificate and to what extent.",
    0
};

static char *sslServerCert2Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "Here is the Certificate that is being presented:<hr>"
    "<table><tr>"
    "<td valign=top><font size=2>Certificate for: "
    "<br>Signed by: "
    "<br>Encryption: </font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "<br>",
    0,
    " Grade (",
    0,
    " with ",
    0,
    "-bit secret key)</font></td>"
    "<td valign=bottom>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</td></tr></table>"
    "<hr>The signer of the Certificate promises you that the holder of this "
    "Certificate is "
    "who they say they are.  The encryption level is an indication of how "
    "difficult it would be for someone to eavesdrop on any information "
    "exchanged between you and this web site.",
    0
};

static char *sslServerCert3Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "Are you willing to accept this certificate for the purposes of receiving "
    "encrypted information from this web site?"
    "<p>"
    "This means that you will be able to browse through the site and receive "
    "documents from it and that all of these documents are protected from "
    "observation by a third party by encryption."
    "<p>"
    "<input type=radio name=accept value=session",
    0,
    ">"
    "Accept this ID for this session<br>"
    "<input type=radio name=accept value=cancel",
    0,
    ">"
    "Do not accept this ID and do not connect<br>"
    "<input type=radio name=accept value=forever",
    0,
    ">"
    "Accept this ID forever (until it expires)<br>",
    0
};

static char *sslServerCert4Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "By accepting this ID you are ensuring that all information you exchange "
    "with this site will be encrypted.  However, encryption will not protect "
    "you from fraud."
    "<p>"
    "To protect yourself from fraud, do not send information (especially "
    "personal information, credit card numbers, or passwords) to this site "
    "if you have any doubt about the site's integrity."
    "<p>"
    "For your own protection, Netscape can remind you of this at the "
    "appropriate time."
    "<p>"
    "<center><input type=checkbox name=postwarn value=yes ",
    0,
    ">"
    "Warn me before I send information to this site</center><p>",
    0
};

static char *sslServerCert5aStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "<b>You have finished examining the certificate presented by:<br>",
    0,
    "</b><p>You have decided to refuse this ID. "
    "If, in the future, you change your mind about this decision, "
    "just visit this site again and this assistant will reappear."
    "<p>"
    "Press Finished to return to the document you were viewing "
    "before you attempted to connect to this site.",
    0
};

/*
 * XXX this is a near-duplicate of the next one; a way to "common"
 * parts of these without creating an l10n nightmare would be nice.
 */
static char *sslServerCert5bStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "<b>You have finished examining the certificate presented by:<br>",
    0,
    "</b><p>You have decided to accept this ID and have "
    "asked that "
    "Netscape Navigator warn you before you send information to this site."
    "<p>"
    "If, in the future, you change your mind about this decision, "
    "open Security Preferences and edit the Site Certificates."
    "<p>"
    "Press Finished to begin receiving documents.",
    0
};

/*
 * XXX this is a near-duplicate of the previous one; a way to "common"
 * parts of these without creating an l10n nightmare would be nice.
 */
static char *sslServerCert5cStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "<b>You have finished examining the certificate presented by:<br>",
    0,
    "</b><p>You have decided to accept this ID and have "
    "decided not to have "
    "Netscape Navigator warn you before you send information to this site."
    "<p>"
    "If, in the future, you change your mind about this decision, "
    "open Security Preferences and edit the Site Certificates."
    "<p>"
    "Press Finished to begin receiving documents.",
    0
};

static char *sslCertDomainDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "The certificate that the site '",
    0,
    "' has presented does not contain the "
    "correct site name. It is possible, though unlikely, that someone may be "
    "trying to intercept your communication "
    "with this site.  If you suspect the certificate shown below does not "
    "belong to the site you are connecting with, please cancel the "
    "connection and notify the site administrator. "
    "<p>"
    "Here is the Certificate that is being presented:<hr>"
    "<table><tr>"
    "<td valign=top><font size=2>Certificate for: "
    "<br>Signed by: "
    "<br>Encryption: </font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "<br>",
    0,
    " Grade (",
    0,
    " with ",
    0,
    "-bit secret key)</font></td>"
    "<td valign=bottom>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</td></tr></table>"
    "<hr>",
    0
};

static char *pwEnterNewStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "Please enter your new password.  The safest passwords are a combination "
    "of letters and numbers, are at least 8 characters long, and contain no "
    "words from a dictionary.<p>"
    "Password: <input type=password name=password1><p>"
    "Type in your password, again, for verification:<p>"
    "Retype Password: <input type=password name=password2><p>"
    "<b>Do not forget your password!  Your password cannot be recovered. "
    "If you forget it, you will have to obtain new Certificates.</b> "
};

static char *pwRetryNewStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "You did not enter your password correctly.  Please try again:<p>"
    "Password: <input type=password name=password1><p>"
    "Type in your password, again, for verification:<p>"
    "Retype Password: <input type=password name=password2><p>"
    "<b>Do not forget your password!  Your password cannot be recovered. "
    "If you forget it, you will have to obtain new Certificates.</b> "
};

static char *pwSetupIntroStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "Netscape Navigator will use a password to encrypt your Certificates' "
    "private keys on disk.  This makes it difficult for others to steal your "
    "Certificates and/or use them to pose as you to other users or sites.<p> "
    "If you are in an environment where other people have access to your "
    "computer (either physically or over the network) you should have a "
    "password.<p> "
    "However, if you are in an environment where no other people have access "
    "to your computer, then you may choose not to have a password.<p>"
    "<input type=radio name=usepw value=true ",
    0,
    "> "
    "Other people have access to this computer; I want a password "
    "(recommended)<br>"
    "<input type=radio name=usepw value=false ",
    0,
    "> "
    "This computer is completely isolated; I do not want a password"
};

static char *pwSetupPrefsStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "Netscape Navigator will need to know your password every time you use "
    "a Certificate (for example, when you connect to a site that requests "
    "your Certificate).<p>"
    "<input type=radio name=askpw value=once ",
    0,
    "> "
    "Ask for password once per session<br>"
    "<input type=radio name=askpw value=every ",
    0,
    "> "
    "Ask for password every time it's needed<br>"
    "<input type=radio name=askpw value=inactive ",
    0,
    "> "
    "Ask for password after "
    "<input type=int size=4 name=interval value=",
    0,
    "> minutes of inactivity<p>"
    "You can change these settings, later, in Security Preferences.<p>"
    "Press Finished to return to Netscape Navigator."
};

static char *pwSetupBlankNewStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "You entered a blank password.  If you do not wish to have a "
    "password, then press Back and select \"I do not want a password\". "
    "Please try again:<p>"
    "Password: <input type=password name=password1><p>"
    "Type in your password, again, for verification:<p>"
    "Retype Password: <input type=password name=password2><p>"
    "<b>Do not forget your password!  Your password cannot be recovered. "
    "If you forget it, you will have to obtain new Certificates.</b> "
};

static char *pwSetupRefusedStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "You have elected to operate without a password.<p>"
    "If you decide that you would like to have a password to protect "
    "your private keys and Certificates, you can set up a password in "
    "Security Preferences.<p>"
    "Press Finished to return to Netscape Navigator."
};

static char *pwChangeEnterOldStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "Please enter your old password:<p> "
    "Password: <input type=password name=password>"
};

static char *pwChangeRetryOldStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "You did not correctly enter your current password.  Please try again:<p>"
    "Password: <input type=password name=password>"
};

static char *pwChangeChoiceStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "You may:<p>"
    "<input type=radio name=usepw value=true ",
    0,
    "> Change the password<br>"
    "<input type=radio name=usepw value=false ",
    0,
    "> Have no password"
};

static char *pwChangeBlankNewStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "You entered a blank password.  If you do not wish to have a "
    "password, then press Back and select \"Have no password\". "
    "Please try again:<p>"
    "Password: <input type=password name=password1><p>"
    "Type in your password, again, for verification:<p>"
    "Retype Password: <input type=password name=password2><p>"
    "<b>Do not forget your password!  Your password cannot be recovered. "
    "If you forget it, you will have to obtain new Certificates.</b> "
};

static char *pwChangeDisabledStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "You will no longer be asked to type in a password."
};

static char *pwFinishFailureStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "Your attempt to change, set, or disable your password failed.<p>"
    "This may be because your key database is inaccessible "
    "(which can happen if you were already running a Navigator when "
    "you started this one), or because of some other error.<p>"
    "It may indicate that your key database file has been corrupted, "
    "in which case you should try to get it off of a backup, if possible. "
    "As a last resort, you may need to delete your key database, "
    "after which you will have to obtain new personal Certificates."
};

static char *notImplementedStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p> "
    "This function is not implemented:<br>",
    0,
    "<br>Certificate name:<br>",
    0,
};

static char *certViewStrings[] = {
    0,
    0,
};

static char *deleteUserCertStrings[] = {
    "Are you sure that you want to delete this Personal Certificate?<p>",
    0
};

static char *deleteSiteCertStrings[] = {
    "Are you sure that you want to delete this Site Certificate?<p>",
    0
};

static char *deleteCACertStrings[] = {
    "Are you sure that you want to delete this Certificate Authority?<p>",
    0
};

static char *editSiteCertStrings[] = {
    0, /* certificate string */
    "<hr>This Certificate belongs to an SSL server site.<br>"
    "<input type=radio name=allow value=yes ",
    0, /* checked */
    ">Allow connections to this site<br>"
    "<input type=radio name=allow value=no ",
    0, /* checked */
    ">Do not allow connections to this site<hr>"
    "<input type=checkbox name=postwarn value=yes ",
    0, /* checked */
    ">Warn before sending data to this site"
};

static char *editCACertStrings[] = {
    0, /* certificate string */
    "<hr>This Certificate belongs to a Certifying Authority<br>"
    "<input type=radio name=allow value=yes ",
    0, /* checked */
    ">Allow connections to sites certified by this authority<br>"
    "<input type=radio name=allow value=no ",
    0, /* checked */
    ">Do not allow connections to sites certified by this authority<hr>"
    "<input type=checkbox name=postwarn value=yes ",
    0, /* checked */
    ">Warn before sending data to sites certified by this authority"
};

static char *sslCertPostWarnDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "<b>Warning: You are about to send encrypted information to the site ",
    0,
    ".</b><p>"
    "It is safer not to send information (particularly personal "
    "information, credit card numbers, or passwords) to this site "
    "if you are in doubt about its Certificate or integrity.<br>"
    "Here is the Certificate for this site:<hr>"
    "<table><tr>"
    "<td valign=top><font size=2>Certificate for: "
    "<br>Signed by: "
    "<br>Encryption: </font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "<br>",
    0,
    " Grade (",
    0,
    " with ",
    0,
    "-bit secret key)</font></td>"
    "<td valign=bottom><font size=2>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</font></td></tr></table>"
    "<hr>"
    "<input type=radio name=action value=sendandwarn checked>"
    "Send this information and warn again next time<br>"
    "<input type=radio name=action value=send>"
    "Send this information and do not warn again<br>"
    "<input type=radio name=action value=dontsend>"
    "Do not send information<br>",
    0
};

static char *caCertDownload1Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "You are about to go through the process of accepting a Certificate "
    "Authority. This has serious implications on the security of future "
    "encryptions using Netscape. This assistant will help you decide whether "
    "or not you wish to accept this Certificate Authority."
};

static char *caCertDownload2Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "A Certificate Authority certifies the identity of sites on the "
    "internet. By accepting this Certificate Authority, you will allow "
    "Netscape Navigator to connect to and receive information from "
    "any site that this authority certifies without prompting or warning you. "
    "<p>"
    "If you choose to refuse this Certificate Authority, you will be "
    "prompted before you connect to or receive information from any "
    "site that this authority certifies. "
};

static char *caCertDownload3Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "Here is the certificate for this Certificate Authority. Examine it "
    "carefully. The Certificate Fingerprint can be used to verify that "
    "this Authority is who they say they are. To do this, compare the "
    "Fingerprint against the Fingerprint published by this authority in "
    "other places.<hr> "
    "<table><tr>"
    "<td valign=top><font size=2>Certificate for: "
    "<br>Signed by: </font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "</font></td>"
    "<td valign=bottom><font size=2>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</font></td></tr></table>"
    "<hr>"
};

static char *caCertDownload4Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "Are you willing to accept this Certificate Authority for the "
    "purposes of certifying other internet sites?<p>"
    "This would mean that if you connect to and receive information from "
    "a site that this Authority certifies, you will not be warned.<p>"
    "<input type=radio name=accept value=yes ",
    0, /* checked */
    "> "
    "Accept this Certificate Authority<br>"
    "<input type=radio name=accept value=no ",
    0, /* checked */
    "> "
    "Do not accept this Certificate Authority<br>"
};

static char *caCertDownload5Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "By accepting this Certificate Authority, you have told Netscape "
    "Navigator to connect to and receive information from any site "
    "that it certifies without warning you or prompting you.<p>"
    "Netscape Navigator can, however, warn you before you send "
    "information to such a site.<p>"
    "<input type=checkbox name=postwarn value=yes ",
    0, /* checked */
    ">"
    "Warn me before sending information to sites certified by this "
    "Certificate Authority"
};

static char *caCertDownload6Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "You have accepted this Certificate Authority.  You must now select "
    "a nickname that will be used to identify this Certificate Authority, "
    "for example <b>Mozilla's Certificate Shack</b>.<p>"
    "Nickname: <font size=4><input type=text name=nickname></font>"
};

static char *caCertDownload7Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "By rejecting this Certificate Authority, you have told Netscape "
    "Navigator not to connect to and receive information from any site "
    "that it certifies without prompting you."
};

static char *emptyStrings[] = {
    0
};

static char *sslWhichCertClientAuthDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "The site '",
    0,
    "' has requested client authentication."
    "<p>"
    "Here is the Certificate that is being presented:<hr>"
    "<table><tr>"
    "<td valign=top><font size=2>Certificate for: "
    "<br>Signed by: "
    "<br>Encryption: </font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "<br>",
    0,
    " Grade (",
    0,
    " with ",
    0,
    "-bit secret key)</font></td>"
    "<td valign=bottom>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</td></tr></table>"
    "<hr>"
    "Select A Certificate:<select name=cert>",
    0,
    "</select>",
    0
};

static char *sslNoCertClientAuthDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "The site '",
    0,
    "' has requested client authentication, but you do not have "
    "a Personal Certificate to authenticate yourself.  The site may "
    "choose not to give you access without one.",
    0
};

static char *browserSecInfoEncryptedStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "<B>All of the files that you have requested were encrypted.</B><p> "
    "This means that the files that make up the document are sent "
    "to you encrypted for privacy while in transit.<p> "
    "For more details on the encryption of this document, open "
    "Document Information.<p> "
    "<center>"
    "<input type=\"submit\" name=\"button\" value=\"OK\">"
    "<input type=\"submit\" name=\"button\" value=\"Show Document Info\">"
    "</center>",
    0
};

static char *browserSecInfoMixedStrings[] = {
    "<img src=about:security?banner-mixed><br clear=all><p>"
    "<B>Some of the files that you have requested were encrypted.</B><p> "
    "Some of these files are sent to you encrypted for privacy "
    "while in transit. Others are not encrypted and can be "
    "observed by a third party while in transit.<p>"
    "To find out exactly which files were encrypted and which were "
    "not, open Document Information.<p> "
    "<center>"
    "<input type=\"submit\" name=\"button\" value=\"OK\">"
    "<input type=\"submit\" name=\"button\" value=\"Show Document Info\">"
    "</center>",
    0
};

static char *browserSecInfoClearStrings[] = {
    "<img src=about:security?banner-insecure><br clear=all><p>"
    "<B>None of the files that you have requested are encrypted.</B><p>"
    "Unencrypted files can be observed by a third party while "
    "in transit.<p> "
    "<center><input type=\"submit\" name=\"button\" "
    "value=\"OK\"></center>",
    0
};

static char *submitPaymentIntroStrings[] = {
    "<img src=about:security?banner-payment><br clear=all><p>"
    "In order to complete your purchase, you will need to "
    "provide credit card information.<p>"
    "The information you provide will be encrypted separately "
    "from your order so that it can only be decrypted by <em>",
    0,
    "</em>.<hr><table><tr><td><font size=2>"
    "Press Show Certificate if you want more information about <em>",
    0,
    "</em>.</font></td><td>"
    "<input type=\"submit\" name=\"button\" value=\"Show Certificate\">"
    "</td></tr></table><hr>"
    "Press Cancel to interrupt your purchase, Next to continue.",
    0
};

static char *submitPaymentOrderStrings[] = {
    "<img src=about:security?banner-payment><br clear=all><p>"
    "Your purchase order, as presented by the Merchant:"
    "<p><font size=4><pre>",
    0,
    "</pre></font>",
    0
};

static char *submitPaymentReviewStrings[] = {
    "<img src=about:security?banner-payment><br clear=all><p>",
    "You have elected to use your <em>",
    0,
    "</em> card.  If you wish to change this, press Cancel and go "
    "back to your order form to make the change there.<p>",
    "The total amount to be billed to your account is: ",
    0,
    "<hr><table><tr><td><font size=2>"
    "Press Show Order if you want to review your order."
    "</font></td><td>"
    "<input type=\"submit\" name=\"button\" value=\"Show Order\">"
    "</td></tr></table><hr>"
    "If this is OK, press Next or press Cancel to interrupt "
    "your purchase.",
    0
};

static char *submitPaymentPanStrings[] = {
    "<img src=about:security?banner-payment><br clear=all><p>"
    "Enter the information for your account:<p>"
    "Account number: "
    "<font size=4><input type=text name=pan size=20 maxlength=30 ",
    0,						/* value=number */
    "></font><p>"
    "Month of expiration: <select name=expmonth>",
    0,						/* <option ... */
    "</select> Year of expiration: <select name=expyear>",
    0,						/* <option ... */
    "</select><p>Press Next to continue with your purchase.",
    0
};

static char *submitPaymentAddrStrings[] = {
    "<img src=about:security?banner-payment><br clear=all><p>"
    "In order to verify your identity, some financial institutions "
    "require that you provide your Billing Address, exactly as "
    "it is used for your account.<p>"
    "Street address: ",
    "<font size=4><input type=text name=street size=30 maxlength=30></font>",
    "<p>City: ",
    "<font size=4><input type=text name=city size=15 maxlength=15></font>",
    " State: ",
    "<font size=4><input type=text name=state size=2 maxlength=2></font>",
    " Zip code: ",
    "<font size=4><input type=text name=zip size=9 maxlength=9></font><p>",
    0
};

static char *submitPaymentFinishStrings[] = {
    "<img src=about:security?banner-payment><br clear=all><p>"
    "You have completed the information necessary to make your purchase. "
    "The amount of ",
    0,
    " will be charged to your account.<p>"
    "If you need to change any information, press Back.<p>"
    "Press Finished to submit the charge to your account and complete "
    "your purchase.",
    0
};

static char *sslConfigCiphersStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "<h3>Select ciphers to enable for SSL v",
    0,
    "</h3><ul>",
    0,
    "</ul>",
    0
};

static char *userCertDownload1Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "You are downloading a new personal certificate that you have previously "
    "requested from ",
     0,
    ".  This certificate may be used, along with "
    "the corresponding private key that was generated for you at the time "
    "you requested your certificate, to identify yourself to sites on the "
    "internet.  Using certificates and private keys to identify yourself "
    "to internet sites is much more secure than the traditional username "
    "and password.",
    0
};

static char *userCertDownload2Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "Here is the information from your new certificate.  Press the More Info "
    "button to see all of the details of the certificate. <hr>"
    "<table><tr>"
    "<td valign=top><font size=2>Certificate for: "
    "<br>Signed by: "
    "</font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "</font></td>"
    "<td valign=bottom>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</td></tr></table><hr>"
};

static char *userCertDownload3Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "You will now enter a name that Netscape will use to refer to this "
    "certificate.  You may use the name provided or enter one that is "
    "more meaningful to you.<p>"
    "Certificate Name: <font size=2><input type=text size=60 name=nickname value=\"",
    0,
    "\"></font><p>",
    0
};

static char *userCertDownload4Strings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "Your new certificate has successfully been installed.  You may now "
    "use it to identify yourself to internet sites that require certificates.",
    0
};

static char *keyGenDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>"
    "Netscape is about to generate a private key for you.  This private key "
    "will be used along with the certificate you are now requesting to "
    "identify yourself to internet sites.  Your private key never leaves your "
    "computer, and is protected by your Netscape password.  It is important "
    "that you never give anyone your password, because that will allow them "
    "to use your private key to impersonate you on the internet. "
    "<p>"
    "When you press the OK button below, Netscape will generate your "
    "private key for you.  This is a complex mathematical operation, and "
    "may take up to several minutes for your computer to complete.  If you "
    "interrupt Netscape during this process it will not create your key, "
    "and you will have to re-apply for your certificate.",
    0
};

static char *certExpiredDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>",
    0,
    " is a site that uses encryption to protect transmitted "
    "information.  However the digital Certificate that identifies "
    "this site has expired.  This may be because the certificate has "
    "actually expired, or because the date on your computer is wrong.<p>"
    "The certificate expires on ",
    0,
    ".<p>"
    "Your computer's date is set to ",
    0,
    ".  If this date is incorrect, then you should reset the date "
    "on your computer.<p>"
    "You may continue or cancel this connection.",
    0
};

static char *certNotYetGoodDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>",
    0,
    " is a site that uses encryption to protect transmitted "
    "information.  However the digital Certificate that identifies "
    "this site is not yet valid.  This may be because the certificate was "
    "installed too soon by the site administrator, or because the date on "
    "your computer is wrong.<p>"
    "The certificate is valid beginning ",
    0,
    ".<p>"
    "Your computer's date is set to ",
    0,
    ".  If this date is incorrect, then you should reset the date "
    "on your computer.<p>"
    "You may continue or cancel this connection.",
    0
};

static char *caExpiredDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>",
    0,
    " is a site that uses encryption to protect transmitted "
    "information.  However one of the Certificate Authorities that identifies "
    "this site has expired.  This may be because a certificate has "
    "actually expired, or because the date on your computer is wrong. "
    "Press the More Info button to see details of the expired certificate.<hr>"
    "<table cellspacing=0 cellpadding=0><tr>"
    "<td valign=top><font size=2>Certificate Authority: "
    "<br>Expiration Date: "
    "</font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "</font></td>"
    "<td valign=center align=right>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</td></tr></table><hr>"
    "Your computer's date is set to ",
    0,
    ".  If this date is incorrect, then you should reset the date on "
    "your computer.<p>"
    "You may continue or cancel this connection."
};

static char *caNotYetGoodDialogStrings[] = {
    "<img src=about:security?banner-secure><br clear=all><p>",
    0,
    " is a site that uses encryption to protect transmitted "
    "information.  However one of the Certificate Authorities that identifies "
    "this site is not yet valid.  This may be because a certificate was "
    "install too soon by the site administrator, or because the date on "
    "your computer is wrong. "
    "Press the More Info button to see details of the expired certificate.<hr>"
    "<table cellspacing=0 cellpadding=0><tr>"
    "<td valign=top><font size=2>Certificate Authority: "
    "<br>Certificate Valid On: "
    "</font></td><td valign=top><font size=2>",
    0,
    "<br>",
    0,
    "</font></td>"
    "<td valign=center align=right>"
    "<input type=\"submit\" name=\"button\" value=\"More Info...\">"
    "</td></tr></table><hr>"
    "Your computer's date is set to ",
    0,
    ".  If this date is incorrect, then you should reset the date on "
    "your computer.<p>"
    "You may continue or cancel this connection."
};

/*
 * global dialog string table
 */
XPDialogStringEntry dialogStringTable[] = {
    { XP_DIALOG_NULL_STRINGS, nullStrings,
	  sizeof(nullStrings) / sizeof(char *) },
    { XP_DIALOG_HEADER_STRINGS, dialogHeaderStrings,
	  sizeof(dialogHeaderStrings) / sizeof(char *) },
    { XP_DIALOG_MIDDLE_STRINGS, dialogMiddleStrings,
	  sizeof(dialogMiddleStrings) / sizeof(char *) },
    { XP_DIALOG_FOOTER_STRINGS, dialogFooterStrings,
	  sizeof(dialogFooterStrings) / sizeof(char *) },
    { XP_DIALOG_CANCEL_BUTTON_STRINGS, dialogCancelButtonStrings,
	  sizeof(dialogCancelButtonStrings) / sizeof(char *) },
    { XP_DIALOG_OK_BUTTON_STRINGS, dialogOkButtonStrings,
	  sizeof(dialogOkButtonStrings) / sizeof(char *) },
    { XP_DIALOG_CONTINUE_BUTTON_STRINGS, dialogContinueButtonStrings,
	  sizeof(dialogContinueButtonStrings) / sizeof(char *) },
    { XP_DIALOG_CANCEL_OK_BUTTON_STRINGS, dialogCancelOKButtonStrings,
	  sizeof(dialogCancelOKButtonStrings) / sizeof(char *) },
    { XP_DIALOG_CANCEL_CONTINUE_BUTTON_STRINGS,
	  dialogCancelContinueButtonStrings,
	  sizeof(dialogCancelContinueButtonStrings) / sizeof(char *) },
    { XP_PANEL_HEADER_STRINGS, panelHeaderStrings,
	  sizeof(panelHeaderStrings) / sizeof(char *) },
    { XP_PANEL_MIDDLE_STRINGS, panelMiddleStrings,
	  sizeof(panelMiddleStrings) / sizeof(char *) },
    { XP_PANEL_FOOTER_STRINGS, panelFooterStrings,
	  sizeof(panelFooterStrings) / sizeof(char *) },
    { XP_PANEL_FIRST_BUTTON_STRINGS, panelFirstButtonStrings,
	  sizeof(panelFirstButtonStrings) / sizeof(char *) },
    { XP_PANEL_MIDDLE_BUTTON_STRINGS, panelMiddleButtonStrings,
	  sizeof(panelMiddleButtonStrings) / sizeof(char *) },
    { XP_PANEL_LAST_BUTTON_STRINGS, panelLastButtonStrings,
	  sizeof(panelLastButtonStrings) / sizeof(char *) },
    { XP_PLAIN_CERT_DIALOG_STRINGS, plainCertDialogStrings,
	  sizeof(plainCertDialogStrings) / sizeof(char *) },
    { XP_CERT_CONFIRM_STRINGS, certConfirmStrings,
	  sizeof(certConfirmStrings) / sizeof(char *) },
    { XP_CERT_PAGE_STRINGS, certPageStrings,
	  sizeof(certPageStrings) / sizeof(char *) },
    { XP_SSL_SERVER_CERT1_STRINGS, sslServerCert1Strings,
	  sizeof(sslServerCert1Strings) / sizeof(char *) },
    { XP_SSL_SERVER_CERT2_STRINGS, sslServerCert2Strings,
	  sizeof(sslServerCert2Strings) / sizeof(char *) },
    { XP_SSL_SERVER_CERT3_STRINGS, sslServerCert3Strings,
	  sizeof(sslServerCert3Strings) / sizeof(char *) },
    { XP_SSL_SERVER_CERT4_STRINGS, sslServerCert4Strings,
	  sizeof(sslServerCert4Strings) / sizeof(char *) },
    { XP_SSL_SERVER_CERT5A_STRINGS, sslServerCert5aStrings,
	  sizeof(sslServerCert5aStrings) / sizeof(char *) },
    { XP_SSL_SERVER_CERT5B_STRINGS, sslServerCert5bStrings,
	  sizeof(sslServerCert5bStrings) / sizeof(char *) },
    { XP_SSL_SERVER_CERT5C_STRINGS, sslServerCert5cStrings,
	  sizeof(sslServerCert5cStrings) / sizeof(char *) },
    { XP_SSL_CERT_DOMAIN_DIALOG_STRINGS, sslCertDomainDialogStrings,
	  sizeof(sslCertDomainDialogStrings) / sizeof(char *) },
    { XP_PW_ENTER_NEW_STRINGS, pwEnterNewStrings,
	  sizeof(pwEnterNewStrings) / sizeof(char *) },
    { XP_PW_RETRY_NEW_STRINGS, pwRetryNewStrings,
	  sizeof(pwRetryNewStrings) / sizeof(char *) },
    { XP_PW_SETUP_INTRO_STRINGS, pwSetupIntroStrings,
	  sizeof(pwSetupIntroStrings) / sizeof(char *) },
    { XP_PW_SETUP_PREFS_STRINGS, pwSetupPrefsStrings,
	  sizeof(pwSetupPrefsStrings) / sizeof(char *) },
    { XP_PW_SETUP_BLANK_NEW_STRINGS, pwSetupBlankNewStrings,
	  sizeof(pwSetupBlankNewStrings) / sizeof(char *) },
    { XP_PW_SETUP_REFUSED_STRINGS, pwSetupRefusedStrings,
	  sizeof(pwSetupRefusedStrings) / sizeof(char *) },
    { XP_PW_CHANGE_ENTER_OLD_STRINGS, pwChangeEnterOldStrings,
	  sizeof(pwChangeEnterOldStrings) / sizeof(char *) },
    { XP_PW_CHANGE_RETRY_OLD_STRINGS, pwChangeRetryOldStrings,
	  sizeof(pwChangeRetryOldStrings) / sizeof(char *) },
    { XP_PW_CHANGE_CHOICE_STRINGS, pwChangeChoiceStrings,
	  sizeof(pwChangeChoiceStrings) / sizeof(char *) },
    { XP_PW_CHANGE_BLANK_NEW_STRINGS, pwChangeBlankNewStrings,
	  sizeof(pwChangeBlankNewStrings) / sizeof(char *) },
    { XP_PW_CHANGE_DISABLED_STRINGS, pwChangeDisabledStrings,
	  sizeof(pwChangeDisabledStrings) / sizeof(char *) },
    { XP_PW_FINISH_FAILURE_STRINGS, pwFinishFailureStrings,
	  sizeof(pwFinishFailureStrings) / sizeof(char *) },
    { XP_PANEL_ONLY_BUTTON_STRINGS, panelOnlyButtonStrings,
	  sizeof(panelOnlyButtonStrings) / sizeof(char *) },
    { XP_NOT_IMPLEMENTED_STRINGS, notImplementedStrings,
	  sizeof(notImplementedStrings) / sizeof(char *) },
    { XP_CERT_VIEW_STRINGS, certViewStrings,
	  sizeof(certViewStrings) / sizeof(char *) },
    { XP_DELETE_USER_CERT_STRINGS, deleteUserCertStrings,
	  sizeof(deleteUserCertStrings) / sizeof(char *) },
    { XP_DELETE_SITE_CERT_STRINGS, deleteSiteCertStrings,
	  sizeof(deleteSiteCertStrings) / sizeof(char *) },
    { XP_DELETE_CA_CERT_STRINGS, deleteCACertStrings,
	  sizeof(deleteCACertStrings) / sizeof(char *) },
    { XP_EDIT_SITE_CERT_STRINGS, editSiteCertStrings,
	  sizeof(editSiteCertStrings) / sizeof(char *) },
    { XP_EDIT_CA_CERT_STRINGS, editCACertStrings,
	  sizeof(editCACertStrings) / sizeof(char *) },
    { XP_SSL_CERT_POST_WARN_DIALOG_STRINGS, sslCertPostWarnDialogStrings,
	  sizeof(sslCertPostWarnDialogStrings) / sizeof(char *) },
    { XP_CA_CERT_DOWNLOAD1_STRINGS, caCertDownload1Strings,
	  sizeof(caCertDownload1Strings) / sizeof(char *) },
    { XP_CA_CERT_DOWNLOAD2_STRINGS, caCertDownload2Strings,
	  sizeof(caCertDownload2Strings) / sizeof(char *) },
    { XP_CA_CERT_DOWNLOAD3_STRINGS, caCertDownload3Strings,
	  sizeof(caCertDownload3Strings) / sizeof(char *) },
    { XP_CA_CERT_DOWNLOAD4_STRINGS, caCertDownload4Strings,
	  sizeof(caCertDownload4Strings) / sizeof(char *) },
    { XP_CA_CERT_DOWNLOAD5_STRINGS, caCertDownload5Strings,
	  sizeof(caCertDownload5Strings) / sizeof(char *) },
    { XP_CA_CERT_DOWNLOAD6_STRINGS, caCertDownload6Strings,
	  sizeof(caCertDownload6Strings) / sizeof(char *) },
    { XP_CA_CERT_DOWNLOAD7_STRINGS, caCertDownload7Strings,
	  sizeof(caCertDownload7Strings) / sizeof(char *) },
    { XP_EMPTY_STRINGS, emptyStrings,
	  sizeof(emptyStrings) / sizeof(char *) },
    { XP_WHICH_CERT_CLIENT_AUTH_DIALOG_STRINGS,
	  sslWhichCertClientAuthDialogStrings,
	  sizeof(sslWhichCertClientAuthDialogStrings) / sizeof(char *) },
    { XP_NO_CERT_CLIENT_AUTH_DIALOG_STRINGS,
	  sslNoCertClientAuthDialogStrings,
	  sizeof(sslNoCertClientAuthDialogStrings) / sizeof(char *) },
    { XP_BROWSER_SEC_INFO_ENCRYPTED_STRINGS, browserSecInfoEncryptedStrings,
	  sizeof(browserSecInfoEncryptedStrings) / sizeof(char *) },
    { XP_BROWSER_SEC_INFO_MIXED_STRINGS, browserSecInfoMixedStrings,
	  sizeof(browserSecInfoMixedStrings) / sizeof(char *) },
    { XP_BROWSER_SEC_INFO_CLEAR_STRINGS, browserSecInfoClearStrings,
	  sizeof(browserSecInfoClearStrings) / sizeof(char *) },
    { XP_SUBMIT_PAYMENT_INTRO_STRINGS, submitPaymentIntroStrings,
	  sizeof(submitPaymentIntroStrings) / sizeof(char *) },
    { XP_SUBMIT_PAYMENT_ORDER_STRINGS, submitPaymentOrderStrings,
	  sizeof(submitPaymentOrderStrings) / sizeof(char *) },
    { XP_SUBMIT_PAYMENT_REVIEW_STRINGS, submitPaymentReviewStrings,
	  sizeof(submitPaymentReviewStrings) / sizeof(char *) },
    { XP_SUBMIT_PAYMENT_PAN_STRINGS, submitPaymentPanStrings,
	  sizeof(submitPaymentPanStrings) / sizeof(char *) },
    { XP_SUBMIT_PAYMENT_ADDR_STRINGS, submitPaymentAddrStrings,
          sizeof(submitPaymentAddrStrings) / sizeof(char *) },
    { XP_SUBMIT_PAYMENT_FINISH_STRINGS, submitPaymentFinishStrings,
 	  sizeof(submitPaymentFinishStrings) / sizeof(char *) },
    { XP_CONFIG_CIPHERS_STRINGS, sslConfigCiphersStrings,
	  sizeof(sslConfigCiphersStrings) / sizeof(char *) },
    { XP_USER_CERT_DOWNLOAD1_STRINGS, userCertDownload1Strings,
	  sizeof(userCertDownload1Strings) / sizeof(char *) },
    { XP_USER_CERT_DOWNLOAD2_STRINGS, userCertDownload2Strings,
	  sizeof(userCertDownload2Strings) / sizeof(char *) },
    { XP_USER_CERT_DOWNLOAD3_STRINGS, userCertDownload3Strings,
	  sizeof(userCertDownload3Strings) / sizeof(char *) },
    { XP_USER_CERT_DOWNLOAD4_STRINGS, userCertDownload4Strings,
	  sizeof(userCertDownload4Strings) / sizeof(char *) },
    { XP_KEY_GEN_DIALOG_STRINGS, keyGenDialogStrings,
	  sizeof(keyGenDialogStrings) / sizeof(char *) },
    { XP_CERT_EXPIRED_DIALOG_STRINGS, certExpiredDialogStrings,
	  sizeof(certExpiredDialogStrings) / sizeof(char *) },
    { XP_CERT_NOT_YET_GOOD_DIALOG_STRINGS, certNotYetGoodDialogStrings,
	  sizeof(certNotYetGoodDialogStrings) / sizeof(char *) },
    { XP_CA_EXPIRED_DIALOG_STRINGS, caExpiredDialogStrings,
	  sizeof(caExpiredDialogStrings) / sizeof(char *) },
    { XP_CA_NOT_YET_GOOD_DIALOG_STRINGS, caNotYetGoodDialogStrings,
	  sizeof(caNotYetGoodDialogStrings) / sizeof(char *) },
};
#endif

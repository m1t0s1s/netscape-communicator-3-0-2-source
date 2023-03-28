/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */

//
// Editor save stuff. LWT 06/01/95
//

#ifdef EDITOR

#include "editor.h"

// Other error values are created from ALLXPSTR.RC during compiling
extern "C" int MK_UNABLE_TO_LOCATE_FILE;
extern "C" int MK_MALFORMED_URL_ERROR;

//-----------------------------------------------------------------------------
// CEditImageLoader
//-----------------------------------------------------------------------------
CEditImageLoader::CEditImageLoader( CEditBuffer *pBuffer, EDT_ImageData *pImageData, 
                XP_Bool bReplaceImage ): 
        m_pBuffer( pBuffer ),
        m_bReplaceImage( bReplaceImage ),
        m_pImageData( edt_DupImageData( pImageData ) )
        {
    char *pStr;
    if( !pBuffer->m_pContext->is_new_document ){
        intn retVal = NET_MakeRelativeURL( LO_GetBaseURL( m_pBuffer->m_pContext ), 
                    m_pImageData->pSrc, 
                    &pStr );
        
        if( retVal != NET_URL_FAIL ){
            XP_FREE( m_pImageData->pSrc );
            m_pImageData->pSrc = pStr;
        }

        retVal = NET_MakeRelativeURL( LO_GetBaseURL( m_pBuffer->m_pContext ), 
                    m_pImageData->pLowSrc, 
                    &pStr );
        
        if( retVal != NET_URL_FAIL ){
            XP_FREE( m_pImageData->pLowSrc );
            m_pImageData->pLowSrc = pStr;
        }

    }
}

CEditImageLoader::~CEditImageLoader(){
    m_pBuffer->m_pLoadingImage = 0;
    FE_ImageLoadDialogDestroy( m_pBuffer->m_pContext );
    FE_EnableClicking( m_pBuffer->m_pContext );
    edt_FreeImageData( m_pImageData );
}

void CEditImageLoader::LoadImage(){
    char *pStr;

    m_pLoImage = LO_NewImageElement( m_pBuffer->m_pContext );
    
    // convert the URL to an absolute path
    pStr = NET_MakeAbsoluteURL( LO_GetBaseURL( m_pBuffer->m_pContext ), 
                m_pImageData->pSrc );
    m_pLoImage->image_url = PA_strdup( pStr );
    XP_FREE( pStr );

    // convert the URL to an absolute path
    pStr = NET_MakeAbsoluteURL( LO_GetBaseURL( m_pBuffer->m_pContext ), 
                m_pImageData->pLowSrc );
    m_pLoImage->lowres_image_url = PA_strdup( pStr );
    XP_FREE( pStr );

    // tell layout to  pass the size info back to us.
    m_pLoImage->ele_id = ED_IMAGE_LOAD_HACK_ID;  
    FE_ImageLoadDialog( m_pBuffer->m_pContext );
    FE_GetImageInfo(m_pBuffer->m_pContext, m_pLoImage, NET_DONT_RELOAD );
}


// The image library is telling us the size of the image.  Actually do the 
//  insert now.
void CEditImageLoader::SetImageInfo(int32 ele_id, int32 width, int32 height){
    if( m_pImageData->iHeight == 0 || m_pImageData->iWidth == 0 ){
        m_pImageData->iHeight = height;
        m_pImageData->iWidth = width;    
    }
    if( m_bReplaceImage ){
        // whack this in here...
        m_pBuffer->m_pCurrent->Image()->SetImageData( m_pImageData );
        // we could be doing a better job here.  We probably repaint too much...
        CEditElement *pContainer = m_pBuffer->m_pCurrent->FindContainer();
        m_pBuffer->Relayout( pContainer, 0, pContainer->GetLastMostChild(), 
                    RELAYOUT_NOCARET );
        m_pBuffer->SelectCurrentElement();
    }
    else {
        m_pBuffer->InsertImage( m_pImageData );
    }
    delete this;
}

//-----------------------------------------------------------------------------
// CEditSaveObject
//-----------------------------------------------------------------------------

unsigned int edt_file_save_stream_write_ready( void *closure ){
    return MAX_WRITE_READY;
}

int edt_FileSaveStreamWrite (void *closure, const char *block, int32 length) {
    CFileSaveObject *pSaver = (CFileSaveObject*)closure;
    return pSaver->NetStreamWrite( block, length );
}

void edt_file_save_stream_complete( void *closure ){
}

void edt_file_save_stream_abort (void *closure, int status) {
}


void edt_UrlExit( URL_Struct *pUrl, int status, MWContext *context )
{
    ((CEditSaveObject*)(pUrl->fe_data))->NetFetchDone( pUrl, status, context );
}



static NET_StreamClass * edt_MakeFileSaveStream (int format_out, void *closure,
    URL_Struct *url, MWContext *context ) {

    NET_StreamClass *stream;
    CFileSaveObject *pSaver = (CFileSaveObject*)url->fe_data;
    // OpenOutputFile will return 0 if OK
    if( !pSaver || pSaver->OpenOutputFile() != 0  ){
        return (NULL);
    }
    
    TRACEMSG(("Setting up attachment stream. Have URL: %s\n", url->address));

    stream = XP_NEW (NET_StreamClass);
    if (stream == NULL) 
        return (NULL);

    XP_MEMSET (stream, 0, sizeof (NET_StreamClass));

    stream->name           = "editor file save";
    stream->complete       = edt_file_save_stream_complete;
    stream->abort          = edt_file_save_stream_abort;
    stream->put_block      = edt_FileSaveStreamWrite;
    stream->is_write_ready = edt_file_save_stream_write_ready;
    stream->data_object    = url->fe_data;
    stream->window_id      = context;

    return stream;
}

static XP_Bool bNetRegistered = FALSE;

//-----------------------------------------------------------------------------
// CFileSaveObject
//-----------------------------------------------------------------------------

CFileSaveObject::CFileSaveObject( MWContext *pContext ) :
        m_pContext( pContext ) ,
        m_bOverwriteAll( FALSE ),
        m_iCurFile(0),
        m_outFile(0),
        m_status( ED_ERROR_NONE ),
        m_bDontOverwriteAll( FALSE ),
        m_bDontOverwrite( FALSE ),
        m_iExtraFiles(0),
        m_pDestPathURL(0)
{
    if( !bNetRegistered ){
        NET_RegisterContentTypeConverter( "*", FO_EDT_SAVE_IMAGE,
                        NULL, edt_MakeFileSaveStream );
#ifdef XP_WIN
        RealNET_RegisterContentTypeConverter( "*", FO_EDT_SAVE_IMAGE,
                        NULL, edt_MakeFileSaveStream );
#else
NET_RegisterContentTypeConverter( "*", FO_EDT_SAVE_IMAGE,
				NULL, edt_MakeFileSaveStream );
#endif /* XP_WIN */
        bNetRegistered = TRUE;
    }

    // MUST DISABLE WINDOW INTERACTION HERE!
    // (It will always be cleared in the GetURLExitRoutine)
    // (This is normally set TRUE by front end before calling NET_GetURL)
    m_pContext->waitingMode = TRUE;
    // Set file save state
    m_pContext->edit_saving_url = TRUE;
}

CFileSaveObject::~CFileSaveObject(){
    if( m_srcImageURLs.Size() ){
        FE_SaveDialogDestroy( m_pContext, m_status, 0);
    }

    // LOU: What should I do here.  There appear to be connections on the 
    //  window that are still open....
    if( NET_AreThereActiveConnectionsForWindow( m_pContext ) ){
        //XP_ASSERT(FALSE);
        //NET_SilentInterruptWindow( m_pContext );
        XP_TRACE(("NET_AreThereActiveConnectionsForWindow is TRUE in ~CFileSaveObject"));
    }

    int i = 0;
    while( i < m_srcImageURLs.Size() ){
        XP_FREE( m_srcImageURLs[i] );
        i++;
    }

    i = 0;
    while( i < m_destImageFileBaseName.Size() ){
        if( m_destImageFileBaseName[i] ){
            XP_FREE( m_destImageFileBaseName[i] );
        }
        i++;
    }

    XP_FREE( m_pDestPathURL );

    // Allow front end interaction
    m_pContext->waitingMode = FALSE;
    // Clear flag (also done in CEditBuffer::FinishedLoad2)
    m_pContext->edit_saving_url = FALSE;
}

void CFileSaveObject::SetDestPathURL( char* pDestURL ){
    m_pDestPathURL = XP_STRDUP( pDestURL );
    char * pLastSlash = XP_STRRCHR( m_pDestPathURL, '/' );
    pLastSlash++;
    *pLastSlash = 0;
}

static ED_FileError edt_StatusResult = ED_ERROR_NONE;

ED_FileError CFileSaveObject::FileFetchComplete() {
    edt_StatusResult = m_status;
    delete this;
    return edt_StatusResult; // ED_ERROR_NONE;
}

ED_FileError CFileSaveObject::SaveFiles(){
    //  Supply count of images + 1 for parent file
    //     Next param indicates if we are downloading (FALSE)
    //     or uploading (TRUE), so this can be used for publishing
    if( m_srcImageURLs.Size() ){
        FE_SaveDialogCreate( m_pContext, m_srcImageURLs.Size()+m_iExtraFiles, FALSE );
    }
    // loop through the tree getting all the image elements...
    return FetchNextFile();
}

intn CFileSaveObject::AddFile( char *pSrcURL, char *pDestURL ){

    int i = 0;
    while( i < m_srcImageURLs.Size() ){
        if( XP_STRCMP( pSrcURL, m_srcImageURLs[i]) == 0 ){
            return i;
        }
        i++;
    }

    m_destImageFileBaseName.Add( XP_STRDUP(pDestURL) );
    return m_srcImageURLs.Add( XP_STRDUP(pSrcURL) );
}



//
// open the file and get the Netlib to fetch it from the cache.
//
ED_FileError CFileSaveObject::FetchNextFile(){
    if( m_status != ED_ERROR_NONE || m_iCurFile >= m_srcImageURLs.Size() ){
        return FileFetchComplete();
    }    
    int i = m_iCurFile++;
    // lou said we shouldn't open file here -- 
    //     should be in edt_MakeFileSaveStream
#if 0
    if( OpenOutputFile() != 0  ){
        // some kind of error opening the file... Punt for now.
        // we should probably make the url absolute or something.
        return FetchNextFile();
    }
#endif
    URL_Struct *pUrl;

    pUrl= NET_CreateURLStruct( m_srcImageURLs[i], NET_DONT_RELOAD );
    // Store file save object so we can access OpenOutputFile etc.
    pUrl->fe_data = this;
    NET_GetURL( pUrl, FO_EDT_SAVE_IMAGE, m_pContext, edt_UrlExit );
    return m_status;
}



void CFileSaveObject::NetFetchDone( URL_Struct *pUrl, int status, MWContext *pContext ){
    // close the file in any case.
    if( m_outFile ){
        XP_FileClose( m_outFile );
        m_outFile = 0;
    }
    // Default: No error, show destination name for most errors
    int   iError = ED_ERROR_NONE;
    char *pFilename = NULL;

    // NOTE: This is the NETLIB error code: (see merrors.h) 
    //   < 0 is an error
    if( status < 0 ){
        // As we become familiar with typical errors,
        //  add specialized error types here
        //  Use " extern MK_..." to access the error value
        if( status == MK_UNABLE_TO_LOCATE_FILE ||
            status == MK_MALFORMED_URL_ERROR ) {
            // Couldn't find source -- use source filename
            //   for error report
            iError = ED_ERROR_SRC_NOT_FOUND;
            if( m_iCurFile) {
                // m_iCurFile is 1-based
                pFilename = GetSrcName(m_iCurFile-1);
            }
        } else if( m_bDontOverwrite ) {
            // We get status = MK_UNABLE_TO_CONVERT 
            //  if user said NO to overwriting an existing file:
            //  edt_MakeFileSaveStream returns 0,
            //  making NET_GetURL think it failed,
            //   but its not really an error
            iError = ED_ERROR_NONE;
        } else if( m_status == ED_ERROR_CANCEL ) {
            // User selected CANCEL at overwrite warning dialog,
            //  thus stream was not created and returns error status,
            //  but its not really an error
            iError = ED_ERROR_CANCEL;
        } else {
            // "Unknown" error default
            iError = ED_ERROR_FILE_WRITE;
        }
        if( m_fileBackup.InTransaction() ){
            m_fileBackup.Rollback();
        }
    }
    else {
        if( m_fileBackup.InTransaction() ){
            m_fileBackup.Commit();
        }
    }

    if( !pFilename ){
        pFilename = GetDestName(m_iCurFile-1);
    }

    // Tell the front end that the file save is finished
    // We use number to tell between Main image (1) and Low-Res image (2)
    FE_FinishedSave( m_pContext, iError, pFilename, m_iCurFile );
    
    if( pFilename ) XP_FREE(pFilename);

    FetchNextFile();
}

intn CFileSaveObject::OpenOutputFile(){
    // Return if we already have a handle
    if( m_outFile ){
        return 0;
    }
    // Current number incremented before comming here,
    //  or think of it as 1-based counting
    int i = m_iCurFile-1;
    char *pDestFileName = 0;
    
    // compute the name of the file.
    if( !m_destImageFileBaseName[i] ){
        // create a temporary name?
        XP_TRACE(("m_destImageFileBaseName is NULL"));
        return 0;
    }
    // create the base name URL...
    char *pDestURL = PR_smprintf( "%s%s", m_pDestPathURL, m_destImageFileBaseName[i] );

    // Must have "file:" URL type and at least 1 character after "///"
    // LTNOTE: this probably only works on windows...
    if ( pDestURL == NULL || !NET_IsLocalFileURL((char*)pDestURL) || XP_STRLEN(pDestURL) <= 8 ) {
        XP_ASSERT(FALSE);
    }

    // Extract file path from URL: e.g. "/c|/foo/file.html"
    char *szPath = NET_ParseURL( pDestURL, GET_PATH_PART);
    if ( szPath == NULL ) {
        if( !FE_SaveErrorContinueDialog( m_pContext, pDestURL, ED_ERROR_FILE_OPEN ) ){
            Cancel();
        }
        XP_FREE(pDestURL);
        return -1;
    }

#if defined(XP_MAC) || defined(XP_UNIX)
    // Get filename - convert it to a local filesystem name.
    pDestFileName = XP_STRDUP(szPath);
#else
    // Get filename - skip over initial "/", convert it to a local filesystem
    //  name.
    pDestFileName = WH_FileName(szPath+1, xpURL);
#endif
    XP_TRACE(("Ready to open file %s to write URL %s", pDestFileName, pDestURL));
    XP_FREE(szPath);
    
    if( !pDestFileName ) {
        XP_TRACE(("Failed to create destination filename"));
        if( !FE_SaveErrorContinueDialog( m_pContext, pDestURL, ED_ERROR_FILE_OPEN ) ){
            Cancel();
        }
        XP_FREE(pDestURL);
        return -1;
    }
    XP_FREE(pDestURL);

    XP_Bool bWriteThis = TRUE;

    // Tell front end the filename even if we are not writing the file
    FE_SaveDialogSetFilename( m_pContext, pDestFileName );
 
    m_bDontOverwrite = m_bDontOverwriteAll;

    if( !m_bOverwriteAll){
        XP_StatStruct statinfo;
        
        if( -1 != XP_Stat(pDestFileName, &statinfo, xpURL) && statinfo.st_mode & S_IFREG ){
            // file exists.  
            if( m_bDontOverwriteAll ){
                bWriteThis = FALSE;
                // Tell front end the filename and that we didn't copy the file
                FE_FinishedSave( m_pContext, ED_ERROR_FILE_EXISTS, 
                                 m_destImageFileBaseName[i], m_iCurFile );
            }
            else {
                ED_SaveOption e;
                e = FE_SaveFileExistsDialog( m_pContext, pDestFileName );

                switch( e ){
                case ED_SAVE_OVERWRITE_THIS:
                    break;
                case ED_SAVE_OVERWRITE_ALL:
                    m_bOverwriteAll = TRUE;
                    break;
                case ED_SAVE_DONT_OVERWRITE_ALL:
                    m_bDontOverwriteAll = TRUE;
                    // intentionally fall through
                case ED_SAVE_DONT_OVERWRITE_THIS:
                    // This flag prevents us from reporting
                    //  "failed" file stream creation as an error
                    m_bDontOverwrite = TRUE;
                    bWriteThis = FALSE;
                    // Tell front end the filename and that we didn't copy the file
                    FE_FinishedSave( m_pContext, ED_ERROR_FILE_EXISTS, 
                                     m_destImageFileBaseName[i], m_iCurFile );
                    break;
                case ED_SAVE_CANCEL:
                    Cancel();
                    bWriteThis = FALSE;
                }
            }
        }
    }

    if( bWriteThis ){

#if 0
        m_fileBackup.Reset();
        ED_FileError status = m_fileBackup.BeginTransaction( pDestURL );
        if( status != ED_ERROR_NONE ){
            if( !FE_SaveErrorContinueDialog( m_pContext, pDestFileName, 
                    status ) ){
                Cancel();
            }
            return -1;
        }
#endif

#ifdef XP_MAC
        m_outFile = XP_FileOpen(pDestFileName, xpURL, XP_FILE_TRUNCATE_BIN );
#else
        m_outFile = XP_FileOpen(pDestFileName, xpTemporary, XP_FILE_TRUNCATE_BIN );
#endif
    }
    else {
        // We are returning, not doing the save, but not an error condition
        return -1;
    }

    XP_TRACE(("XP_FileOpen handle = %d", m_outFile));
    
    if( m_outFile == 0 ){
        XP_TRACE(("Failed to opened file %s", pDestFileName));

        if( !FE_SaveErrorContinueDialog( m_pContext, pDestFileName, 
                ED_ERROR_FILE_OPEN ) ){
            Cancel();
        }
        // file error...
        return -1;
    }
    else {
        FE_SaveErrorContinueDialog( m_pContext, pDestFileName, 
                ED_ERROR_NONE );
    }
    XP_FREE(pDestFileName);
    return 0;
}

int CFileSaveObject::NetStreamWrite( const char *block, int32 length ){
    XP_TRACE(("NetStreamWrite: length = %lu", length));
    return XP_FileWrite( block, length, m_outFile ) != 0;
}

void CFileSaveObject::Cancel(){
    // tell net URL to stop...
    m_status = ED_ERROR_CANCEL;
    // Above alone does seem to work! Don't we need this?
    // NET_InterruptWindow causes a "reentrant interrupt" error message now!
    NET_SilentInterruptWindow( m_pContext );
}

char *CFileSaveObject::GetDestName( intn index ){ 
    if( index >= m_destImageFileBaseName.Size() ){
        return 0;
    }
    else {
        return XP_STRDUP( m_destImageFileBaseName[index] );
    }
}

char *CFileSaveObject::GetSrcName( intn index ){ 
    if( index >= m_srcImageURLs.Size() ){
        return 0;
    }
    else {
        return XP_STRDUP( m_srcImageURLs[index] );
    }
}

//-----------------------------------------------------------------------------
// CEditSaveObject
//-----------------------------------------------------------------------------

CEditSaveObject::CEditSaveObject( CEditBuffer *pBuffer, 
                                    char *pSrcURL, 
                                    char* pDestFileURL,
                                    XP_Bool bSaveAs, 
                                    XP_Bool bKeepImagesWithDoc, 
                                    XP_Bool bAutoAdjustLinks ):
        CFileSaveObject( pBuffer->m_pContext ),
        m_pBuffer( pBuffer ),
        m_pSrcURL( XP_STRDUP( pSrcURL ) ),
        m_bSaveAs( bSaveAs ),
        m_bKeepImagesWithDoc( bKeepImagesWithDoc ),
        m_bAutoAdjustLinks( bAutoAdjustLinks ),
        m_backgroundIndex(-1),
        m_bOverwriteAll( FALSE ),
        m_bFixupImageLinks( FALSE ),
        m_bDontOverwriteAll( FALSE ),
        m_pDestFileURL( XP_STRDUP( pDestFileURL ) )
    {
    // Set a flag so we know when we are saving a file
    m_pContext->edit_saving_url = TRUE;
    // take the destination filename and whack what follows the training slash.
    //  The URL is supposed to be of the form file:///fdslkjfdslkj/fdsfd.html
    //  we don't check.
    //
    m_iExtraFiles = 1;
    SetDestPathURL( m_pDestFileURL );
}

CEditSaveObject::~CEditSaveObject(){
    // This flag must be cleared before GetFileWriteTime,
    //  else the write time will not be recorded
    m_pContext->edit_saving_url = FALSE;
    // Get the current file time
    m_pBuffer->GetFileWriteTime();

    m_pBuffer->m_pSaveObject = 0;
    XP_FREE( m_pDestFileURL );
    XP_FREE( m_pSrcURL );
}


XP_Bool CEditSaveObject::FindAllDocumentImages(){
    EDT_ImageData *pData;
    EDT_PageData *pPageData;
    XP_Bool bFound = FALSE;

    if( m_bKeepImagesWithDoc || MakeImagesAbsolute() ){
        m_bFixupImageLinks = TRUE;
        CEditElement *pImage = m_pBuffer->m_pRoot->FindNextElement( 
                                                 &CEditElement::FindImage, 0 );

        // If there is a background Image, make it the first one.
        pPageData = m_pBuffer->GetPageData();
        if( pPageData->pBackgroundImage ){
            m_backgroundIndex = AddImage( pPageData->pBackgroundImage );        
            bFound = TRUE;
        }
        else {
            m_backgroundIndex = -1;
        }
        m_pBuffer->FreePageData( pPageData );

        while( pImage ){
            // If this assert fails, it is probably because FinishedLoad
            // didn't get called on the image. And that's probably because
            // of some bug in the recursive FinishedLoad code.
            // If the size isn't known, the relayout after the save will block,
            // and the fourth .. nth images in the document will get zero size.
            // XP_ASSERT(pImage->Image()->SizeIsKnown());
            pImage->Image()->m_iSaveIndex = -1;
            pImage->Image()->m_iSaveLowIndex = -1;
            pData = pImage->Image()->GetImageData();
            if( pData->pSrc ){
                pImage->Image()->m_iSaveIndex = AddImage( pData->pSrc );
                bFound = TRUE;
            }
            if( pData->pLowSrc ){
                pImage->Image()->m_iSaveLowIndex = AddImage( pData->pLowSrc );
                bFound = TRUE;
            }
            edt_FreeImageData( pData );
            pImage = pImage->FindNextElement( &CEditElement::FindImage, 0 );
        }
    }
    return bFound;
}

//
// create a full path.  Search for it in our src Image Array.  If we find it
//  return the index of the image we found.
//
intn  CEditSaveObject::AddImage( char *pSrc ){
    intn retVal = -1;
    if( EDT_IS_LIVEWIRE_MACRO( pSrc ) ){
        // ignore this image
        return -1;  
    }

    char *pFullPath = NET_MakeAbsoluteURL( m_pSrcURL, pSrc );
    char *pLocalName;
    int i = 0;

    // this can return 0 if it couldn't generate a good name.
    if( m_bKeepImagesWithDoc ){
        pLocalName = edt_LocalName( pFullPath );
        if( pLocalName != 0 ){  
            if( !IsSameURL( pFullPath, pLocalName ) ){
                retVal = AddFile( pFullPath, pLocalName );
            }
            else {
                retVal = -2;
            }
        }
        else {
            retVal =  -2;       // couldn't make a good local name, make it abs.
        }
        XP_FREE( pFullPath );
        if( pLocalName ) XP_FREE( pLocalName );
    }
    else if( MakeImagesAbsolute() ){
        retVal = -2;
    }
    return retVal;
}

//
// INTL:
// check to see if the source URL is the same as the destPathURL+LocalName
//  probably has international issues on MAC and WIN...
//
XP_Bool CEditSaveObject::IsSameURL( char* pSrcURL, char* pLocalName ){
    char *pDestURL = PR_smprintf( "%s%s", m_pDestPathURL, pLocalName );
#if defined(XP_WIN)                                   
    XP_Bool bRetVal = (strcmpi( pSrcURL, pDestURL ) == 0);
#else
    XP_Bool bRetVal = (XP_STRCMP( pSrcURL, pDestURL ) == 0 );
#endif
    XP_FREE( pDestURL );
    return bRetVal;
}

ED_FileError CEditSaveObject::FileFetchComplete(){
    XP_ASSERT(XP_IsContextInList(m_pBuffer->m_pContext));
    
    // We're done saving all the images, now save Edit buffer to HTML file
    if( m_status == ED_ERROR_NONE ){
        FixupLinks();
        m_status = m_pBuffer->WriteBufferToFile( m_pDestFileURL );

        if( m_status == ED_ERROR_NONE ){
            m_pBuffer->m_pContext->is_new_document = FALSE;
            
            // Set this first - FE_SetDocTitle will get URL from it
            LO_SetBaseURL( m_pBuffer->m_pContext, m_pDestFileURL );
    		History_entry * hist_ent =
    		    SHIST_GetCurrent(&(m_pBuffer->m_pContext->hist));
            char * pTitle = NULL;
	    	if(hist_ent) {
                if ( hist_ent->title && XP_STRLEN(hist_ent->title) > 0 ){
                    // Note: hist_ent->title should always = m_pContext->title?
                    // Change "file:///Untitled" into URL
                    // Use the new address if old title == old address
                    if( 0 != XP_STRCMP(hist_ent->title, "file:///Untitled") && 
                        0 != XP_STRCMP(hist_ent->title, hist_ent->address) ){
                        pTitle = hist_ent->title;
                    } else {
                        pTitle = m_pDestFileURL;
                    }

                } else {
                    // Use the URL if no Document title
                    pTitle = m_pDestFileURL;
                }
                if( pTitle == m_pDestFileURL ){
                    XP_FREE(hist_ent->title);
                    hist_ent->title = XP_STRDUP(m_pDestFileURL);
                }

                // Set history entry address to new URL
	    	    if (hist_ent->address) {
                    XP_FREE(hist_ent->address);
                }
                hist_ent->address = XP_STRDUP(m_pDestFileURL);
                // This changes doc title, window caption, history title,
                ///  but not history address
                FE_SetDocTitle(m_pBuffer->m_pContext, pTitle);
            }
        }
        else {
            FE_SaveErrorContinueDialog( m_pContext, m_pDestFileURL, 
                m_status );
        }
        // Pass current status to CEditBuffer
        m_pBuffer->FileFetchComplete(m_status);
    }
    return CFileSaveObject::FileFetchComplete();
}

void CEditSaveObject::FixupLinks(){
    EDT_ImageData *pData;

    // If there is a background Image, make it the first one.

    if( m_bFixupImageLinks ){

        if( m_backgroundIndex != -1 ){
            char *pNewURL = 0;

            if( m_backgroundIndex == -2 ){
                char *pAbsURL = NET_MakeAbsoluteURL( m_pSrcURL,
                        m_pBuffer->m_pBackgroundImage );
                
                NET_MakeRelativeURL( m_pDestPathURL, 
                    pAbsURL, 
                    &pNewURL );

                XP_FREE( pAbsURL );

            }
            else {
                pNewURL = GetDestName(m_backgroundIndex);
            }

            if( m_pBuffer->m_pBackgroundImage ){
                XP_FREE( m_pBuffer->m_pBackgroundImage );
            }
            m_pBuffer->m_pBackgroundImage = pNewURL;
        }

        CEditElement *pImage = m_pBuffer->m_pRoot->FindNextElement( 
                                                    &CEditElement::FindImage, 0 );
        while( pImage ){
            pData = pImage->Image()->GetImageData();

            // do the normal image
            if( pImage->Image()->m_iSaveIndex >= 0 ){
                XP_FREE( pData->pSrc );
                pData->pSrc = GetDestName( pImage->Image()->m_iSaveIndex );
            }
            else if( pImage->Image()->m_iSaveIndex == -2 ){
                char *pNewURL;
                char *pAbs = NET_MakeAbsoluteURL( m_pSrcURL, pData->pSrc );
                NET_MakeRelativeURL( m_pDestPathURL, 
                    pAbs, 
                    &pNewURL );
                XP_FREE( pData->pSrc );
                XP_FREE( pAbs );

                pData->pSrc = pNewURL;
            }

            // do the lowres image
            if( pImage->Image()->m_iSaveLowIndex >= 0 ){
                XP_FREE( pData->pLowSrc );
                pData->pLowSrc = GetDestName( pImage->Image()->m_iSaveLowIndex );
            }
            else if( pImage->Image()->m_iSaveLowIndex == -2 ){
                char *pNewURL;
                char *pAbs = NET_MakeAbsoluteURL( m_pSrcURL, pData->pLowSrc );
                NET_MakeRelativeURL( m_pDestPathURL, 
                    pAbs, 
                    &pNewURL );
                XP_FREE( pData->pLowSrc );
                XP_FREE( pAbs );

                pData->pLowSrc = pNewURL;
            }

            
            pImage->Image()->SetImageData( pData );
            edt_FreeImageData( pData );
            pImage = pImage->FindNextElement( &CEditElement::FindImage, 0 );
        }
    }
    
    // Adjust the HREFs.
    if( m_bAutoAdjustLinks ){
        m_pBuffer->linkManager.AdjustAllLinks( m_pSrcURL, m_pDestPathURL );
    }
}

//-----------------------------------------------------------------------------
// CEditImageSaveObject
//-----------------------------------------------------------------------------

CEditImageSaveObject::CEditImageSaveObject( CEditBuffer *pBuffer, 
                EDT_ImageData *pData, XP_Bool bReplaceImage ) 
    :
        CFileSaveObject( pBuffer->m_pContext ),
        m_pBuffer( pBuffer ),
        m_pData( edt_DupImageData( pData ) ),
        m_srcIndex(-1),
        m_lowSrcIndex(-1),
        m_bReplaceImage( bReplaceImage )
{
    // we are going to write the image files into
    SetDestPathURL( LO_GetBaseURL( pBuffer->m_pContext ) );
}

CEditImageSaveObject::~CEditImageSaveObject(){
    edt_FreeImageData( m_pData );
    m_pBuffer->m_pSaveObject = 0;
}

ED_FileError CEditImageSaveObject::FileFetchComplete(){

    if( m_status == ED_ERROR_NONE ){
        char *pName = 0;
        if( m_srcIndex != -1 && (pName = GetDestName(m_srcIndex)) != 0 ){
            if( m_pData->pSrc) XP_FREE( m_pData->pSrc );
            m_pData->pSrc = pName;
        }

        if( m_lowSrcIndex != -1 && (pName = GetDestName(m_lowSrcIndex)) != 0 ){
            XP_FREE( m_pData->pLowSrc );
            m_pData->pLowSrc = pName;
        }

        m_pBuffer->m_pLoadingImage = new CEditImageLoader( m_pBuffer, 
                    m_pData, m_bReplaceImage );
        m_pBuffer->m_pLoadingImage->LoadImage();
    }
    return CFileSaveObject::FileFetchComplete();
}


//-----------------------------------------------------------------------------
// CEditBackgroundImageSaveObject
//-----------------------------------------------------------------------------

CEditBackgroundImageSaveObject::CEditBackgroundImageSaveObject( CEditBuffer *pBuffer)
    :
        CFileSaveObject( pBuffer->m_pContext ),
        m_pBuffer( pBuffer )
{
    // we are going to write the image files into
    SetDestPathURL( LO_GetBaseURL( pBuffer->m_pContext ) );
}

CEditBackgroundImageSaveObject::~CEditBackgroundImageSaveObject(){
    m_pBuffer->m_pSaveObject = 0;
}

ED_FileError CEditBackgroundImageSaveObject::FileFetchComplete(){
    // Note: If user canceled in overwrite dialog, then we don't change the background
    if( m_status == ED_ERROR_NONE ){
        if( m_pBuffer->m_pBackgroundImage ) XP_FREE(m_pBuffer->m_pBackgroundImage);

        // Get the one and only image file (without local or URL path)
        m_pBuffer->m_pBackgroundImage = GetDestName(0);

        // This will set background image
        m_pBuffer->RefreshLayout();
        // LO_SetBackgroundImage( m_pBuffer->m_pContext, m_pBuffer->m_pBackgroundImage );
    }
    return CFileSaveObject::FileFetchComplete();
}


//-----------------------------------------------------------------------------
//  Streams, Implementation
//-----------------------------------------------------------------------------

//
// LTNOTE:
// There is some uglyness on different platforms to make this work.  We'll 
//  cross that bridge when we come to it.
//

static char* stream_buffer = 0;
int IStreamOut::Printf( char * pFormat, ... ){
    va_list stack;
    int32 len;

    // if the buffer has been allocated, resize it.
    if( stream_buffer ){
        stream_buffer[0] = 0;
    }
    va_start (stack, pFormat);
    stream_buffer = PR_vsprintf_append( stream_buffer, pFormat, stack );
    va_end (stack);

    len = XP_STRLEN( stream_buffer ); 
    Write( stream_buffer, len );
    return len;
}

void IStreamOut::WriteZString(char* pString){
    if( pString ){
        int32 iLen = XP_STRLEN( pString )+1;
        WriteInt( iLen );
        Write( pString, iLen );
    }
    else {
        WriteInt(0);
    }
}

void IStreamOut::WritePartialZString(char* pString, ElementIndex start, ElementIndex end){
    int32 iLen = end - start;
    XP_ASSERT(iLen >= 0);
    if( pString && iLen > 0 ){
        WriteInt( iLen + 1 ); // Account for the '\0'
        Write( pString + start, iLen );
        Write( "", 1 ); // write the '\0'
    }
    else {
        WriteInt(0);
    }
}

char* IStreamIn::ReadZString(){
    char *pRet = 0;
    int32 iLen = ReadInt();
    if( iLen ){
        pRet = (char*)XP_ALLOC(iLen);
        Read( pRet, iLen );
    }
    return pRet;
}

//-----------------------------------------------------------------------------
// NetStream
//-----------------------------------------------------------------------------
CStreamOutNet::CStreamOutNet( MWContext* pContext )
{
    URL_Struct * URL_s;
    Chrome chrome;

    XP_BZERO( &chrome, sizeof( Chrome ) );
    chrome.allow_close = TRUE;
    chrome.allow_resize = TRUE;
    chrome.show_scrollbar = TRUE;

    //
    // LTNOTE: Ownership of the 'chrome' struct isn't documented in the interface.
    //  The windows implementation doesn't appear to keep pointers to the struct.
    //
    MWContext *pNewContext = FE_MakeNewWindow(pContext, NULL, "view-source",
                    &chrome );
    pNewContext->edit_view_source_hack = TRUE;

    URL_s = NET_CreateURLStruct("View Document Source", NET_DONT_RELOAD);  

    URL_s->content_type = XP_STRDUP(TEXT_PLAIN);

    m_pStream = NET_StreamBuilder(FO_PRESENT, URL_s, pNewContext);

    if(!m_pStream){
        XP_ASSERT( FALSE );
    }
}

CStreamOutNet::~CStreamOutNet(){
    (*m_pStream->complete)(m_pStream->data_object );
}

void CStreamOutNet::Write( char *pBuffer, int32 iCount ){

    // Buffer the output.
    // pBuffer may be a const string, even a ROM string.
    // (e.g. string constants on a Mac with VM.)
    // But networking does in-place transformations on the
    // data to convert it into other character sets.
    
    const int32 kBufferSize = 128;
    char buffer[kBufferSize];
    while ( iCount > 0 ) {
        int32 iChunkSize = iCount;
        if ( iChunkSize > kBufferSize ) {
            iChunkSize = kBufferSize;
        }
        XP_MEMCPY(buffer, pBuffer, iChunkSize);

        int status = (*m_pStream->put_block)(m_pStream->data_object, buffer, iChunkSize );
    
        if(status < 0){
            (*m_pStream->abort)(m_pStream->data_object, status);
        }
        // status??

        iCount -= iChunkSize;
    }
}

//-----------------------------------------------------------------------------
// File Stream
//-----------------------------------------------------------------------------
//CM: For better cross-platform use, call with file handle already open
CStreamOutFile::CStreamOutFile( XP_File hFile, XP_Bool bBinary ){
    m_status = EOS_NoError;
    m_outputFile = hFile;
    m_bBinary = bBinary;
}

CStreamOutFile::~CStreamOutFile(){
    XP_FileClose( m_outputFile );
}

void CStreamOutFile::Write( char *pBuffer, int32 iCount ){

    if( m_status != EOS_NoError ){
        return;
    }

    // this code doesn't work and it probably should be done at the other end
    //  it is designed to fix CR's showing up in the text.

    int iWrote;

    if( !m_bBinary ){
        int i = 0;
        int iLast = 0;
        int iWrite;
        while( i < iCount ){
            if( pBuffer[i] == '\n' ){
                iWrite = i - iLast;
                if( iWrite ){
                    iWrote = XP_FileWrite( &pBuffer[iLast], iWrite, m_outputFile );
                    if( iWrote != iWrite ){ m_status = EOS_DeviceFull; }
                }
#ifdef XP_MAC
				iWrote = XP_FileWrite("\r", 1, m_outputFile );
#else
				iWrote = XP_FileWrite("\n", 1, m_outputFile );
#endif
                if( iWrote != 1 ){ m_status = EOS_DeviceFull; }
                iLast = i+1;
            }
            i++;
        }
        iWrite = i - iLast;
        if( iWrite ){
            iWrote = XP_FileWrite( &pBuffer[iLast], iWrite, m_outputFile );
            if( iWrote != iWrite ){ m_status = EOS_DeviceFull; }
        }
        return;
    }
    else {
        iWrote = XP_FileWrite( pBuffer, iCount, m_outputFile );
        if( iWrote != iCount ){ m_status = EOS_DeviceFull; }
    }
}

//-----------------------------------------------------------------------------
// Memory Streams
//-----------------------------------------------------------------------------

#define MEMBUF_GROW 10
#define MEMBUF_START 32
//
CStreamOutMemory::CStreamOutMemory(): m_pBuffer(0), m_bufferSize(MEMBUF_START),
        m_bufferEnd(0){
    m_pBuffer = (XP_HUGE_CHAR_PTR) XP_HUGE_ALLOC( m_bufferSize );
    m_pBuffer[m_bufferEnd] = '\0';
}
  
//
// This implementation intenttionally does not destroy the buffer.  The buffer
//  is kept and destroyed by the stream user.
//   
CStreamOutMemory::~CStreamOutMemory(){
}

//
//
void CStreamOutMemory::Write( char *pSrc, int32 iCount ){
    int32 neededSize = iCount + m_bufferEnd + 1; /* Extra byte for '\0' */

    //
    // Grow the buffer if need be.
    //
    if( neededSize  > m_bufferSize ){
        int32 iNewSize = (neededSize * 5 / 4) + MEMBUF_GROW;
        XP_HUGE_CHAR_PTR pBuf = (XP_HUGE_CHAR_PTR) XP_HUGE_ALLOC(iNewSize);
        if( pBuf ){
            XP_HUGE_MEMCPY(pBuf, m_pBuffer, m_bufferSize );
            XP_HUGE_FREE(m_pBuffer);
            m_pBuffer = pBuf;
            m_bufferSize = iNewSize;
        }
        else {
            // LTNOTE: throw an out of memory exception
            XP_ASSERT(FALSE);
            return;
        }
    }

    XP_HUGE_MEMCPY( &m_pBuffer[m_bufferEnd], pSrc, iCount );
    m_bufferEnd += iCount;
    m_pBuffer[m_bufferEnd] = '\0';
}

// class CStreamInMemory

void CStreamInMemory::Read( char *pBuffer, int32 iCount ){
    XP_HUGE_MEMCPY( pBuffer, &m_pBuffer[m_iCurrentPos], iCount );
    m_iCurrentPos += iCount;
}


void CEditFileBackup::Reset(){
    if( m_pBackupName ){
        XP_FREE( m_pBackupName );
        m_pBackupName = 0;
    }
    if( m_pFileName ){
        XP_FREE( m_pFileName );
        m_pFileName = 0;
    }
    m_bInitOk = FALSE;
}

ED_FileError CEditFileBackup::BeginTransaction( char *pDestURL ){
    XP_StatStruct statinfo;

    if ( pDestURL == NULL || XP_STRLEN(pDestURL) == 0 || 
             !NET_IsLocalFileURL(pDestURL) ) {
        return ED_ERROR_BAD_URL;
    }
    
    if (XP_ConvertUrlToLocalFile(pDestURL, &m_pFileName) ) {
        // (This must succeed if above test is true, it calls XP_STAT)
        XP_Stat(m_pFileName, &statinfo, xpURL);
        if ( XP_STAT_READONLY( statinfo ) ){
            return ED_ERROR_READ_ONLY;
        }

        // File exists - rename to backup to protect data
        m_pBackupName = XP_BackupFileName(pDestURL);
        if ( m_pBackupName == NULL ) {
            return ED_ERROR_CREATE_BAKNAME;
        }
        // Delete backup file if it exists
        if ( -1 != XP_Stat(m_pBackupName, &statinfo, xpURL) &&
              statinfo.st_mode & S_IFREG ) {
            if ( 0 != XP_FileRemove(m_pBackupName, xpURL) ) {
                return ED_ERROR_DELETE_BAKFILE;
            }
        }
        if ( 0 != XP_FileRename(m_pFileName, xpURL,
                                m_pBackupName, xpURL) ){
            return ED_ERROR_FILE_RENAME_TO_BAK;
        }
    }
    m_bInitOk = TRUE;
    return ED_ERROR_NONE;
}

void CEditFileBackup::Commit() {
    XP_ASSERT( m_bInitOk );

#ifdef XP_UNIX
    XP_StatStruct statinfo;

    if (m_pBackupName != NULL && m_pFileName != NULL &&
		XP_Stat(m_pBackupName, &statinfo, xpURL) != -1) {
		
		/*
		 *    Is there an XP_chmod()? I cannot find one. This will
		 *    work for Unix, which is probably the only place that cares.
		 *    Don't bother to check the return status, as it's too late to
		 *    do anything about, and we are not in dire straights if it fails.
		 *    Bug #28775..djw
		 */
		chmod(m_pFileName, statinfo.st_mode);
	}
#endif

    XP_FileRemove(m_pBackupName, xpURL);
}

void CEditFileBackup::Rollback(){
    XP_ASSERT( m_bInitOk );
    if ( m_pBackupName ) {
        // Restore previous file
        // If this fails, we're really messed up!
        XP_FileRemove(m_pFileName, xpURL);
        XP_FileRename(m_pBackupName, xpURL, m_pFileName, xpURL);
    }
}



#endif

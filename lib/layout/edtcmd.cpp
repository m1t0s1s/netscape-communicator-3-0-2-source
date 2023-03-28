/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */

//
// Editor save stuff. LWT 06/01/95
// this should be on the branch
//

#ifdef EDITOR

#include "editor.h"
#ifdef DEBUG

const char* kCommandNames[kCommandIDMax+1] = {
    "Null",
    "Typing", 
    "AddText", 
    "DeleteText", 
    "Cut", 
    "PasteText", 
    "PasteHTML", 
    "PasteHREF", 
    "ChangeAttributes", 
    "Indent", 
    "ParagraphAlign", 
    "MorphContainer", 
    "InsertHorizRule", 
    "SetHorizRuleData", 
    "InsertImage", 
    "SetImageData", 
    "InsertBreak", 
    "ChangePageData", 
    "SetMetaData", 
    "DeleteMetaData", 
    "InsertTarget", 
    "SetTargetData", 
    "InsertUnknownTag", 
    "SetUnknownTagData", 
    "GroupOfChanges",                                           
    "SetListData",
    "InsertTable",
    "DeleteTable",
    "SetTableData",
    "InsertTableCaption",
    "SetTableCaptionData",
    "DeleteTableCaption",
    "InsertTableRow",
    "SetTableRowData",
    "DeleteTableRow",
    "InsertTableColumn",
    "SetTableCellData",
    "DeleteTableColumn",
    "InsertTableCell",
    "DeleteTableCell",
    "InsertMultiColumn",
    "DeleteMultiColumn",
    "SetMultiColumnData",
    "SetSelection"
};

#endif

CEditText::CEditText() { m_pChars = NULL; m_iLength = 0; m_iCount = 0; }
CEditText::~CEditText() { Clear(); }

void CEditText::Clear() {
    if ( m_pChars )
        XP_HUGE_FREE(m_pChars);  // Tied to implementation of CStreamOutMemory
    m_pChars = 0;
    m_iCount = 0;
    m_iLength = 0;
}

char* CEditText::GetChars() { return m_pChars; }
int32 CEditText::Length() { return m_iLength; }
char** CEditText::GetPChars() { return &m_pChars; }
int32* CEditText::GetPLength() { return &m_iLength; }

// CEditCommand

CEditCommand::CEditCommand(CEditBuffer* editBuffer, intn id)
{
    m_id = id;
    m_editBuffer = editBuffer;
}

CEditCommand::~CEditCommand()
{
}

void CEditCommand::Do()
{
}

void CEditCommand::Undo()
{
}

void CEditCommand::Redo()
{
    Do();
}

intn CEditCommand::GetID()
{
    return m_id;
}

#ifdef DEBUG
void CEditCommand::Print(IStreamOut& stream) {
    const char* name = "Unknown";
    if ( m_id >= 0 && m_id <= kCommandIDMax ){
        name = kCommandNames[m_id];
    }
    stream.Printf("%s(%ld) ", name, (long)m_id);
}
#endif

// CEditCommandLog

// We were having problems where we were entering a command while processing the undo of another command.
// This would cause the command log to be trimmed, which would delete the undo log, including the
// command that was being undone. This helps detect that situation.

#ifdef DEBUG

class CEditCommandLogRecursionCheckEntry {
private:
    XP_Bool m_bOldBusy;
    CEditCommandLog* m_log;
public:
    CEditCommandLogRecursionCheckEntry(CEditCommandLog* log) {
        m_log = log;
        m_bOldBusy = m_log->m_bBusy;
        XP_ASSERT(m_log->m_bBusy == FALSE);
        m_log->m_bBusy = TRUE;
    }
    ~CEditCommandLogRecursionCheckEntry() {
        m_log->m_bBusy = m_bOldBusy;
    }
};

#define DEBUG_RECURSION_CHECK CEditCommandLogRecursionCheckEntry debugRecursionCheckEntry(this);
#else
#define DEBUG_RECURSION_CHECK
#endif

CEditCommandLog::CEditCommandLog()
{
    m_size = m_undoSize = 0;
    m_historyLimit = EDT_CMD_LOG_MAXHISTORY;
    m_commandList.SetSize(m_historyLimit);
#ifdef DEBUG
    m_bBusy = FALSE;
#endif
}

CEditCommandLog::~CEditCommandLog()
{
    Trim();
}

void CEditCommandLog::AdoptAndDo(CEditCommand* pCommand)
{
    DEBUG_RECURSION_CHECK

    if ( m_batch.IsEmpty() ){
        InternalAdoptAndDo(pCommand);
    }
    else
    {
        m_batch.Top()->AdoptAndDo(pCommand);
    }
}

void CEditCommandLog::InternalAdoptAndDo(CEditCommand* command)
{
    command->Do();
    Trim(m_undoSize, m_size);
    if ( m_undoSize >= m_historyLimit ) {
        delete m_commandList[0];
        for ( intn i = 0; i < m_undoSize - 1; i++ ) {
            m_commandList[i] = m_commandList[i + 1];
        }
        m_undoSize--;
    }
    m_commandList[m_undoSize++] = command;
    m_size = m_undoSize;
}

void CEditCommandLog::Undo()
{
    DEBUG_RECURSION_CHECK
    if ( m_undoSize > 0 )
    {
        FinishBatchCommands();
        CEditCommand* command = m_commandList[--m_undoSize];
        command->Undo();
    }
}

void CEditCommandLog::Redo()
{
    DEBUG_RECURSION_CHECK
    if ( m_undoSize < m_size )
    {
        FinishBatchCommands();
        CEditCommand* command = m_commandList[m_undoSize++];
        command->Redo();
    }
}

void CEditCommandLog::FinishBatchCommands()
{
    if ( !m_batch.IsEmpty() ){
        XP_ASSERT(FALSE);
        while ( !m_batch.IsEmpty() ){
            EndBatchChanges();
        }
    }
}

void CEditCommandLog::Trim()
{
    XP_TRACE(("Trimming undo log."));
    Trim(0, m_size);
    m_undoSize = 0;
    m_size = 0;
}

void CEditCommandLog::Trim(intn start, intn end)
{
    for ( intn i = start; i < end; i++ )
    {
        CEditCommand* command = m_commandList[i];
        delete command;
         m_commandList[i] = 0;
    }
}

intn CEditCommandLog::GetCommandHistoryLimit()
{
    return m_historyLimit;
}

void CEditCommandLog::SetCommandHistoryLimit(intn newLimit) {
    if ( newLimit >= 0 ) {
        Trim();
        // To Do: Preserve existing commands, if any.
        m_historyLimit = newLimit;
        m_commandList.SetSize(m_historyLimit);
    }
}

intn CEditCommandLog::GetNumberOfCommandsToUndo()
{
    return m_undoSize;
}

intn CEditCommandLog::GetNumberOfCommandsToRedo()
{
    return m_size - m_undoSize;
}

// Returns NULL if out of range
CEditCommand* CEditCommandLog::GetUndoCommand(intn index)
{
    if ( index < 0 || index >= m_undoSize)
        return NULL;
    return m_commandList[(m_undoSize - 1) - index];
}

CEditCommand* CEditCommandLog::GetRedoCommand(intn index)
{
    if ( index < 0 || index >= GetNumberOfCommandsToRedo() )
        return NULL;
    return m_commandList[m_undoSize + index];
}

#ifdef DEBUG
void CEditCommandLog::Print(IStreamOut& stream) {
    {
        int size = m_batch.StackSize();
        stream.Printf("Batch command stack: %d items\n", size);
        for ( intn i = 0; i < size; i++ ) {
            CEditCommandGroup* pCommand = m_batch[i];
            stream.Printf(" %2d 0x%08x ", i, pCommand);
            pCommand->Print(stream);
            stream.Printf("\n");
        }
    }
    {
        stream.Printf("Undo list: %d commands\n", GetNumberOfCommandsToUndo());
        for ( intn i = 0; i < GetNumberOfCommandsToUndo(); i++ ) {
            CEditCommand* pCommand = GetUndoCommand(i);
            stream.Printf(" %2d 0x%08x ", i, pCommand);
            pCommand->Print(stream);
            stream.Printf("\n");
        }
    }
    {
        stream.Printf("Redo list: %d commands\n", GetNumberOfCommandsToRedo());
        for ( intn i = 0; i < GetNumberOfCommandsToRedo(); i++ ) {
            CEditCommand* pCommand = GetRedoCommand(i);
            stream.Printf(" %2d 0x%08x ", i, pCommand);
            pCommand->Print(stream);
            stream.Printf("\n");
        }
    }
}
#endif

void CEditCommandLog::BeginBatchChanges(CEditCommandGroup* pBatch) {
    m_batch.Push(pBatch);
}

void CEditCommandLog::EndBatchChanges() {
    if(m_batch.IsEmpty()) {
        XP_ASSERT(FALSE);
    }
    else {
        CEditCommandGroup* pBatch = m_batch.Pop();
        if ( pBatch->GetNumberOfCommands() > 0 ){
            if ( ! m_batch.IsEmpty() ) {
                m_batch.Top()->AdoptAndDo(pBatch);
            }
            else {
                InternalAdoptAndDo(pBatch);
            }
        }
        else {
            delete pBatch;
        }
    }
}

// CEditDataSaver

CEditDataSaver::CEditDataSaver(CEditBuffer* pBuffer){
    m_pEditBuffer = pBuffer;
    m_bModifiedTextHasBeenSaved = FALSE;
#ifdef DEBUG
    m_bDoState = 0;
#endif
}

CEditDataSaver::~CEditDataSaver(){
}

void CEditDataSaver::DoBegin(CPersistentEditSelection& original){
#ifdef DEBUG
    XP_ASSERT(m_bDoState == 0);
    m_bDoState++;
#endif
    m_original = original;
    m_pEditBuffer->GetSelection(m_originalDocument);
    CEditSelection selection =
        m_pEditBuffer->PersistentToEphemeral(m_original);
    selection.ExpandToNotCrossStructures();
    m_expandedOriginal = m_pEditBuffer->EphemeralToPersistent(selection);
    m_pEditBuffer->CopyEditText(m_expandedOriginal, m_originalText);
}

void CEditDataSaver::DoEnd(CPersistentEditSelection& modified){
#ifdef DEBUG
    XP_ASSERT(m_bDoState == 1);
    m_bDoState++;
#endif
    m_pEditBuffer->GetSelection(m_modifiedDocument);
    m_expandedModified = m_expandedOriginal;
    m_expandedModified.Map(m_original, modified);
}

void CEditDataSaver::Undo(){
#ifdef DEBUG
    XP_ASSERT(m_bDoState == 2);
    m_bDoState++;
#endif
    m_pEditBuffer->SetSelection(m_expandedModified);
    if ( ! m_bModifiedTextHasBeenSaved ) {
        m_pEditBuffer->CutEditText(m_modifiedText);
        m_bModifiedTextHasBeenSaved = TRUE;
    }
    else {
        m_pEditBuffer->DeleteSelection();
    }
    m_pEditBuffer->PasteEditText(m_originalText);
    m_pEditBuffer->SetSelection(m_originalDocument);
}

void CEditDataSaver::Redo(){
#ifdef DEBUG
    XP_ASSERT(m_bDoState == 3);
    m_bDoState--;
#endif
    m_pEditBuffer->SetSelection(m_expandedOriginal);
    m_pEditBuffer->DeleteSelection();
    m_pEditBuffer->PasteEditText(m_modifiedText);
    m_pEditBuffer->SetSelection(m_modifiedDocument);
}

// CInsertTableCommand
CInsertTableCommand::CInsertTableCommand(CEditBuffer* buffer, CEditTableElement* pTable, intn id)
    : CEditCommand(buffer, id)
{
    GetEditBuffer()->GetSelection(m_originalSelection);
    if( GetEditBuffer()->IsSelected() ){
        GetEditBuffer()->ClearSelection();
    }
    m_helper = new CPasteCommand(buffer, id, FALSE);
    GetEditBuffer()->InsertNonLeaf(pTable);
    m_helper->Do();
    CEditSelection selection = GetEditBuffer()->ComputeCursor(pTable, 0, 0);
    GetEditBuffer()->SetSelection(selection);
}

CInsertTableCommand::~CInsertTableCommand()
{
    delete m_helper;
}

void CInsertTableCommand::Do() {
    // All done in constructor
}

void CInsertTableCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    m_helper->Undo();
    GetEditBuffer()->SetSelection(m_originalSelection);
}

void CInsertTableCommand::Redo() {
    GetEditBuffer()->SetSelection(m_originalSelection);
    m_helper->Redo();
    GetEditBuffer()->SetSelection(m_changedSelection);
}

// CDeleteTableCommand
CDeleteTableCommand::CDeleteTableCommand(CEditBuffer* buffer, intn id)
    : CEditCommand(buffer, id)
{
	m_pTable = NULL;
}

CDeleteTableCommand::~CDeleteTableCommand()
{
    delete m_pTable;
}

void CDeleteTableCommand::Do() {
    GetEditBuffer()->GetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    m_pTable = ip.m_pElement->GetTableIgnoreSubdoc();
	if ( m_pTable ) {
		CEditElement* pRefreshStart = m_pTable->GetFirstMostChild()->PreviousLeaf();
		CEditInsertPoint replacePoint(m_pTable->GetLastMostChild()->NextLeaf(), 0);
		GetEditBuffer()->SetInsertPoint(replacePoint);
		m_pTable->Unlink();
		m_replacePoint = GetEditBuffer()->EphemeralToPersistent(replacePoint);
		GetEditBuffer()->Relayout(pRefreshStart, 0, replacePoint.m_pElement);
	}
}

void CDeleteTableCommand::Undo() {
	if ( m_pTable ) {
		CEditInsertPoint replacePoint = 
			GetEditBuffer()->PersistentToEphemeral(m_replacePoint);
		m_pTable->InsertBefore(replacePoint.m_pElement->GetParent());
		m_pTable = NULL;
		GetEditBuffer()->Relayout(m_pTable, 0, replacePoint.m_pElement, RELAYOUT_NOCARET);
		GetEditBuffer()->SetSelection(m_originalSelection);
	}
}

void CDeleteTableCommand::Redo() {
    CDeleteTableCommand::Do();
}

// CInsertTableCaptionCommand
CInsertTableCaptionCommand::CInsertTableCaptionCommand(CEditBuffer* buffer,
  EDT_TableCaptionData* pData, intn id)
    : CEditCommand(buffer, id)
{
    m_pOldCaption = NULL;
    GetEditBuffer()->GetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ){
        CEditCaptionElement* pCaption = new CEditCaptionElement();
        pCaption->SetData(pData);
        pTable->SetCaption(pCaption);
        pTable->FinishedLoad(GetEditBuffer());
        // CLM: Don't move insert point if we have a selected cell
        //      (We use selection to indicate current cell in property dialogs)
        if( !GetEditBuffer()->IsSelected() &&
            ip.m_pElement->GetTableCellIgnoreSubdoc() != NULL ){
            // Put cursor at end of caption
            ip.m_pElement = pTable->GetCaption()->GetLastMostChild()->Leaf();
            ip.m_iPos = ip.m_pElement->GetLen();
            GetEditBuffer()->SetInsertPoint(ip);
        }
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

CInsertTableCaptionCommand::~CInsertTableCaptionCommand()
{
    delete m_pOldCaption;
}

void CInsertTableCaptionCommand::Do() {
    // All done in constructor
}

void CInsertTableCaptionCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ) {
        m_pOldCaption = pTable->GetCaption();
        m_pOldCaption->Unlink();
        pTable->FinishedLoad(GetEditBuffer());
        GetEditBuffer()->SetSelection(m_originalSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

void CInsertTableCaptionCommand::Redo() {
    GetEditBuffer()->SetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable) {
        pTable->SetCaption(m_pOldCaption);
        m_pOldCaption = NULL;
        pTable->FinishedLoad(GetEditBuffer());
        GetEditBuffer()->Relayout(pTable, 0);
        // Put cursor at end of caption
        ip.m_pElement = pTable->GetCaption()->GetLastMostChild()->Leaf();
        ip.m_iPos = ip.m_pElement->GetLen();
        GetEditBuffer()->SetInsertPoint(ip);
    }
}

// CDeleteTableCaptionCommand
CDeleteTableCaptionCommand::CDeleteTableCaptionCommand(CEditBuffer* buffer, intn id)
    : CEditCommand(buffer, id),
    m_pOldCaption(NULL)
{
}

CDeleteTableCaptionCommand::~CDeleteTableCaptionCommand()
{
    delete m_pOldCaption;
}

void CDeleteTableCaptionCommand::Do() {
    GetEditBuffer()->GetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ){
        m_pOldCaption = pTable->GetCaption();
        if ( m_pOldCaption ) {
            CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
//            intn row = pTableCell ? pTableCell->GetRowIndex() : 0;
//            intn column = pTableCell ? pTableCell->GetColumnIndex() : 0;
            m_pOldCaption->Unlink();
            pTable->FinishedLoad(GetEditBuffer());
            if( !pTableCell ){
                // Set cursor to someplace that still exists
                // CLM: Don't do this if we we were already in a cell
                //GetEditBuffer()->SyncCursor(pTable, column, row);
                GetEditBuffer()->SyncCursor(pTable, 0, 0);
            }
            GetEditBuffer()->Relayout(pTable, 0);
        }
    }
}

void CDeleteTableCaptionCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ) {
        pTable->SetCaption(m_pOldCaption);
        m_pOldCaption = NULL;
        pTable->FinishedLoad(GetEditBuffer());
        GetEditBuffer()->SetSelection(m_originalSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

void CDeleteTableCaptionCommand::Redo() {
    CDeleteTableCaptionCommand::Do();
}

// CInsertTableRowCommand
CInsertTableRowCommand::CInsertTableRowCommand(CEditBuffer* buffer,
  EDT_TableRowData* /* pData */, XP_Bool bAfterCurrentRow, intn number, intn id)
    : CEditCommand(buffer, id)
{
    m_number = number;
    GetEditBuffer()->GetSelection(m_originalSelection);
    if( GetEditBuffer()->IsSelected() ){
        GetEditBuffer()->ClearSelection();
    }
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell) {
        CEditTableElement* pTable = pTableCell->GetTable();
        if ( pTable ){
            intn row = pTableCell->GetRowIndex();
            intn column = pTableCell->GetColumnIndex();
            m_row = row + (bAfterCurrentRow ? 1 : 0);
            pTable->InsertRows(m_row, number);
            pTable->FinishedLoad(GetEditBuffer());
            GetEditBuffer()->SyncCursor(pTable, column, m_row);
            GetEditBuffer()->Relayout(pTable, 0);
        }
    }
}

CInsertTableRowCommand::~CInsertTableRowCommand()
{
}

void CInsertTableRowCommand::Do() {
    // All done in constructor
}

void CInsertTableRowCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ) {
        pTable->DeleteRows(m_row, m_number);
        GetEditBuffer()->SetSelection(m_originalSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

void CInsertTableRowCommand::Redo() {
    GetEditBuffer()->SetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable) {
        pTable->InsertRows(m_row, m_number);
        pTable->FinishedLoad(GetEditBuffer());
        GetEditBuffer()->SetSelection(m_changedSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

// CDeleteTableRowCommand
CDeleteTableRowCommand::CDeleteTableRowCommand(CEditBuffer* buffer, intn rows, intn id)
    : CEditCommand(buffer, id),
    m_table(0,0)
{
	m_rows = rows;
}

CDeleteTableRowCommand::~CDeleteTableRowCommand()
{
}

void CDeleteTableRowCommand::Do() {
    GetEditBuffer()->GetSelection(m_originalSelection);
    Redo();
}

void CDeleteTableRowCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ) {
        pTable->InsertRows(m_row, m_rows, &m_table);
        if ( m_bDeletedWholeTable ) {
            pTable->DeleteRows(pTable->GetRows()-1,1); // Get rid of extra cell
        }
        GetEditBuffer()->Relayout(pTable, 0, 0, RELAYOUT_NOCARET);
        GetEditBuffer()->SetSelection(m_originalSelection);
    }
}

void CDeleteTableRowCommand::Redo() {
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
        CEditTableElement* pTable = pTableCell->GetTable();
        if ( pTable ){
            m_row = pTableCell->GetRowIndex();
            intn iColumn = pTableCell->GetColumnIndex();
            m_bDeletedWholeTable = m_row == 0 && m_rows >= pTable->GetRows();
            pTable->DeleteRows(m_row, m_rows, &m_table);
            pTable->FinishedLoad(GetEditBuffer());
            // Set cursor to someplace that still exists
            GetEditBuffer()->SyncCursor(pTable, iColumn, m_row);
            GetEditBuffer()->Relayout(pTable, 0);
        }
    }
}

// CInsertTableColumnCommand
CInsertTableColumnCommand::CInsertTableColumnCommand(CEditBuffer* buffer,
  EDT_TableCellData* /* pData */, XP_Bool bAfterCurrentCell, intn number, intn id)
    : CEditCommand(buffer, id)
{
    m_number = number;
    GetEditBuffer()->GetSelection(m_originalSelection);
    if( GetEditBuffer()->IsSelected() ){
        GetEditBuffer()->ClearSelection();
    }
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell) {
        CEditTableElement* pTable = pTableCell->GetTable();
        if ( pTable ){
            intn row = pTableCell->GetRowIndex();
            intn column = pTableCell->GetColumnIndex();
            m_column = column + (bAfterCurrentCell ? 1 : 0);
            pTable->InsertColumns(m_column, number);
            pTable->FinishedLoad(GetEditBuffer());
            GetEditBuffer()->SyncCursor(pTable, m_column, row);
            GetEditBuffer()->Relayout(pTable, 0);
        }
    }
}

CInsertTableColumnCommand::~CInsertTableColumnCommand()
{
}

void CInsertTableColumnCommand::Do() {
    // All done in constructor
}

void CInsertTableColumnCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ) {
        pTable->DeleteColumns(m_column, m_number);
        GetEditBuffer()->SetSelection(m_originalSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

void CInsertTableColumnCommand::Redo() {
    GetEditBuffer()->SetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable) {
        pTable->InsertColumns(m_column, m_number);
        pTable->FinishedLoad(GetEditBuffer());
        GetEditBuffer()->SetSelection(m_changedSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

// CDeleteTableColumnCommand
CDeleteTableColumnCommand::CDeleteTableColumnCommand(CEditBuffer* buffer, intn columns, intn id)
    : CEditCommand(buffer, id),
    m_table(0,0)
{
	m_columns = columns;
}

CDeleteTableColumnCommand::~CDeleteTableColumnCommand()
{
}

void CDeleteTableColumnCommand::Do() {
    GetEditBuffer()->GetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
        CEditTableElement* pTable = pTableCell->GetTable();
        if ( pTable ){
            intn iRow = pTableCell->GetRowIndex();
            m_column = pTableCell->GetColumnIndex();
            m_bDeletedWholeTable = m_column == 0 && m_columns >= pTable->GetColumns();
            pTable->DeleteColumns(m_column, m_columns, &m_table);
            pTable->FinishedLoad(GetEditBuffer());
            // Set cursor to someplace that still exists
            GetEditBuffer()->SyncCursor(pTable, m_column, iRow);
            GetEditBuffer()->Relayout(pTable, 0);
        }
    }
}

void CDeleteTableColumnCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ) {
        pTable->InsertColumns(m_column, m_columns, &m_table);
         if ( m_bDeletedWholeTable ) {
            pTable->DeleteColumns(pTable->GetColumns()-1,1); // Get rid of extra cell
        }
        GetEditBuffer()->Relayout(pTable, 0, 0, RELAYOUT_NOCARET);
        GetEditBuffer()->SetSelection(m_originalSelection);
    }
}

void CDeleteTableColumnCommand::Redo() {
    CDeleteTableColumnCommand::Do();
}

// CInsertTableCellCommand
CInsertTableCellCommand::CInsertTableCellCommand(CEditBuffer* buffer,
  XP_Bool bAfterCurrentCell, intn number, intn id)
    : CEditCommand(buffer, id)
{
    m_number = number;
    GetEditBuffer()->GetSelection(m_originalSelection);
    if( GetEditBuffer()->IsSelected() ){
        GetEditBuffer()->ClearSelection();
    }
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell) {
        CEditTableRowElement* pTableRow = pTableCell->GetTableRow();
        CEditTableElement* pTable = pTableCell->GetTable();
        if ( pTable && pTableRow ){
            intn row = pTableCell->GetRowIndex();
            intn column = pTableCell->GetColumnIndex();
            m_column = column + (bAfterCurrentCell ? 1 : 0);
            pTableRow->InsertCells(m_column, number);
            pTable->FinishedLoad(GetEditBuffer());
            GetEditBuffer()->SyncCursor(pTable, m_column, row);
            GetEditBuffer()->Relayout(pTable, 0);
        }
    }
}

CInsertTableCellCommand::~CInsertTableCellCommand()
{
}

void CInsertTableCellCommand::Do() {
    // All done in constructor
}

void CInsertTableCellCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableRowElement* pTableRow = ip.m_pElement->GetTableRowIgnoreSubdoc();
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable && pTableRow ) {
        pTableRow->DeleteCells(m_column, m_number);
        GetEditBuffer()->SetSelection(m_originalSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

void CInsertTableCellCommand::Redo() {
    GetEditBuffer()->SetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    CEditTableRowElement* pTableRow = ip.m_pElement->GetTableRowIgnoreSubdoc();
    if ( pTable && pTableRow) {
        pTableRow->InsertCells(m_column, m_number);
        pTable->FinishedLoad(GetEditBuffer());
        GetEditBuffer()->SetSelection(m_changedSelection);
        GetEditBuffer()->Relayout(pTable, 0);
    }
}

// CDeleteTableCellCommand
CDeleteTableCellCommand::CDeleteTableCellCommand(CEditBuffer* buffer, intn columns, intn id)
    : CEditCommand(buffer, id)
{
	m_columns = columns;
}

CDeleteTableCellCommand::~CDeleteTableCellCommand()
{
}

void CDeleteTableCellCommand::Do() {
    GetEditBuffer()->GetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
        CEditTableRowElement* pTableRow = pTableCell->GetTableRowIgnoreSubdoc();
        CEditTableElement* pTable = pTableCell->GetTableIgnoreSubdoc();
        if ( pTable && pTableRow ){
            intn iRow = pTableCell->GetRowIndex();
            m_column = pTableCell->GetColumnIndex();
            m_bDeletedWholeTable = m_column == 0 && m_columns >= pTableRow->GetCells();
            pTableRow->DeleteCells(m_column, m_columns, &m_tableRow);
            pTable->FinishedLoad(GetEditBuffer());
            // Set cursor to someplace that still exists
            GetEditBuffer()->SyncCursor(pTable, m_column, iRow);
            GetEditBuffer()->Relayout(pTable, 0);
        }
    }
}

void CDeleteTableCellCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetTableInsertPoint(ip);
    CEditTableRowElement* pTableRow = ip.m_pElement->GetTableRowIgnoreSubdoc();
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable && pTableRow ) {
        pTableRow->InsertCells(m_column, m_columns, &m_tableRow);
         if ( m_bDeletedWholeTable ) {
            pTableRow->DeleteCells(pTableRow->GetCells()-1,1); // Get rid of extra cell
        }
        GetEditBuffer()->Relayout(pTable, 0, 0, RELAYOUT_NOCARET);
        GetEditBuffer()->SetSelection(m_originalSelection);
    }
}

void CDeleteTableCellCommand::Redo() {
    CDeleteTableCellCommand::Do();
}

// CInsertMultiColumnCommand
CInsertMultiColumnCommand::CInsertMultiColumnCommand(CEditBuffer* buffer, CEditMultiColumnElement* pMultiColumn, intn id)
    : CEditCommand(buffer, id)
{
    GetEditBuffer()->GetSelection(m_originalSelection);
    if( GetEditBuffer()->IsSelected() ){
        GetEditBuffer()->ClearSelection();
    }
    m_helper = new CPasteCommand(buffer, id, FALSE);
    GetEditBuffer()->InsertNonLeaf(pMultiColumn);
    m_helper->Do();
    GetEditBuffer()->SyncCursor(pMultiColumn);
}

CInsertMultiColumnCommand::~CInsertMultiColumnCommand()
{
    delete m_helper;
}

void CInsertMultiColumnCommand::Do() {
    // All done in constructor
}

void CInsertMultiColumnCommand::Undo() {
    GetEditBuffer()->GetSelection(m_changedSelection);
    m_helper->Undo();
    GetEditBuffer()->SetSelection(m_originalSelection);
}

void CInsertMultiColumnCommand::Redo() {
    GetEditBuffer()->SetSelection(m_originalSelection);
    m_helper->Redo();
    GetEditBuffer()->SetSelection(m_changedSelection);
}

// CDeleteMultiColumnCommand
CDeleteMultiColumnCommand::CDeleteMultiColumnCommand(CEditBuffer* buffer, intn id)
    : CEditCommand(buffer, id)
{
	m_pMultiColumn = NULL;
}

CDeleteMultiColumnCommand::~CDeleteMultiColumnCommand()
{
    delete m_pMultiColumn;
}

void CDeleteMultiColumnCommand::Do() {
    GetEditBuffer()->GetSelection(m_originalSelection);
    CEditInsertPoint ip;
    GetEditBuffer()->GetInsertPoint(ip);
    m_pMultiColumn = ip.m_pElement->GetMultiColumnIgnoreSubdoc();
	if ( m_pMultiColumn ) {
		CEditElement* pRefreshStart = m_pMultiColumn->GetFirstMostChild()->PreviousLeaf();
		CEditInsertPoint replacePoint(m_pMultiColumn->GetLastMostChild()->NextLeaf(), 0);
		GetEditBuffer()->SetInsertPoint(replacePoint);
		m_pMultiColumn->Unlink();
		m_replacePoint = GetEditBuffer()->EphemeralToPersistent(replacePoint);
		GetEditBuffer()->Relayout(pRefreshStart, 0, replacePoint.m_pElement);
	}
}

void CDeleteMultiColumnCommand::Undo() {
	if ( m_pMultiColumn ) {
		CEditInsertPoint replacePoint = 
			GetEditBuffer()->PersistentToEphemeral(m_replacePoint);
		m_pMultiColumn->InsertBefore(replacePoint.m_pElement->GetParent());
		m_pMultiColumn = NULL;
		GetEditBuffer()->SetSelection(m_originalSelection);
		GetEditBuffer()->Relayout(m_pMultiColumn, 0, replacePoint.m_pElement);
	}
}

void CDeleteMultiColumnCommand::Redo() {
    CDeleteMultiColumnCommand::Do();
}

// CPasteCommand

CPasteCommand::CPasteCommand(CEditBuffer* buffer, intn id, XP_Bool bClearOriginal, XP_Bool bPredictiveEnd)
    : CEditCommand( buffer, id),
     m_saver(buffer),
     m_bClearOriginal(bClearOriginal),
     m_bPredictiveEnd(bPredictiveEnd)
{
    CPersistentEditSelection selection;
    if ( m_bClearOriginal ) {
        selection = GetEditBuffer()->GetEffectiveDeleteSelection();
        m_saver.DoBegin(selection);
        // We have to delete the old text here because
        // otherwise we aren't able to figure out the extent of the
        // paste.
        if ( m_bClearOriginal && ! selection.IsInsertPoint() ) {
            GetEditBuffer()->DeleteSelection();
        }
        // remember what is pasted.
        GetEditBuffer()->GetInsertPoint(m_selection.m_start);
    }
    else {
        GetEditBuffer()->GetInsertPoint(m_selection.m_start);
        m_selection.m_end = m_selection.m_start;
        m_saver.DoBegin(m_selection);
    }
}

CPasteCommand::~CPasteCommand()
{
}

void CPasteCommand::Do()
{
    if ( ! m_bPredictiveEnd ){
        GetEditBuffer()->GetInsertPoint(m_selection.m_end);
    }
    else {
        // Assume the item is one unit long.
        m_selection.m_end.m_index = m_selection.m_start.m_index + 1;
    }
    m_saver.DoEnd(m_selection);
}

void CPasteCommand::Undo()
{
    m_saver.Undo();
}

void CPasteCommand::Redo()
{
    m_saver.Redo();
}

#if 0
// CCutCommand

CCutCommand::CCutCommand(CEditBuffer* buffer)
    : CEditTextCommand( buffer, kCutCommandID)
{
}

CCutCommand::~CCutCommand()
{
}

void CCutCommand::Do()
{
    // The cut happens externally, so nothing to do.
}

void CCutCommand::Undo()
{
    GetEditBuffer()->PasteEditText(m_oldText);
    RestoreSelection();
}

void CCutCommand::Redo()
{
    // Fake cut -- leave clipboard alone.
    RestoreSelection();
    GetEditBuffer()->DeleteSelection();
}
#endif

// CAddTextCommand

CAddTextCommand::CAddTextCommand(CEditBuffer* buffer, int cmdNo)
    : CEditCommand(buffer, cmdNo),
      m_saver(buffer)
{
    m_bNeverUndone = TRUE;
    m_bDeleteSpace = FALSE;
}

CAddTextCommand::~CAddTextCommand(){
}


void CAddTextCommand::Do(){
    GetEditBuffer()->GetSelection(m_addedText);
}

void CAddTextCommand::Undo(){
    // The m_saver is being used to undo, rather than to do.
    // That's why we "do" and "redo" here.
    if ( m_bNeverUndone ){
        m_bNeverUndone = FALSE;
        // Cut all the text we added.
        m_saver.DoBegin(m_addedText);
        GetEditBuffer()->SetSelection(m_addedText);
        GetEditBuffer()->DeleteSelection();
        CPersistentEditSelection selection;
        GetEditBuffer()->GetSelection(selection);
        m_saver.DoEnd(selection);
    }
    else {
        m_saver.Redo();
    }
    if ( m_bDeleteSpace ) {
        CPersistentEditSelection selection;
        GetEditBuffer()->GetSelection(selection);
        GetEditBuffer()->InsertChar(' ', FALSE);
        GetEditBuffer()->SetSelection(selection);
    }
}

void CAddTextCommand::Redo(){
    if ( m_bDeleteSpace ) {
        CPersistentEditSelection selection;
        GetEditBuffer()->GetSelection(selection);
        GetEditBuffer()->DeleteChar(TRUE, FALSE);
        GetEditBuffer()->SetSelection(selection);
    }
    m_saver.Undo();
}

void CAddTextCommand::AddCharStart(){
}

void CAddTextCommand::AddCharDeleteSpace(){
    m_bDeleteSpace = TRUE;
}

void CAddTextCommand::AddCharEnd(){
    CPersistentEditSelection current;
    GetEditBuffer()->GetSelection(current);
    m_addedText.m_end = current.m_end;
}

// CTypingCommand

CTypingCommand::CTypingCommand(CEditBuffer* pEditBuffer, int cmdNo)
	: CEditCommand(pEditBuffer, cmdNo),
      m_commands(pEditBuffer, cmdNo)
{
    m_pAdder = NULL;
    m_pDeleter = NULL;
}

CTypingCommand::~CTypingCommand()
{
	GetEditBuffer()->TypingDeleted(this);
}

void CTypingCommand::Do(){
}

void CTypingCommand::Undo()
{
    m_commands.Undo();
}

void CTypingCommand::Redo()
{
    m_commands.Redo();
}

void CTypingCommand::AddCharStart() {
    if ( m_pDeleter ){
        XP_ASSERT(FALSE);
        DeleteEnd(); // bug
    }
    if ( ! m_pAdder ){
        m_pAdder = new CAddTextCommand(GetEditBuffer());
        m_commands.AdoptAndDo(m_pAdder);
    }
    m_pAdder->AddCharStart();
}


void CTypingCommand::AddCharDeleteSpace() {
    XP_ASSERT(m_pAdder);
    if ( m_pAdder ){
        m_pAdder->AddCharDeleteSpace();
    }
}

void CTypingCommand::AddCharEnd() {
    XP_ASSERT(m_pAdder);
    if ( m_pAdder ){
        m_pAdder->AddCharEnd();
    }
}

#ifdef DEBUG
void CTypingCommand::Print(IStreamOut& stream) {
    CEditCommand::Print(stream);
    stream.Printf(" m_pAdder 0x%08x ", m_pAdder);
    stream.Printf(" m_pDeleter 0x%08x ", m_pDeleter);
    stream.Printf(" m_commands: ");
    m_commands.Print(stream);
}
#endif

void CTypingCommand::DeleteStart(CEditSelection& selection){
    if ( m_pDeleter ){
        XP_ASSERT(FALSE);
        DeleteEnd();
    }
    if ( m_pAdder ){
        m_pAdder = NULL; // Ends adder
    }
    CPersistentEditSelection persistentSelection =
        GetEditBuffer()->EphemeralToPersistent(selection);
    m_pDeleter = new CChangeAttributesCommand(GetEditBuffer(), persistentSelection, kDeleteTextCommandID);
}

void CTypingCommand::DeleteEnd(){
    XP_ASSERT(!m_pAdder);
    XP_ASSERT(m_pDeleter);
    if ( m_pDeleter ){
        m_commands.AdoptAndDo(m_pDeleter);
        m_pDeleter = NULL;
    }
}

// CEditCommandGroup

CEditCommandGroup::CEditCommandGroup(CEditBuffer* pEditBuffer, int id)
    : CEditCommand( pEditBuffer, id){
}

CEditCommandGroup::~CEditCommandGroup(){
    for ( int i = 0; i < m_commands.Size(); i++ ) {
        CEditCommand* item = m_commands[i];
        delete item;
    }
}

void CEditCommandGroup::AdoptAndDo(CEditCommand* pCommand){
    pCommand->Do();
    m_commands.Add(pCommand);
}

void CEditCommandGroup::Undo(){
    for ( int i = m_commands.Size() - 1; i >= 0 ; i-- ) {
        CEditCommand* item = m_commands[i];
        item->Undo();
    }
}

void CEditCommandGroup::Redo(){
    for ( int i = 0; i < m_commands.Size(); i++ ) {
        CEditCommand* item = m_commands[i];
        item->Redo();
    }
}

intn CEditCommandGroup::GetNumberOfCommands(){
    return m_commands.Size();
}

#ifdef DEBUG
void CEditCommandGroup::Print(IStreamOut& stream){
    CEditCommand::Print(stream);
    stream.Printf(" %d commands\n", m_commands.Size());
    for ( int i = 0; i < m_commands.Size(); i++ ) {
        CEditCommand* item = m_commands[i];
        stream.Printf("    [%d] 0x%08x ", i, item);
        item->Print(stream);
        stream.Printf("\n");
    }
}
#endif

// CChangeAttributesCommand

CChangeAttributesCommand::CChangeAttributesCommand(CEditBuffer* pEditBuffer, intn id)
    : CEditCommand(pEditBuffer, id),
      m_saver(pEditBuffer)
{
    CEditSelection selection;
    GetEditBuffer()->GetSelection(selection);
    selection.ExcludeLastDocumentContainerEnd();
    selection.ExpandToIncludeFragileSpaces();
    CPersistentEditSelection persel = GetEditBuffer()->EphemeralToPersistent(selection);
    m_saver.DoBegin(persel);
}

CChangeAttributesCommand::CChangeAttributesCommand(CEditBuffer* pEditBuffer, CPersistentEditSelection& selection, intn id)
    : CEditCommand(pEditBuffer, id),
      m_saver(pEditBuffer)
{
    m_saver.DoBegin(selection);
}

CChangeAttributesCommand::~CChangeAttributesCommand() {
}

void CChangeAttributesCommand::Do(){
    CEditSelection selection;
    GetEditBuffer()->GetSelection(selection);
    selection.ExcludeLastDocumentContainerEnd();
    selection.ExpandToIncludeFragileSpaces();
    CPersistentEditSelection persel = GetEditBuffer()->EphemeralToPersistent(selection);
    m_saver.DoEnd(persel);
}

void CChangeAttributesCommand::Undo(){
    m_saver.Undo();
}

void CChangeAttributesCommand::Redo(){
    m_saver.Redo();
}

// CChangeIndentCommand

CChangeIndentCommand::CChangeIndentCommand(CEditBuffer* pEditBuffer, intn id)
    : CEditCommand(pEditBuffer, id),
      m_saver(pEditBuffer)
{
    CEditSelection selection;
    GetEditBuffer()->GetSelection(selection);
    selection.ExpandToEncloseWholeContainers();
    // That made it not cross any list boundaries, but the result may cross
    // table cell boundaries. So call ExpandToBeCutable to get over any
    // table cell boundaries.
    selection.ExpandToBeCutable();
    XP_ASSERT(!selection.CrossesSubDocBoundary());
    m_selection = GetEditBuffer()->EphemeralToPersistent(selection);
    m_saver.DoBegin(m_selection);
}

CChangeIndentCommand::~CChangeIndentCommand(){
}

void CChangeIndentCommand::Do(){
    m_saver.DoEnd(m_selection);
}

void CChangeIndentCommand::Undo(){
    m_saver.Undo();
}

void CChangeIndentCommand::Redo(){
    m_saver.Redo();
}

// CSetListDataCommand
CSetListDataCommand::CSetListDataCommand(CEditBuffer* pBuffer, EDT_ListData& listData, intn id)
    : CEditCommand(pBuffer, id)
{
    m_newData = listData;
    m_pOldData = NULL;
}

CSetListDataCommand::~CSetListDataCommand(){
    if ( m_pOldData ){
        CEditListElement::FreeData( m_pOldData );
    }
}

void CSetListDataCommand::Do(){
    m_pOldData = GetEditBuffer()->GetListData();
    XP_ASSERT(m_pOldData);
    GetEditBuffer()->SetListData(&m_newData);
}

void CSetListDataCommand::Undo(){
    if ( m_pOldData ){
        GetEditBuffer()->SetListData(m_pOldData);
    }
}

void CSetListDataCommand::Redo(){
    GetEditBuffer()->SetListData(&m_newData);
}


// CChangeParagraphCommand
CChangeParagraphCommand::CChangeParagraphCommand(CEditBuffer* buffer, intn id)
    : CChangeAttributesCommand(buffer, id) {
    CEditContainerElement* container = FindContainer(*GetOriginalDocumentSelection());
    m_tag = container->GetType();
    m_align = container->GetAlignment();
}

CChangeParagraphCommand::~CChangeParagraphCommand() {
}

void CChangeParagraphCommand::Undo() {
    SwapAttributes();
    CChangeAttributesCommand::Undo();
}

void CChangeParagraphCommand::Redo() {
    CChangeAttributesCommand::Redo();
    SwapAttributes();
}

void CChangeParagraphCommand::SwapAttributes() {
    CPersistentEditSelection selection;
    GetEditBuffer()->GetSelection(selection);
    CEditContainerElement* pContainer = FindContainer(selection);
    if ( pContainer ) {
        TagType tempTag = pContainer->GetType();
        pContainer->SetType(m_tag);
        m_tag = tempTag;
        ED_Alignment tempAlign = pContainer->GetAlignment();
        pContainer->SetAlignment(m_align);
        m_align = tempAlign;
        GetEditBuffer()->Relayout( pContainer, 0, 0);
    }
}

CEditContainerElement* CChangeParagraphCommand::FindContainer(CPersistentEditSelection& selection) {
    CEditInsertPoint insertPoint = GetEditBuffer()->PersistentToEphemeral(selection.m_end);
    return insertPoint.m_pElement->FindContainer();
}

// CChangePageDataCommand

CChangePageDataCommand::CChangePageDataCommand(CEditBuffer* buffer, intn id)
    : CEditCommand(buffer, id)
{
    m_oldData = GetEditBuffer()->GetPageData();
    m_newData = NULL;
}

CChangePageDataCommand::~CChangePageDataCommand(){
    if ( m_oldData ) {
        GetEditBuffer()->FreePageData(m_oldData);
    }
    if ( m_newData ) {
        GetEditBuffer()->FreePageData(m_newData);
    }
}

void CChangePageDataCommand::Undo(){
    if ( ! m_newData ) {
        m_newData = GetEditBuffer()->GetPageData();
    }
    GetEditBuffer()->SetPageData(m_oldData);
}

void CChangePageDataCommand::Redo(){
    GetEditBuffer()->SetPageData(m_newData);
}

// CSetMetaDataCommand

CSetMetaDataCommand::CSetMetaDataCommand(CEditBuffer* buffer, EDT_MetaData *pMetaData, XP_Bool bDelete, intn id)
    : CEditCommand(buffer, id){
    m_bDelete = bDelete;
    int existingIndex = GetEditBuffer()->FindMetaData( pMetaData);
    m_bNewItem = existingIndex < 0;
    if ( m_bNewItem ) {
        m_pOldData = 0;
    }
    else {
        m_pOldData = GetEditBuffer()->GetMetaData(existingIndex);
    }
    if ( m_bDelete ) {
        GetEditBuffer()->DeleteMetaData(pMetaData);
        m_pNewData = 0;
    }
    else {
        GetEditBuffer()->SetMetaData(pMetaData);
        m_pNewData = GetEditBuffer()->GetMetaData(GetEditBuffer()->FindMetaData(pMetaData));
    }
}

CSetMetaDataCommand::~CSetMetaDataCommand(){
    if ( m_pOldData ) {
        GetEditBuffer()->FreeMetaData(m_pOldData);
    }
    if ( m_pNewData ) {
        GetEditBuffer()->FreeMetaData(m_pNewData);
    }
}

void CSetMetaDataCommand::Undo(){
    if ( m_bNewItem  ) {
        if ( m_pNewData ) {
            GetEditBuffer()->DeleteMetaData(m_pNewData);
        }
    }
    else {
        if ( m_pOldData ) {
            GetEditBuffer()->SetMetaData(m_pOldData);
        }
    }
}

void CSetMetaDataCommand::Redo(){
    if ( m_bDelete ) {
        if ( m_pOldData ) {
            GetEditBuffer()->DeleteMetaData(m_pOldData);
        }
    }
    else {
        if ( m_pNewData ) {
            GetEditBuffer()->SetMetaData(m_pNewData);
        }
    }
}

// CSetTableDataCommand

CSetTableDataCommand::CSetTableDataCommand(CEditBuffer* buffer, EDT_TableData* pTableData, intn id)
    : CEditCommand(buffer, id){
    m_pOldData = GetEditBuffer()->GetTableData();
    GetEditBuffer()->SetTableData(pTableData);
    m_pNewData = GetEditBuffer()->GetTableData();
}

CSetTableDataCommand::~CSetTableDataCommand(){
    CEditTableElement::FreeData(m_pOldData);
    CEditTableElement::FreeData(m_pNewData);
}

void CSetTableDataCommand::Undo(){
    GetEditBuffer()->SetTableData(m_pOldData);
}

void CSetTableDataCommand::Redo(){
    GetEditBuffer()->SetTableData(m_pNewData);
}

CSetTableCaptionDataCommand::CSetTableCaptionDataCommand(CEditBuffer* buffer, EDT_TableCaptionData* pTableData, intn id)
    : CEditCommand(buffer, id){
    m_pOldData = GetEditBuffer()->GetTableCaptionData();
    GetEditBuffer()->SetTableCaptionData(pTableData);
    m_pNewData = GetEditBuffer()->GetTableCaptionData();
}

CSetTableCaptionDataCommand::~CSetTableCaptionDataCommand(){
    CEditCaptionElement::FreeData(m_pOldData);
    CEditCaptionElement::FreeData(m_pNewData);
}

void CSetTableCaptionDataCommand::Undo(){
    GetEditBuffer()->SetTableCaptionData(m_pOldData);
}

void CSetTableCaptionDataCommand::Redo(){
    GetEditBuffer()->SetTableCaptionData(m_pNewData);
}

CSetTableRowDataCommand::CSetTableRowDataCommand(CEditBuffer* buffer, EDT_TableRowData* pTableData, intn id)
    : CEditCommand(buffer, id){
    m_pOldData = GetEditBuffer()->GetTableRowData();
    GetEditBuffer()->SetTableRowData(pTableData);
    m_pNewData = GetEditBuffer()->GetTableRowData();
}

CSetTableRowDataCommand::~CSetTableRowDataCommand(){
    CEditTableRowElement::FreeData(m_pOldData);
    CEditTableRowElement::FreeData(m_pNewData);
}

void CSetTableRowDataCommand::Undo(){
    GetEditBuffer()->SetTableRowData(m_pOldData);
}

void CSetTableRowDataCommand::Redo(){
    GetEditBuffer()->SetTableRowData(m_pNewData);
}

CSetTableCellDataCommand::CSetTableCellDataCommand(CEditBuffer* buffer, EDT_TableCellData* pTableData, intn id)
    : CEditCommand(buffer, id){
    m_pOldData = GetEditBuffer()->GetTableCellData();
    GetEditBuffer()->SetTableCellData(pTableData);
    m_pNewData = GetEditBuffer()->GetTableCellData();
}

CSetTableCellDataCommand::~CSetTableCellDataCommand(){
    CEditTableCellElement::FreeData(m_pOldData);
    CEditTableCellElement::FreeData(m_pNewData);
}

void CSetTableCellDataCommand::Undo(){
    GetEditBuffer()->SetTableCellData(m_pOldData);
}

void CSetTableCellDataCommand::Redo(){
    GetEditBuffer()->SetTableCellData(m_pNewData);
}

CSetMultiColumnDataCommand::CSetMultiColumnDataCommand(CEditBuffer* buffer, EDT_MultiColumnData* pMultiColumnData, intn id)
    : CEditCommand(buffer, id){
    m_pOldData = GetEditBuffer()->GetMultiColumnData();
    GetEditBuffer()->SetMultiColumnData(pMultiColumnData);
    m_pNewData = GetEditBuffer()->GetMultiColumnData();
}

CSetMultiColumnDataCommand::~CSetMultiColumnDataCommand(){
    CEditMultiColumnElement::FreeData(m_pOldData);
    CEditMultiColumnElement::FreeData(m_pNewData);
}

void CSetMultiColumnDataCommand::Undo(){
    GetEditBuffer()->SetMultiColumnData(m_pOldData);
}

void CSetMultiColumnDataCommand::Redo(){
    GetEditBuffer()->SetMultiColumnData(m_pNewData);
}

// CSetSelectionCommand

CSetSelectionCommand::CSetSelectionCommand(CEditBuffer* buffer, CEditSelection& selection, intn id)
    : CEditCommand(buffer, id){
    m_NewSelection = GetEditBuffer()->EphemeralToPersistent(selection);
}

CSetSelectionCommand::~CSetSelectionCommand(){
}

void CSetSelectionCommand::Do(){
    GetEditBuffer()->GetSelection(m_OldSelection);
    GetEditBuffer()->SetSelection(m_NewSelection);
}

void CSetSelectionCommand::Undo(){
    GetEditBuffer()->SetSelection(m_OldSelection);
}

void CSetSelectionCommand::Redo(){
    GetEditBuffer()->SetSelection(m_NewSelection);
}

#endif

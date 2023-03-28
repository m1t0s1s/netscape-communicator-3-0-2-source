//	===========================================================================
//	WRegistry
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<WRegistry.h>
#include	<URegistrar.h>

//	DM classes
#include	<LSelection.h>
#include	<LSelectionModelAEOM.h>
#include	<LEventMachine.h>
#include	<LEventHandler.h>
#include	<LDataDragEventHandler.h>

void	WRegistry::RegisterDMClasses(void)
{
	URegistrar::RegisterStreamable(LSelection::class_ID,
		WStreamableFactory<LSelection>::Create);
	URegistrar::RegisterStreamable(LSelectionModelAEOM::class_ID,
		WStreamableFactory<LSelectionModelAEOM>::Create);
	URegistrar::RegisterStreamable(LEventMachine::class_ID,
		WStreamableFactory<LEventMachine>::Create);
	URegistrar::RegisterStreamable(LEventHandler::class_ID,
		WStreamableFactory<LEventHandler>::Create);
	URegistrar::RegisterStreamable(LDataDragEventHandler::class_ID,
		WStreamableFactory<LDataDragEventHandler>::Create);
}

//	Text
#include	<VTextView.h>
#include	<VTextHandler.h>
#include	<LTextSelection.h>
#include	<VOTextModel.h>
#include	<VTextEngine.h>
#include	<VStyleSet.h>
#include	<LRulerSet.h>
//#include	<VTextElemAEOM.h>

void	WRegistry::RegisterTextClasses(void)
{
/*?
	URegistrar::RegisterClass(LNTextEdit::class_ID,
		(ClassCreatorFunc)WStreamedObjectFactory<LNTextEdit>::Create);
	URegistrar::RegisterClass(LTextView::class_ID,
		(ClassCreatorFunc)WStreamedObjectFactory<LTextView>::Create);
*/

	URegistrar::RegisterClass(VTextView::class_ID,				//	view
		(ClassCreatorFunc)WStreamedObjectFactory<VTextView>::Create);

	URegistrar::RegisterStreamable(VTextHandler::class_ID,		//	handler
		WStreamableFactory<VTextHandler>::Create);
	URegistrar::RegisterStreamable(LTextSelection::class_ID,	//	selection
		WStreamableFactory<LTextSelection>::Create);
	URegistrar::RegisterStreamable(VOTextModel::class_ID,		//	model
		WStreamableFactory<VOTextModel>::Create);
	URegistrar::RegisterStreamable(VTextEngine::class_ID,		//	engine
		WStreamableFactory<VTextEngine>::Create);
	URegistrar::RegisterStreamable(VStyleSet::class_ID,			//	styleset
		WStreamableFactory<VStyleSet>::Create);
	URegistrar::RegisterStreamable(LRulerSet::class_ID,			//	rulerset
		WStreamableFactory<LRulerSet>::Create);
}

//	Tables
#include	<LNTable.h>
#include	<LNTableView.h>
#include	<LActionButtonTable.h>
#include	<L2dDynamicArray.h>
#include	<LTableModel.h>
#include	<LActionButtonArray.h>
#include	<LActionTableHandler.h>

void	WRegistry::RegisterTableClasses(void)
{
	URegistrar::RegisterClass(LNTable::class_ID,
		(ClassCreatorFunc)WStreamedObjectFactory<LNTable>::Create);
	URegistrar::RegisterClass(LNTableView::class_ID,
		(ClassCreatorFunc)WStreamedObjectFactory<LNTableView>::Create);
	URegistrar::RegisterClass(LActionButtonTable::class_ID,
		(ClassCreatorFunc)WStreamedObjectFactory<LActionButtonTable>::Create);

	URegistrar::RegisterClass(L2dDynamicArray::class_ID,
		(ClassCreatorFunc)WStreamedObjectFactory<L2dDynamicArray>::Create);
	URegistrar::RegisterClass(LActionButtonArray::class_ID,
		(ClassCreatorFunc)WStreamedObjectFactory<LActionButtonArray>::Create);

	URegistrar::RegisterStreamable(LTableModel::class_ID,
		WStreamableFactory<LTableModel>::Create);
	URegistrar::RegisterStreamable(LActionTableHandler::class_ID,
		WStreamableFactory<LActionTableHandler>::Create);
}

//	----------------------------------------------------------------------
//	following belongs in LWindow?

#ifndef	PP_No_ObjectStreaming

#include	<UMemoryMgr.h>
#include	<LWindow.h>
#include	<LView.h>
#include	<LModelObject.h>
#include	<LDataStream.h>
#include	<LObjectTable.h>
#include	<UReanimator.h>

LWindow *	WRegistry::ReadVCMWindow(
	ResIDT			inResID,
	LCommander		*inSuperCommander,
	LModelObject	*inSuperModel)
{
	LWindow	*theWindow = NULL;
	
	LCommander::SetDefaultCommander(inSuperCommander);
	
	StResource	objectRes('PPob', inResID);
	HLockHi(objectRes.mResourceH);
	
	LDataStream		stream(*objectRes.mResourceH, GetHandleSize(objectRes.mResourceH));
	LObjectTable	*table = new LObjectTable;
	ThrowIfNULL_(table);
	stream.SetObjectTable(table);
	
	LView			*currentView = LPane::GetDefaultView();
	LCommander		*currentCommander = inSuperCommander
							? inSuperCommander
							: LCommander::GetDefaultCommander();
	LModelObject	*currentModel = inSuperModel
							? inSuperModel
							: LModelObject::GetDefaultModel();

	table->AddObject(kPPSK_View, currentView);
	table->AddObject(kPPSK_Commander, currentCommander);
	table->AddObject(kPPSK_ModelObject, currentModel);
	
	theWindow = (LWindow *)UReanimator::ReadObjects(&stream);
	ThrowIfNULL_(theWindow);
//	Assert_(member(theWindow, LWindow));

	theWindow->FinishCreate();
	if (theWindow->HasAttribute(windAttr_ShowNew)) {
		theWindow->Show();
	}
	
	return theWindow;
}

#endif
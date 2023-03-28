#if defined(AIX) || defined(BSDI) || defined(HPUX) || defined(LINUX) || defined(XP_MAC) || defined(XP_PC)
#include "prtypes.h"

#include "prlink.h"

extern void Java_sun_awt_windows_WToolkit_init_stub();
extern void Java_sun_awt_windows_WToolkit_callbackLoop_stub();
extern void Java_sun_awt_windows_WToolkit_eventLoop_stub();
extern void Java_sun_awt_windows_WToolkit_notImplemented_stub();
extern void Java_sun_awt_windows_WToolkit_makeColorModel_stub();
extern void Java_sun_awt_windows_WToolkit_getScreenResolution_stub();
extern void Java_sun_awt_windows_WToolkit_sync_stub();
extern void Java_sun_awt_windows_WToolkit_getScreenWidth_stub();
extern void Java_sun_awt_windows_WToolkit_getScreenHeight_stub();
extern void Java_sun_awt_windows_WComponentPeer_dispose_stub();
extern void Java_sun_awt_windows_WComponentPeer_show_stub();
extern void Java_sun_awt_windows_WComponentPeer_hide_stub();
extern void Java_sun_awt_windows_WComponentPeer_enable_stub();
extern void Java_sun_awt_windows_WComponentPeer_disable_stub();
extern void Java_sun_awt_windows_WComponentPeer_setForeground_stub();
extern void Java_sun_awt_windows_WComponentPeer_setBackground_stub();
extern void Java_sun_awt_windows_WComponentPeer_setFont_stub();
extern void Java_sun_awt_windows_WComponentPeer_requestFocus_stub();
extern void Java_sun_awt_windows_WComponentPeer_nextFocus_stub();
extern void Java_sun_awt_windows_WComponentPeer_reshape_stub();
extern void Java_sun_awt_windows_WComponentPeer_handleEvent_stub();
extern void Java_sun_awt_windows_WComponentPeer_widget_0005frepaint_stub();
extern void Java_sun_awt_windows_WComponentPeer_setData_stub();
extern void Java_sun_awt_windows_WCanvasPeer_create_stub();
extern void Java_sun_awt_windows_WPanelPeer_create_stub();
extern void Java_sun_awt_windows_WPanelPeer_calculateInsets_stub();
extern void Java_sun_awt_windows_WTextComponentPeer_getText_stub();
extern void Java_sun_awt_windows_WTextComponentPeer_setText_stub();
extern void Java_sun_awt_windows_WTextComponentPeer_getSelectionStart_stub();
extern void Java_sun_awt_windows_WTextComponentPeer_getSelectionEnd_stub();
extern void Java_sun_awt_windows_WTextComponentPeer_select_stub();
extern void Java_sun_awt_windows_WTextComponentPeer_widget_0005fsetEditable_stub();
extern void Java_sun_awt_windows_WTextAreaPeer_create_stub();
extern void Java_sun_awt_windows_WTextAreaPeer_insertText_stub();
extern void Java_sun_awt_windows_WTextAreaPeer_replaceText_stub();
extern void Java_sun_awt_windows_WTextFieldPeer_create_stub();
extern void Java_sun_awt_windows_WTextFieldPeer_setEchoCharacter_stub();
extern void Java_sun_awt_windows_WLabelPeer_create_stub();
extern void Java_sun_awt_windows_WLabelPeer_setText_stub();
extern void Java_sun_awt_windows_WLabelPeer_setAlignment_stub();
extern void Java_sun_awt_windows_WListPeer_create_stub();
extern void Java_sun_awt_windows_WListPeer_setMultipleSelections_stub();
extern void Java_sun_awt_windows_WListPeer_addItem_stub();
extern void Java_sun_awt_windows_WListPeer_delItems_stub();
extern void Java_sun_awt_windows_WListPeer_select_stub();
extern void Java_sun_awt_windows_WListPeer_deselect_stub();
extern void Java_sun_awt_windows_WListPeer_makeVisible_stub();
extern void Java_sun_awt_windows_WListPeer_isSelected_stub();
extern void Java_sun_awt_windows_WScrollbarPeer_pCreate_stub();
extern void Java_sun_awt_windows_WScrollbarPeer_setValue_stub();
extern void Java_sun_awt_windows_WScrollbarPeer_setValues_stub();
extern void Java_sun_awt_windows_WScrollbarPeer_setLineIncrement_stub();
extern void Java_sun_awt_windows_WScrollbarPeer_setPageIncrement_stub();
extern void Java_sun_awt_windows_WCheckboxPeer_create_stub();
extern void Java_sun_awt_windows_WCheckboxPeer_setLabel_stub();
extern void Java_sun_awt_windows_WCheckboxPeer_setState_stub();
extern void Java_sun_awt_windows_WCheckboxPeer_setCheckboxGroup_stub();
extern void Java_sun_awt_windows_WButtonPeer_create_stub();
extern void Java_sun_awt_windows_WButtonPeer_setLabel_stub();
extern void Java_sun_awt_windows_WChoicePeer_create_stub();
extern void Java_sun_awt_windows_WChoicePeer_addItem_stub();
extern void Java_sun_awt_windows_WChoicePeer_select_stub();
extern void Java_sun_awt_windows_WChoicePeer_remove_stub();
extern void Java_sun_awt_windows_WFramePeer_create_stub();
extern void Java_sun_awt_windows_WFramePeer_widget_0005fsetIconImage_stub();
extern void Java_sun_awt_windows_WFramePeer_setTitle_stub();
extern void Java_sun_awt_windows_WFramePeer_setMenuBar_stub();
extern void Java_sun_awt_windows_WFramePeer_setResizable_stub();
extern void Java_sun_awt_windows_WFramePeer_setCursor_stub();
extern void Java_sun_awt_windows_WMenuPeer_createMenu_stub();
extern void Java_sun_awt_windows_WMenuPeer_createSubMenu_stub();
extern void Java_sun_awt_windows_WMenuPeer_dispose_stub();
extern void Java_sun_awt_windows_WMenuPeer_pEnable_stub();
extern void Java_sun_awt_windows_WMenuPeer_pSetLabel_stub();
extern void Java_sun_awt_windows_WMenuBarPeer_create_stub();
extern void Java_sun_awt_windows_WMenuBarPeer_dispose_stub();
extern void Java_sun_awt_windows_WMenuItemPeer_create_stub();
extern void Java_sun_awt_windows_WMenuItemPeer_pEnable_stub();
extern void Java_sun_awt_windows_WMenuItemPeer_pSetLabel_stub();
extern void Java_sun_awt_windows_WMenuItemPeer_pDispose_stub();
extern void Java_sun_awt_windows_WCheckboxMenuItemPeer_create_stub();
extern void Java_sun_awt_windows_WCheckboxMenuItemPeer_pSetState_stub();
extern void Java_sun_awt_windows_WFontMetrics_stringWidth_stub();
extern void Java_sun_awt_windows_WFontMetrics_charsWidth_stub();
extern void Java_sun_awt_windows_WFontMetrics_bytesWidth_stub();
extern void Java_sun_awt_windows_WFontMetrics_loadFontMetrics_stub();
extern void Java_sun_awt_windows_WGraphics_createFromComponent_stub();
extern void Java_sun_awt_windows_WGraphics_createFromGraphics_stub();
extern void Java_sun_awt_windows_WGraphics_imageCreate_stub();
extern void Java_sun_awt_windows_WGraphics_pSetFont_stub();
extern void Java_sun_awt_windows_WGraphics_pSetForeground_stub();
extern void Java_sun_awt_windows_WGraphics_dispose_stub();
extern void Java_sun_awt_windows_WGraphics_setPaintMode_stub();
extern void Java_sun_awt_windows_WGraphics_setXORMode_stub();
extern void Java_sun_awt_windows_WGraphics_getClipRect_stub();
extern void Java_sun_awt_windows_WGraphics_clipRect_stub();
extern void Java_sun_awt_windows_WGraphics_clearRect_stub();
extern void Java_sun_awt_windows_WGraphics_fillRect_stub();
extern void Java_sun_awt_windows_WGraphics_drawRect_stub();
extern void Java_sun_awt_windows_WGraphics_drawString_stub();
extern void Java_sun_awt_windows_WGraphics_drawChars_stub();
extern void Java_sun_awt_windows_WGraphics_drawBytes_stub();
extern void Java_sun_awt_windows_WGraphics_drawStringWidth_stub();
extern void Java_sun_awt_windows_WGraphics_drawCharsWidth_stub();
extern void Java_sun_awt_windows_WGraphics_drawBytesWidth_stub();
extern void Java_sun_awt_windows_WGraphics_drawLine_stub();
extern void Java_sun_awt_windows_WGraphics_copyArea_stub();
extern void Java_sun_awt_windows_WGraphics_drawRoundRect_stub();
extern void Java_sun_awt_windows_WGraphics_fillRoundRect_stub();
extern void Java_sun_awt_windows_WGraphics_drawPolygon_stub();
extern void Java_sun_awt_windows_WGraphics_fillPolygon_stub();
extern void Java_sun_awt_windows_WGraphics_drawOval_stub();
extern void Java_sun_awt_windows_WGraphics_fillOval_stub();
extern void Java_sun_awt_windows_WGraphics_drawArc_stub();
extern void Java_sun_awt_windows_WGraphics_fillArc_stub();
extern void Java_sun_awt_image_ImageRepresentation_offscreenInit_stub();
extern void Java_sun_awt_image_ImageRepresentation_setBytePixels_stub();
extern void Java_sun_awt_image_ImageRepresentation_setIntPixels_stub();
extern void Java_sun_awt_image_ImageRepresentation_finish_stub();
extern void Java_sun_awt_image_ImageRepresentation_imageDraw_stub();
extern void Java_sun_awt_image_ImageRepresentation_disposeImage_stub();
extern void Java_sun_awt_image_OffScreenImageSource_sendPixels_stub();











PRStaticLinkTable FAR awt_nodl_tab[] = {
  { "Java_sun_awt_windows_WToolkit_init_stub", Java_sun_awt_windows_WToolkit_init_stub },
  { "Java_sun_awt_windows_WToolkit_callbackLoop_stub", Java_sun_awt_windows_WToolkit_callbackLoop_stub },
  { "Java_sun_awt_windows_WToolkit_eventLoop_stub", Java_sun_awt_windows_WToolkit_eventLoop_stub },
  { "Java_sun_awt_windows_WToolkit_notImplemented_stub", Java_sun_awt_windows_WToolkit_notImplemented_stub },
  { "Java_sun_awt_windows_WToolkit_makeColorModel_stub", Java_sun_awt_windows_WToolkit_makeColorModel_stub },
  { "Java_sun_awt_windows_WToolkit_getScreenResolution_stub", Java_sun_awt_windows_WToolkit_getScreenResolution_stub },
  { "Java_sun_awt_windows_WToolkit_sync_stub", Java_sun_awt_windows_WToolkit_sync_stub },
  { "Java_sun_awt_windows_WToolkit_getScreenWidth_stub", Java_sun_awt_windows_WToolkit_getScreenWidth_stub },
  { "Java_sun_awt_windows_WToolkit_getScreenHeight_stub", Java_sun_awt_windows_WToolkit_getScreenHeight_stub },
  { "Java_sun_awt_windows_WComponentPeer_dispose_stub", Java_sun_awt_windows_WComponentPeer_dispose_stub },
  { "Java_sun_awt_windows_WComponentPeer_show_stub", Java_sun_awt_windows_WComponentPeer_show_stub },
  { "Java_sun_awt_windows_WComponentPeer_hide_stub", Java_sun_awt_windows_WComponentPeer_hide_stub },
  { "Java_sun_awt_windows_WComponentPeer_enable_stub", Java_sun_awt_windows_WComponentPeer_enable_stub },
  { "Java_sun_awt_windows_WComponentPeer_disable_stub", Java_sun_awt_windows_WComponentPeer_disable_stub },
  { "Java_sun_awt_windows_WComponentPeer_setForeground_stub", Java_sun_awt_windows_WComponentPeer_setForeground_stub },
  { "Java_sun_awt_windows_WComponentPeer_setBackground_stub", Java_sun_awt_windows_WComponentPeer_setBackground_stub },
  { "Java_sun_awt_windows_WComponentPeer_setFont_stub", Java_sun_awt_windows_WComponentPeer_setFont_stub },
  { "Java_sun_awt_windows_WComponentPeer_requestFocus_stub", Java_sun_awt_windows_WComponentPeer_requestFocus_stub },
  { "Java_sun_awt_windows_WComponentPeer_nextFocus_stub", Java_sun_awt_windows_WComponentPeer_nextFocus_stub },
  { "Java_sun_awt_windows_WComponentPeer_reshape_stub", Java_sun_awt_windows_WComponentPeer_reshape_stub },
  { "Java_sun_awt_windows_WComponentPeer_handleEvent_stub", Java_sun_awt_windows_WComponentPeer_handleEvent_stub },
  { "Java_sun_awt_windows_WComponentPeer_widget_0005frepaint_stub", Java_sun_awt_windows_WComponentPeer_widget_0005frepaint_stub },
  { "Java_sun_awt_windows_WComponentPeer_setData_stub", Java_sun_awt_windows_WComponentPeer_setData_stub },
  { "Java_sun_awt_windows_WCanvasPeer_create_stub", Java_sun_awt_windows_WCanvasPeer_create_stub },
  { "Java_sun_awt_windows_WPanelPeer_create_stub", Java_sun_awt_windows_WPanelPeer_create_stub },
  { "Java_sun_awt_windows_WPanelPeer_calculateInsets_stub", Java_sun_awt_windows_WPanelPeer_calculateInsets_stub },
  { "Java_sun_awt_windows_WTextComponentPeer_getText_stub", Java_sun_awt_windows_WTextComponentPeer_getText_stub },
  { "Java_sun_awt_windows_WTextComponentPeer_setText_stub", Java_sun_awt_windows_WTextComponentPeer_setText_stub },
  { "Java_sun_awt_windows_WTextComponentPeer_getSelectionStart_stub", Java_sun_awt_windows_WTextComponentPeer_getSelectionStart_stub },
  { "Java_sun_awt_windows_WTextComponentPeer_getSelectionEnd_stub", Java_sun_awt_windows_WTextComponentPeer_getSelectionEnd_stub },
  { "Java_sun_awt_windows_WTextComponentPeer_select_stub", Java_sun_awt_windows_WTextComponentPeer_select_stub },
  { "Java_sun_awt_windows_WTextComponentPeer_widget_0005fsetEditable_stub", Java_sun_awt_windows_WTextComponentPeer_widget_0005fsetEditable_stub },
  { "Java_sun_awt_windows_WTextAreaPeer_create_stub", Java_sun_awt_windows_WTextAreaPeer_create_stub },
  { "Java_sun_awt_windows_WTextAreaPeer_insertText_stub", Java_sun_awt_windows_WTextAreaPeer_insertText_stub },
  { "Java_sun_awt_windows_WTextAreaPeer_replaceText_stub", Java_sun_awt_windows_WTextAreaPeer_replaceText_stub },
  { "Java_sun_awt_windows_WTextFieldPeer_create_stub", Java_sun_awt_windows_WTextFieldPeer_create_stub },
  { "Java_sun_awt_windows_WTextFieldPeer_setEchoCharacter_stub", Java_sun_awt_windows_WTextFieldPeer_setEchoCharacter_stub },
  { "Java_sun_awt_windows_WLabelPeer_create_stub", Java_sun_awt_windows_WLabelPeer_create_stub },
  { "Java_sun_awt_windows_WLabelPeer_setText_stub", Java_sun_awt_windows_WLabelPeer_setText_stub },
  { "Java_sun_awt_windows_WLabelPeer_setAlignment_stub", Java_sun_awt_windows_WLabelPeer_setAlignment_stub },
  { "Java_sun_awt_windows_WListPeer_create_stub", Java_sun_awt_windows_WListPeer_create_stub },
  { "Java_sun_awt_windows_WListPeer_setMultipleSelections_stub", Java_sun_awt_windows_WListPeer_setMultipleSelections_stub },
  { "Java_sun_awt_windows_WListPeer_addItem_stub", Java_sun_awt_windows_WListPeer_addItem_stub },
  { "Java_sun_awt_windows_WListPeer_delItems_stub", Java_sun_awt_windows_WListPeer_delItems_stub },
  { "Java_sun_awt_windows_WListPeer_select_stub", Java_sun_awt_windows_WListPeer_select_stub },
  { "Java_sun_awt_windows_WListPeer_deselect_stub", Java_sun_awt_windows_WListPeer_deselect_stub },
  { "Java_sun_awt_windows_WListPeer_makeVisible_stub", Java_sun_awt_windows_WListPeer_makeVisible_stub },
  { "Java_sun_awt_windows_WListPeer_isSelected_stub", Java_sun_awt_windows_WListPeer_isSelected_stub },
  { "Java_sun_awt_windows_WScrollbarPeer_pCreate_stub", Java_sun_awt_windows_WScrollbarPeer_pCreate_stub },
  { "Java_sun_awt_windows_WScrollbarPeer_setValue_stub", Java_sun_awt_windows_WScrollbarPeer_setValue_stub },
  { "Java_sun_awt_windows_WScrollbarPeer_setValues_stub", Java_sun_awt_windows_WScrollbarPeer_setValues_stub },
  { "Java_sun_awt_windows_WScrollbarPeer_setLineIncrement_stub", Java_sun_awt_windows_WScrollbarPeer_setLineIncrement_stub },
  { "Java_sun_awt_windows_WScrollbarPeer_setPageIncrement_stub", Java_sun_awt_windows_WScrollbarPeer_setPageIncrement_stub },
  { "Java_sun_awt_windows_WCheckboxPeer_create_stub", Java_sun_awt_windows_WCheckboxPeer_create_stub },
  { "Java_sun_awt_windows_WCheckboxPeer_setLabel_stub", Java_sun_awt_windows_WCheckboxPeer_setLabel_stub },
  { "Java_sun_awt_windows_WCheckboxPeer_setState_stub", Java_sun_awt_windows_WCheckboxPeer_setState_stub },
  { "Java_sun_awt_windows_WCheckboxPeer_setCheckboxGroup_stub", Java_sun_awt_windows_WCheckboxPeer_setCheckboxGroup_stub },
  { "Java_sun_awt_windows_WButtonPeer_create_stub", Java_sun_awt_windows_WButtonPeer_create_stub },
  { "Java_sun_awt_windows_WButtonPeer_setLabel_stub", Java_sun_awt_windows_WButtonPeer_setLabel_stub },
  { "Java_sun_awt_windows_WChoicePeer_create_stub", Java_sun_awt_windows_WChoicePeer_create_stub },
  { "Java_sun_awt_windows_WChoicePeer_addItem_stub", Java_sun_awt_windows_WChoicePeer_addItem_stub },
  { "Java_sun_awt_windows_WChoicePeer_select_stub", Java_sun_awt_windows_WChoicePeer_select_stub },
  { "Java_sun_awt_windows_WChoicePeer_remove_stub", Java_sun_awt_windows_WChoicePeer_remove_stub },
  { "Java_sun_awt_windows_WFramePeer_create_stub", Java_sun_awt_windows_WFramePeer_create_stub },
  { "Java_sun_awt_windows_WFramePeer_widget_0005fsetIconImage_stub", Java_sun_awt_windows_WFramePeer_widget_0005fsetIconImage_stub },
  { "Java_sun_awt_windows_WFramePeer_setTitle_stub", Java_sun_awt_windows_WFramePeer_setTitle_stub },
  { "Java_sun_awt_windows_WFramePeer_setMenuBar_stub", Java_sun_awt_windows_WFramePeer_setMenuBar_stub },
  { "Java_sun_awt_windows_WFramePeer_setResizable_stub", Java_sun_awt_windows_WFramePeer_setResizable_stub },
  { "Java_sun_awt_windows_WFramePeer_setCursor_stub", Java_sun_awt_windows_WFramePeer_setCursor_stub },
  { "Java_sun_awt_windows_WMenuPeer_createMenu_stub", Java_sun_awt_windows_WMenuPeer_createMenu_stub },
  { "Java_sun_awt_windows_WMenuPeer_createSubMenu_stub", Java_sun_awt_windows_WMenuPeer_createSubMenu_stub },
  { "Java_sun_awt_windows_WMenuPeer_dispose_stub", Java_sun_awt_windows_WMenuPeer_dispose_stub },
  { "Java_sun_awt_windows_WMenuPeer_pEnable_stub", Java_sun_awt_windows_WMenuPeer_pEnable_stub },
  { "Java_sun_awt_windows_WMenuPeer_pSetLabel_stub", Java_sun_awt_windows_WMenuPeer_pSetLabel_stub },
  { "Java_sun_awt_windows_WMenuBarPeer_create_stub", Java_sun_awt_windows_WMenuBarPeer_create_stub },
  { "Java_sun_awt_windows_WMenuBarPeer_dispose_stub", Java_sun_awt_windows_WMenuBarPeer_dispose_stub },
  { "Java_sun_awt_windows_WMenuItemPeer_create_stub", Java_sun_awt_windows_WMenuItemPeer_create_stub },
  { "Java_sun_awt_windows_WMenuItemPeer_pEnable_stub", Java_sun_awt_windows_WMenuItemPeer_pEnable_stub },
  { "Java_sun_awt_windows_WMenuItemPeer_pSetLabel_stub", Java_sun_awt_windows_WMenuItemPeer_pSetLabel_stub },
  { "Java_sun_awt_windows_WMenuItemPeer_pDispose_stub", Java_sun_awt_windows_WMenuItemPeer_pDispose_stub },
  { "Java_sun_awt_windows_WCheckboxMenuItemPeer_create_stub", Java_sun_awt_windows_WCheckboxMenuItemPeer_create_stub },
  { "Java_sun_awt_windows_WCheckboxMenuItemPeer_pSetState_stub", Java_sun_awt_windows_WCheckboxMenuItemPeer_pSetState_stub },
  { "Java_sun_awt_windows_WFontMetrics_stringWidth_stub", Java_sun_awt_windows_WFontMetrics_stringWidth_stub },
  { "Java_sun_awt_windows_WFontMetrics_charsWidth_stub", Java_sun_awt_windows_WFontMetrics_charsWidth_stub },
  { "Java_sun_awt_windows_WFontMetrics_bytesWidth_stub", Java_sun_awt_windows_WFontMetrics_bytesWidth_stub },
  { "Java_sun_awt_windows_WFontMetrics_loadFontMetrics_stub", Java_sun_awt_windows_WFontMetrics_loadFontMetrics_stub },
  { "Java_sun_awt_windows_WGraphics_createFromComponent_stub", Java_sun_awt_windows_WGraphics_createFromComponent_stub },
  { "Java_sun_awt_windows_WGraphics_createFromGraphics_stub", Java_sun_awt_windows_WGraphics_createFromGraphics_stub },
  { "Java_sun_awt_windows_WGraphics_imageCreate_stub", Java_sun_awt_windows_WGraphics_imageCreate_stub },
  { "Java_sun_awt_windows_WGraphics_pSetFont_stub", Java_sun_awt_windows_WGraphics_pSetFont_stub },
  { "Java_sun_awt_windows_WGraphics_pSetForeground_stub", Java_sun_awt_windows_WGraphics_pSetForeground_stub },
  { "Java_sun_awt_windows_WGraphics_dispose_stub", Java_sun_awt_windows_WGraphics_dispose_stub },
  { "Java_sun_awt_windows_WGraphics_setPaintMode_stub", Java_sun_awt_windows_WGraphics_setPaintMode_stub },
  { "Java_sun_awt_windows_WGraphics_setXORMode_stub", Java_sun_awt_windows_WGraphics_setXORMode_stub },
  { "Java_sun_awt_windows_WGraphics_getClipRect_stub", Java_sun_awt_windows_WGraphics_getClipRect_stub },
  { "Java_sun_awt_windows_WGraphics_clipRect_stub", Java_sun_awt_windows_WGraphics_clipRect_stub },
  { "Java_sun_awt_windows_WGraphics_clearRect_stub", Java_sun_awt_windows_WGraphics_clearRect_stub },
  { "Java_sun_awt_windows_WGraphics_fillRect_stub", Java_sun_awt_windows_WGraphics_fillRect_stub },
  { "Java_sun_awt_windows_WGraphics_drawRect_stub", Java_sun_awt_windows_WGraphics_drawRect_stub },
  { "Java_sun_awt_windows_WGraphics_drawString_stub", Java_sun_awt_windows_WGraphics_drawString_stub },
  { "Java_sun_awt_windows_WGraphics_drawChars_stub", Java_sun_awt_windows_WGraphics_drawChars_stub },
  { "Java_sun_awt_windows_WGraphics_drawBytes_stub", Java_sun_awt_windows_WGraphics_drawBytes_stub },
  { "Java_sun_awt_windows_WGraphics_drawStringWidth_stub", Java_sun_awt_windows_WGraphics_drawStringWidth_stub },
  { "Java_sun_awt_windows_WGraphics_drawCharsWidth_stub", Java_sun_awt_windows_WGraphics_drawCharsWidth_stub },
  { "Java_sun_awt_windows_WGraphics_drawBytesWidth_stub", Java_sun_awt_windows_WGraphics_drawBytesWidth_stub },
  { "Java_sun_awt_windows_WGraphics_drawLine_stub", Java_sun_awt_windows_WGraphics_drawLine_stub },
  { "Java_sun_awt_windows_WGraphics_copyArea_stub", Java_sun_awt_windows_WGraphics_copyArea_stub },
  { "Java_sun_awt_windows_WGraphics_drawRoundRect_stub", Java_sun_awt_windows_WGraphics_drawRoundRect_stub },
  { "Java_sun_awt_windows_WGraphics_fillRoundRect_stub", Java_sun_awt_windows_WGraphics_fillRoundRect_stub },
  { "Java_sun_awt_windows_WGraphics_drawPolygon_stub", Java_sun_awt_windows_WGraphics_drawPolygon_stub },
  { "Java_sun_awt_windows_WGraphics_fillPolygon_stub", Java_sun_awt_windows_WGraphics_fillPolygon_stub },
  { "Java_sun_awt_windows_WGraphics_drawOval_stub", Java_sun_awt_windows_WGraphics_drawOval_stub },
  { "Java_sun_awt_windows_WGraphics_fillOval_stub", Java_sun_awt_windows_WGraphics_fillOval_stub },
  { "Java_sun_awt_windows_WGraphics_drawArc_stub", Java_sun_awt_windows_WGraphics_drawArc_stub },
  { "Java_sun_awt_windows_WGraphics_fillArc_stub", Java_sun_awt_windows_WGraphics_fillArc_stub },
  { "Java_sun_awt_image_ImageRepresentation_offscreenInit_stub", Java_sun_awt_image_ImageRepresentation_offscreenInit_stub },
  { "Java_sun_awt_image_ImageRepresentation_setBytePixels_stub", Java_sun_awt_image_ImageRepresentation_setBytePixels_stub },
  { "Java_sun_awt_image_ImageRepresentation_setIntPixels_stub", Java_sun_awt_image_ImageRepresentation_setIntPixels_stub },
  { "Java_sun_awt_image_ImageRepresentation_finish_stub", Java_sun_awt_image_ImageRepresentation_finish_stub },
  { "Java_sun_awt_image_ImageRepresentation_imageDraw_stub", Java_sun_awt_image_ImageRepresentation_imageDraw_stub },
  { "Java_sun_awt_image_ImageRepresentation_disposeImage_stub", Java_sun_awt_image_ImageRepresentation_disposeImage_stub },
  { "Java_sun_awt_image_OffScreenImageSource_sendPixels_stub", Java_sun_awt_image_OffScreenImageSource_sendPixels_stub }
};

#endif

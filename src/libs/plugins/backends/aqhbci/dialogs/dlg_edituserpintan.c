/***************************************************************************
 begin       : Thu Jul 08 2010
 copyright   : (C) 2024 by Martin Preuss
 email       : martin@aqbanking.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "dlg_edituserpintan_p.h"
#include "banking/provider_l.h"
#include "aqbanking/i18n_l.h"

#include "aqhbci/banking/user.h"
#include "aqhbci/banking/provider.h"
#include "aqhbci/banking/provider_online.h"

#include <aqbanking/backendsupport/user.h>
#include <aqbanking/banking_be.h>
#include <aqbanking/dialogs/dlg_selectbankinfo.h>

#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/text.h>



/* ------------------------------------------------------------------------------------------------
 * defines, types
 * ------------------------------------------------------------------------------------------------
 */

#define DIALOG_MINWIDTH  200
#define DIALOG_MINHEIGHT 200

/* for improved readability */
#define DLG_WITHPROGRESS 1
#define DLG_UMOUNT       0
#define DLG_DIALOGFILE   "aqbanking/backends/aqhbci/dialogs/dlg_edituserpintan.dlg"


typedef int (*_DIALOG_SIGNAL_HANDLER_FN)(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
typedef struct _DIALOG_SIGNAL_ENTRY _DIALOG_SIGNAL_ENTRY;
struct _DIALOG_SIGNAL_ENTRY {
  const char *sender;
  GWEN_DIALOG_EVENTTYPE eventType;
  _DIALOG_SIGNAL_HANDLER_FN handlerFn;
};

typedef const char*(*_USER_GETCHARVALUE_FN)(const AB_USER *user);
typedef void (*_USER_SETCHARVALUE_FN)(AB_USER *user, const char *s);



/* ------------------------------------------------------------------------------------------------
 * forward declarations
 * ------------------------------------------------------------------------------------------------
 */


static void GWENHYWFAR_CB _freeData(void *bp, void *p);

static int GWENHYWFAR_CB _dlgApi_signalHandler(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);

static int _saveUser(GWEN_DIALOG *dlg);

static int _createTanMethodString(const AH_TAN_METHOD *tm, GWEN_BUFFER *tbuf);
static void _tanMethodsComboRebuild(GWEN_DIALOG *dlg);
static void _removeAllSpaces(uint8_t *s);
static int _fromGui(GWEN_DIALOG *dlg, AB_USER *u, int quiet);
static int _handleInit(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleFini(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedBankCode(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedOk(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedApply(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedReject(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedGetCert(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedGetSysId(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedGetBankInfo(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedGetItanModes(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleActivatedGetAccounts(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);
static int _handleValueChanged(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender);

static void _setModified(GWEN_DIALOG *dlg, int enabled);

static void _toGui(GWEN_DIALOG *dlg, const AB_USER *user);

/*static int _tanMethodsComboFindMethodById(GWEN_DIALOG *dlg, const char *widgetName, int id);*/
/*static void _tanMethodsComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int id);*/
static int _tanMethodsComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName);

static void _tanMechanismComboSetup(GWEN_DIALOG *dlg, const char *widgetName);
static void _tanMechanismComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int t);
static int _tanMechanismComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName);

static void _hbciVersionComboSetup(GWEN_DIALOG *dlg, const char *widgetName);
static void _hbciVersionComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int v);
static int _hbciVersionComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName);

static void _httpVersionComboSetup(GWEN_DIALOG *dlg, const char *widgetName);
static void _httpVersionComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int v);
static int _httpVersionComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName);

static void _userToGuiText(GWEN_DIALOG *dlg, const char *widgetName, const AB_USER *user, _USER_GETCHARVALUE_FN fn);
static int _guiTextToUserDeleSpaces(GWEN_DIALOG *dlg, const char *widgetName,
                                    AB_USER *user, _USER_SETCHARVALUE_FN fn,
                                    const char *errMsgIfMissing);
static int _guiTextToUserKeepSpaces(GWEN_DIALOG *dlg, const char *widgetName,
                                    AB_USER *user, _USER_SETCHARVALUE_FN fn,
                                    const char *errMsgIfMissing);

static void _userFlagsToGui(GWEN_DIALOG *dlg, uint32_t flags);
static uint32_t _userFlagsFromGui(GWEN_DIALOG *dlg);



/* ------------------------------------------------------------------------------------------------
 * static vars
 * ------------------------------------------------------------------------------------------------
 */


GWEN_INHERIT(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG)

static _DIALOG_SIGNAL_ENTRY _signalMap[]={
  {NULL,                  GWEN_DialogEvent_TypeInit,         _handleInit},
  {NULL,                  GWEN_DialogEvent_TypeFini,         _handleFini},
  {"bankCodeButton",      GWEN_DialogEvent_TypeActivated,    _handleActivatedBankCode},
  {"getCertButton",       GWEN_DialogEvent_TypeActivated,    _handleActivatedGetCert},
  {"getBankInfoButton",   GWEN_DialogEvent_TypeActivated,    _handleActivatedGetBankInfo},
  {"getSysIdButton",      GWEN_DialogEvent_TypeActivated,    _handleActivatedGetSysId},
  {"getItanModesButton",  GWEN_DialogEvent_TypeActivated,    _handleActivatedGetItanModes},
  {"getAccountsButton",   GWEN_DialogEvent_TypeActivated,    _handleActivatedGetAccounts},

  {"userNameEdit",        GWEN_DialogEvent_TypeValueChanged, _handleValueChanged},
  {"bankCodeEdit",        GWEN_DialogEvent_TypeValueChanged, _handleValueChanged},
  {"userIdEdit",          GWEN_DialogEvent_TypeValueChanged, _handleValueChanged},
  {"customerIdEdit",      GWEN_DialogEvent_TypeValueChanged, _handleValueChanged},
  {"tanMediumIdEdit",     GWEN_DialogEvent_TypeValueChanged, _handleValueChanged},
  {"urlEdit",             GWEN_DialogEvent_TypeValueChanged, _handleValueChanged},
  {"hbciVersionCombo",    GWEN_DialogEvent_TypeActivated,    _handleValueChanged},
  {"httpVersionCombo",    GWEN_DialogEvent_TypeActivated,    _handleValueChanged},
  {"tanMethodCombo",      GWEN_DialogEvent_TypeActivated,    _handleValueChanged},
  {"tanMechanismCombo",   GWEN_DialogEvent_TypeActivated,    _handleValueChanged},
  {"noBase64Check",       GWEN_DialogEvent_TypeActivated,    _handleValueChanged},
  {"omitSmsAccountCheck", GWEN_DialogEvent_TypeActivated,    _handleValueChanged},

  {"okButton",            GWEN_DialogEvent_TypeActivated,    _handleActivatedOk},
  {"applyButton",         GWEN_DialogEvent_TypeActivated,    _handleActivatedApply},
  {"abortButton",         GWEN_DialogEvent_TypeActivated,    _handleActivatedReject},

  {NULL, 0, NULL}
};



/* ------------------------------------------------------------------------------------------------
 * implementations
 * ------------------------------------------------------------------------------------------------
 */

GWEN_DIALOG *AH_EditUserPinTanDialog_new(AB_PROVIDER *pro, AB_USER *u, int doLock)
{
  GWEN_DIALOG *dlg;
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;

  dlg=GWEN_Dialog_CreateAndLoadWithPath("ah_edit_user_pintan", AB_PM_LIBNAME, AB_PM_DATADIR, DLG_DIALOGFILE);
  if (dlg==NULL) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here");
    return NULL;
  }

  GWEN_NEW_OBJECT(AH_EDIT_USER_PINTAN_DIALOG, xdlg);
  GWEN_INHERIT_SETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg, xdlg, _freeData);
  GWEN_Dialog_SetSignalHandler(dlg, _dlgApi_signalHandler);

  /* preset */
  xdlg->provider=pro;
  xdlg->banking=AB_Provider_GetBanking(pro);
  xdlg->user=u;
  xdlg->doLock=doLock;

  /* done */
  return dlg;
}



void GWENHYWFAR_CB _freeData(void *bp, void *p)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;

  xdlg=(AH_EDIT_USER_PINTAN_DIALOG *) p;

  GWEN_FREE_OBJECT(xdlg);
}



int _handleInit(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  GWEN_DB_NODE *dbPrefs;
  int i;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  dbPrefs=GWEN_Dialog_GetPreferences(dlg);

  /* init */
  GWEN_Dialog_SetCharProperty(dlg, "", GWEN_DialogProperty_Title, 0, I18N("Edit User"), 0);

  _tanMechanismComboSetup(dlg, "tanMechanismCombo");

  _hbciVersionComboSetup(dlg, "hbciVersionCombo");
  _httpVersionComboSetup(dlg, "httpVersionCombo");

  GWEN_Dialog_SetCharProperty(dlg, "tanMediumIdEdit", GWEN_DialogProperty_ToolTip, 0,
                              I18N("For smsTAN or mTAN this is your mobile phone number. "
                                   "Please ask your bank for the necessary format of this number."),
                              0);

  _toGui(dlg, xdlg->user);

  /* read width */
  i=GWEN_DB_GetIntValue(dbPrefs, "dialog_width", 0, -1);
  if (i>=DIALOG_MINWIDTH)
    GWEN_Dialog_SetIntProperty(dlg, "", GWEN_DialogProperty_Width, 0, i, 0);

  /* read height */
  i=GWEN_DB_GetIntValue(dbPrefs, "dialog_height", 0, -1);
  if (i>=DIALOG_MINHEIGHT)
    GWEN_Dialog_SetIntProperty(dlg, "", GWEN_DialogProperty_Height, 0, i, 0);

  return GWEN_DialogEvent_ResultHandled;
}



void _toGui(GWEN_DIALOG *dlg, const AB_USER *user)
{
  const GWEN_URL *gu;

  _tanMethodsComboRebuild(dlg);

  _userToGuiText(dlg, "userNameEdit",    user, AB_User_GetUserName);
  _userToGuiText(dlg, "bankCodeEdit",    user, AB_User_GetBankCode);
  _userToGuiText(dlg, "userIdEdit",      user, AB_User_GetUserId);
  _userToGuiText(dlg, "customerIdEdit",  user, AB_User_GetCustomerId);
  _userToGuiText(dlg, "tanMediumIdEdit", user, AH_User_GetTanMediumId);

  _tanMechanismComboSetCurrent(dlg, "tanMechanismCombo", AH_User_GetSelectedTanInputMechanism(user));
  _hbciVersionComboSetCurrent(dlg, "hbciVersionCombo", AH_User_GetHbciVersion(user));
  _httpVersionComboSetCurrent(dlg, "httpVersionCombo", ((AH_User_GetHttpVMajor(user))<<8)+AH_User_GetHttpVMinor(user));

  _userFlagsToGui(dlg, AH_User_GetFlags(user));

  gu=AH_User_GetServerUrl(user);
  if (gu) {
    GWEN_BUFFER *tbuf;

    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Url_toString(gu, tbuf);
    GWEN_Dialog_SetCharProperty(dlg, "urlEdit", GWEN_DialogProperty_Value, 0, GWEN_Buffer_GetStart(tbuf), 0);
    GWEN_Buffer_free(tbuf);
  }
  _setModified(dlg, 0);
}



int _fromGui(GWEN_DIALOG *dlg, AB_USER *u, int quiet)
{
  const char *s;
  int i;

  if (_guiTextToUserKeepSpaces(dlg, "userNameEdit",    u, AB_User_SetUserName,    NULL)<0 ||
      _guiTextToUserDeleSpaces(dlg, "bankCodeEdit",    u, AB_User_SetBankCode,    NULL)<0 ||
      _guiTextToUserKeepSpaces(dlg, "userIdEdit",      u, AB_User_SetUserId,      quiet?NULL:I18N("Missing user id"))<0 ||
      _guiTextToUserKeepSpaces(dlg, "customerIdEdit",  u, AB_User_SetCustomerId,  NULL)<0 ||
      _guiTextToUserKeepSpaces(dlg, "tanMediumIdEdit", u, AH_User_SetTanMediumId, NULL)<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here");
    return GWEN_ERROR_INVALID;
  }

  if (u)
    AB_User_SetCountry(u, "de");

  AH_User_SetHbciVersion(u, _hbciVersionComboGetCurrent(dlg, "hbciVersionCombo"));

  i=_httpVersionComboGetCurrent(dlg, "httpVersionCombo");
  AH_User_SetHttpVMajor(u, ((i>>8) & 0xff));
  AH_User_SetHttpVMinor(u, (i & 0xff));

  AH_User_SetSelectedTanInputMechanism(u, _tanMechanismComboGetCurrent(dlg, "tanMechanismCombo"));

  i=_tanMethodsComboGetCurrent(dlg, "tanMethodCombo");
  if (i>0) {
    AH_User_SetSelectedTanMethod(u, i);
  }
  else {
    if (!quiet) {
      GWEN_Gui_ShowError(I18N("Error on Input"), "%s", I18N("Please select tan method."));
      GWEN_Dialog_SetIntProperty(dlg, "tanMethodCombo", GWEN_DialogProperty_Focus, 0, 1, 0);
      DBG_INFO(AQHBCI_LOGDOMAIN, "Missing tan method");
      return GWEN_ERROR_INVALID;
    }
  }

  s=GWEN_Dialog_GetCharProperty(dlg, "urlEdit", GWEN_DialogProperty_Value, 0, NULL);
  if (s && *s) {
    GWEN_BUFFER *tbuf;
    GWEN_URL *gu;

    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(tbuf, s);
    GWEN_Text_CondenseBuffer(tbuf);
    _removeAllSpaces((uint8_t *)GWEN_Buffer_GetStart(tbuf));
    gu=GWEN_Url_fromString(GWEN_Buffer_GetStart(tbuf));
    if (gu==NULL) {
      if (!quiet) {
        GWEN_Gui_ShowError(I18N("Error"), "%s", I18N("Invalid URL"));
        GWEN_Buffer_free(tbuf);
        return GWEN_ERROR_BAD_DATA;
      }
    }
    if (u)
      AH_User_SetServerUrl(u, gu);
    GWEN_Url_free(gu);
    GWEN_Buffer_free(tbuf);
  }

  AH_User_SetFlags(u, _userFlagsFromGui(dlg));

  return 0;
}



int _handleFini(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  int i;
  GWEN_DB_NODE *dbPrefs;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  dbPrefs=GWEN_Dialog_GetPreferences(dlg);

  /* store dialog width */
  i=GWEN_Dialog_GetIntProperty(dlg, "", GWEN_DialogProperty_Width, 0, -1);
  GWEN_DB_SetIntValue(dbPrefs, GWEN_DB_FLAGS_OVERWRITE_VARS, "dialog_width", i);

  /* store dialog height */
  i=GWEN_Dialog_GetIntProperty(dlg, "", GWEN_DialogProperty_Height, 0, -1);
  GWEN_DB_SetIntValue(dbPrefs, GWEN_DB_FLAGS_OVERWRITE_VARS, "dialog_height", i);

  return GWEN_DialogEvent_ResultHandled;
}



int _handleActivatedBankCode(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  GWEN_DIALOG *dlg2;
  int rv;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  dlg2=AB_SelectBankInfoDialog_new(xdlg->banking, "de", NULL);
  if (dlg2==NULL) {
    GWEN_Gui_ShowError(I18N("Error"), "%s", I18N("Could create dialog, maybe incomplete installation?"));
    return GWEN_DialogEvent_ResultHandled;
  }

  rv=GWEN_Gui_ExecDialog(dlg2, 0);
  if (rv==0) {
    /* rejected */
    GWEN_Dialog_free(dlg2);
    return GWEN_DialogEvent_ResultHandled;
  }
  else {
    const AB_BANKINFO *bi;

    bi=AB_SelectBankInfoDialog_GetSelectedBankInfo(dlg2);
    if (bi) {
      const char *s;

      s=AB_BankInfo_GetBankId(bi);
      GWEN_Dialog_SetCharProperty(dlg, "bankCodeEdit", GWEN_DialogProperty_Value, 0, (s && *s)?s:"", 0);
    }
  }
  GWEN_Dialog_free(dlg2);

  return GWEN_DialogEvent_ResultHandled;
}



int _handleActivatedOk(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  int rv;

  rv=_saveUser(dlg);
  if (rv==0)
    return GWEN_DialogEvent_ResultAccept;
  return GWEN_DialogEvent_ResultHandled;
}



int _handleActivatedApply(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  int rv;

  rv=_saveUser(dlg);
  if (rv==0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
  }
  return GWEN_DialogEvent_ResultHandled;
}



int _saveUser(GWEN_DIALOG *dlg)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  int rv;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  rv=_fromGui(dlg, NULL, 0);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  if (xdlg->doLock) {
    int rv;

    rv=AB_Provider_BeginExclUseUser(xdlg->provider, xdlg->user);
    if (rv<0) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      GWEN_Gui_ShowError(I18N("Error"), "%s", I18N("Unable to lock user. Maybe already in use?"));
      return rv;
    }
  }

  _fromGui(dlg, xdlg->user, 1);

  if (xdlg->doLock) {
    int rv;

    rv=AB_Provider_EndExclUseUser(xdlg->provider, xdlg->user, 0);
    if (rv<0) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      GWEN_Gui_ShowError(I18N("Error"), "%s", I18N("Unable to unlock user."));
      AB_Provider_EndExclUseUser(xdlg->provider, xdlg->user, 1);
      return rv;
    }
  }

  return 0;
}



int _handleActivatedReject(GWEN_DIALOG *dlg, GWEN_UNUSED GWEN_DIALOG_EVENTTYPE t, GWEN_UNUSED const char *sender)
{
  return GWEN_DialogEvent_ResultReject;
}



int _handleActivatedGetCert(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  int rv;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  if (xdlg->modified) {
    GWEN_Gui_ShowError(I18N("User Modified"), "%s", I18N("Please apply current changes first."));
    return GWEN_DialogEvent_ResultHandled;
  }

  rv=AH_Provider_GetCert(xdlg->provider, xdlg->user, DLG_WITHPROGRESS, DLG_UMOUNT, xdlg->doLock);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
  }

  return GWEN_DialogEvent_ResultHandled;
}



int _handleActivatedGetSysId(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  int rv;
  AB_IMEXPORTER_CONTEXT *ctx;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  if (xdlg->modified) {
    GWEN_Gui_ShowError(I18N("User Modified"), "%s", I18N("Please apply current changes first."));
    return GWEN_DialogEvent_ResultHandled;
  }

  ctx=AB_ImExporterContext_new();
  rv=AH_Provider_GetSysId(xdlg->provider, xdlg->user, ctx, DLG_WITHPROGRESS, DLG_UMOUNT, xdlg->doLock);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
  }

  _toGui(dlg, xdlg->user);

  AB_ImExporterContext_free(ctx);
  return GWEN_DialogEvent_ResultHandled;
}



int _handleActivatedGetBankInfo(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  int rv;
  AB_IMEXPORTER_CONTEXT *ctx;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  if (xdlg->modified) {
    GWEN_Gui_ShowError(I18N("User Modified"), "%s", I18N("Please apply current changes first."));
    return GWEN_DialogEvent_ResultHandled;
  }

  ctx=AB_ImExporterContext_new();
  rv=AH_Provider_GetBankInfo(xdlg->provider,
                             xdlg->user,
                             ctx,
                             0,   /* without TAN segment, maybe later add button for call with TAN segment */
                             DLG_WITHPROGRESS,
                             DLG_UMOUNT,
                             xdlg->doLock);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
  }

  _toGui(dlg, xdlg->user);

  AB_ImExporterContext_free(ctx);
  return GWEN_DialogEvent_ResultHandled;
}



int _handleActivatedGetItanModes(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  int rv;
  AB_IMEXPORTER_CONTEXT *ctx;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  if (xdlg->modified) {
    GWEN_Gui_ShowError(I18N("User Modified"), "%s", I18N("Please apply current changes first."));
    return GWEN_DialogEvent_ResultHandled;
  }

  ctx=AB_ImExporterContext_new();
  rv=AH_Provider_GetItanModes(xdlg->provider, xdlg->user, ctx, DLG_WITHPROGRESS, DLG_UMOUNT, xdlg->doLock);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
  }

  _toGui(dlg, xdlg->user);

  AB_ImExporterContext_free(ctx);
  return GWEN_DialogEvent_ResultHandled;
}



int _handleActivatedGetAccounts(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  int rv;
  AB_IMEXPORTER_CONTEXT *ctx;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  if (xdlg->modified) {
    GWEN_Gui_ShowError(I18N("User Modified"), "%s", I18N("Please apply current changes first."));
    return GWEN_DialogEvent_ResultHandled;
  }

  ctx=AB_ImExporterContext_new();
  rv=AH_Provider_GetAccounts(xdlg->provider, xdlg->user, ctx, DLG_WITHPROGRESS, DLG_UMOUNT, xdlg->doLock);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
  }

  _toGui(dlg, xdlg->user);

  AB_ImExporterContext_free(ctx);
  return GWEN_DialogEvent_ResultHandled;
}



int _handleValueChanged(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  _setModified(dlg, 1);
  return GWEN_DialogEvent_ResultHandled;
}



void _setModified(GWEN_DIALOG *dlg, int enabled)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  xdlg->modified=enabled;
  GWEN_Dialog_SetIntProperty(dlg, "applyButton", GWEN_DialogProperty_Enabled, 0, enabled, 0);
}



int GWENHYWFAR_CB _dlgApi_signalHandler(GWEN_DIALOG *dlg, GWEN_DIALOG_EVENTTYPE t, const char *sender)
{
  const _DIALOG_SIGNAL_ENTRY *entry;

  entry=_signalMap;
  while(entry->handlerFn) {
    if (entry->eventType==t && (entry->sender==NULL || (sender && strcasecmp(sender, entry->sender)==0))) {
      return entry->handlerFn(dlg, t, sender);
    }
    entry++;
  }

  return GWEN_DialogEvent_ResultNotHandled;
}






#if 0
int _tanMethodsComboFindMethodById(GWEN_DIALOG *dlg, const char *widgetName, int id)
{
  int idx;

  for (idx=0; ; idx++) {
    const char *s;

    s=GWEN_Dialog_GetCharProperty(dlg, widgetName, GWEN_DialogProperty_Value, idx, NULL);
    if (s && *s) {
      int currentId;

      if (1==sscanf(s, "%u", &currentId) && currentId==id)
        return idx;
    }
    else
      break;
  } /* for */

  return -1;
}



void _tanMethodsComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int id)
{
  if (id) {
    int idx;

    idx=_tanMethodsComboFindMethodById(dlg, widgetName, id);
    if (idx>=0)
      GWEN_Dialog_SetIntProperty(dlg, widgetName, GWEN_DialogProperty_Value, 0, idx, 0);
  }
}
#endif



int _tanMethodsComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName)
{
  int idx;

  idx=GWEN_Dialog_GetIntProperty(dlg, widgetName,  GWEN_DialogProperty_Value, 0, -1);
  if (idx>=0) {
    const char *s;

    s=GWEN_Dialog_GetCharProperty(dlg, widgetName, GWEN_DialogProperty_Value, idx, NULL);
    if (s && *s) {
      int currentId;

      if (1==sscanf(s, "%u", &currentId))
        return currentId;
    }
  }

  return 0;
}


void _tanMethodsComboRebuild(GWEN_DIALOG *dlg)
{
  AH_EDIT_USER_PINTAN_DIALOG *xdlg;
  const AH_TAN_METHOD_LIST *ctl;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AH_EDIT_USER_PINTAN_DIALOG, dlg);
  assert(xdlg);

  ctl=AH_User_GetTanMethodDescriptions(xdlg->user);

  /* setup tanmethod combo */
  GWEN_Dialog_SetIntProperty(dlg, "tanMethodCombo", GWEN_DialogProperty_ClearValues, 0, 0, 0);
  GWEN_Dialog_SetCharProperty(dlg, "tanMethodCombo", GWEN_DialogProperty_AddValue, 0, I18N("-- select --"), 0);
  if (ctl) {
    AH_TAN_METHOD *tm;
    GWEN_BUFFER *tbuf;
    int i;
    int idx;
    int selectedMethod;
    int tjv;
    int tfn;

    selectedMethod=AH_User_GetSelectedTanMethod(xdlg->user);
    tjv=selectedMethod / 1000;
    tfn=selectedMethod % 1000;
    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    idx=-1;
    i=1;
    tm=AH_TanMethod_List_First(ctl);
    while (tm) {
      if (_createTanMethodString(tm, tbuf)==0) {
        if (AH_TanMethod_GetFunction(tm)==tfn && AH_TanMethod_GetGvVersion(tm)==tjv)
          idx=i;
        GWEN_Dialog_SetCharProperty(dlg, "tanMethodCombo", GWEN_DialogProperty_AddValue, 0, GWEN_Buffer_GetStart(tbuf), 0);
        i++;
      }
      GWEN_Buffer_Reset(tbuf);

      tm=AH_TanMethod_List_Next(tm);
    }
    GWEN_Buffer_free(tbuf);
    if (idx>=0)
      /* chooses selected entry in combo box */
      GWEN_Dialog_SetIntProperty(dlg, "tanMethodCombo", GWEN_DialogProperty_Value, 0, idx, 0);
  }
}



int _createTanMethodString(const AH_TAN_METHOD *tm, GWEN_BUFFER *tbuf)
{
  const char *s;

  GWEN_Buffer_AppendArgs(tbuf, "%d - ", (AH_TanMethod_GetGvVersion(tm)*1000)+AH_TanMethod_GetFunction(tm));
  GWEN_Buffer_AppendArgs(tbuf, "%d", AH_TanMethod_GetFunction(tm));

  s=AH_TanMethod_GetMethodName(tm);
  if (!(s && *s))
    s=AH_TanMethod_GetMethodId(tm);
  if (s && *s)
    GWEN_Buffer_AppendArgs(tbuf, " - %s", s);

  /* add HKTAN version */
  GWEN_Buffer_AppendArgs(tbuf, " (Version %d)", AH_TanMethod_GetGvVersion(tm));

  return 0;
}



void _tanMechanismComboSetup(GWEN_DIALOG *dlg, const char *widgetName)
{
  const GWEN_DIALOG_PROPERTY addValue=GWEN_DialogProperty_AddValue;
  const GWEN_DIALOG_PROPERTY clrValue=GWEN_DialogProperty_ClearValues;
  const GWEN_DIALOG_PROPERTY toolTip=GWEN_DialogProperty_ToolTip;

  GWEN_Dialog_SetCharProperty(dlg, widgetName, toolTip, 0,
                              I18N("Please only change this value if you know what you are doing, otherwise leave it at \"auto\"."),
                              0);
  GWEN_Dialog_SetIntProperty(dlg,  widgetName, clrValue, 0, 0, 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("tanMechanism|auto"), 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("tanMechanism|text"), 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("tanMechanism|chipTAN optic"), 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("tanMechanism|QR image"), 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("tanMechanism|photoTAN"), 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("tanMechanism|chipTAN USB"), 0);
}



void _tanMechanismComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int t)
{
  const GWEN_DIALOG_PROPERTY setValue=GWEN_DialogProperty_Value;

  switch (t) {
  case AB_BANKING_TANMETHOD_TEXT:          GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 1, 0); break;
  case AB_BANKING_TANMETHOD_CHIPTAN_OPTIC: GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 2, 0); break;
  case AB_BANKING_TANMETHOD_CHIPTAN_QR:    GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 3, 0); break;
  case AB_BANKING_TANMETHOD_PHOTOTAN:      GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 4, 0); break;
  case AB_BANKING_TANMETHOD_CHIPTAN_USB:   GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 5, 0); break;
  default:
    DBG_WARN(AQHBCI_LOGDOMAIN, "Using default tanMechanism");
    GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 0, 0);
    break;
  }
}



int _tanMechanismComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName)
{
  int idx;

  idx=GWEN_Dialog_GetIntProperty(dlg, widgetName, GWEN_DialogProperty_Value, 0, -1);
  switch(idx) {
    case 1:  return AB_BANKING_TANMETHOD_TEXT;
    case 2:  return AB_BANKING_TANMETHOD_CHIPTAN_OPTIC;
    case 3:  return AB_BANKING_TANMETHOD_CHIPTAN_QR;
    case 4:  return AB_BANKING_TANMETHOD_PHOTOTAN;
    case 5:  return AB_BANKING_TANMETHOD_CHIPTAN_USB;
    default: return AB_BANKING_TANMETHOD_TEXT;
  }
}



void _hbciVersionComboSetup(GWEN_DIALOG *dlg, const char *widgetName)
{
  const GWEN_DIALOG_PROPERTY addValue=GWEN_DialogProperty_AddValue;
  const GWEN_DIALOG_PROPERTY clrValue=GWEN_DialogProperty_ClearValues;

  GWEN_Dialog_SetIntProperty(dlg,  widgetName, clrValue, 0, 0, 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("-- select --"), 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, "2.20", 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, "3.0", 0);
}



void _hbciVersionComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int v)
{
  const GWEN_DIALOG_PROPERTY setValue=GWEN_DialogProperty_Value;

  switch (v) {
    case 220: GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 1, 0); break;
    case 300: GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 2, 0); break;
    default:  GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 2, 0); break;
  }
}



int _hbciVersionComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName)
{
  int idx;

  idx=GWEN_Dialog_GetIntProperty(dlg, widgetName, GWEN_DialogProperty_Value, 0, -1);
  switch(idx) {
    case 1:  return 220;
    case 2:  return 300;
    default: return 300;
  }
}





void _httpVersionComboSetup(GWEN_DIALOG *dlg, const char *widgetName)
{
  const GWEN_DIALOG_PROPERTY addValue=GWEN_DialogProperty_AddValue;
  const GWEN_DIALOG_PROPERTY clrValue=GWEN_DialogProperty_ClearValues;

  GWEN_Dialog_SetIntProperty(dlg,  widgetName, clrValue, 0, 0, 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, I18N("-- select --"), 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, "1.0", 0);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, addValue, 0, "1.1", 0);
}



void _httpVersionComboSetCurrent(GWEN_DIALOG *dlg, const char *widgetName, int v)
{
  const GWEN_DIALOG_PROPERTY setValue=GWEN_DialogProperty_Value;

  switch (v) {
    case 0x0100: GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 1, 0); break;
    case 0x0101: GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 2, 0); break;
    default:     GWEN_Dialog_SetIntProperty(dlg, widgetName, setValue, 0, 2, 0); break;
  }
}



int _httpVersionComboGetCurrent(GWEN_DIALOG *dlg, const char *widgetName)
{
  int idx;

  idx=GWEN_Dialog_GetIntProperty(dlg, widgetName, GWEN_DialogProperty_Value, 0, -1);
  switch(idx) {
    case 1:  return 0x0100;
    case 2:  return 0x0101;
    default: return 0x0101;
  }
}



void _userFlagsToGui(GWEN_DIALOG *dlg, uint32_t flags)
{
  const GWEN_DIALOG_PROPERTY setValue=GWEN_DialogProperty_Value;

  GWEN_Dialog_SetIntProperty(dlg, "noBase64Check", setValue, 0, (flags & AH_USER_FLAGS_NO_BASE64)?1:0, 0);
  GWEN_Dialog_SetIntProperty(dlg, "omitSmsAccountCheck", setValue, 0, (flags & AH_USER_FLAGS_TAN_OMIT_SMS_ACCOUNT)?1:0, 0);
}



uint32_t _userFlagsFromGui(GWEN_DIALOG *dlg)
{
  uint32_t flags=0;

  if (GWEN_Dialog_GetIntProperty(dlg, "noBase64Check", GWEN_DialogProperty_Value, 0, 0))
    flags|=AH_USER_FLAGS_NO_BASE64;
  if (GWEN_Dialog_GetIntProperty(dlg, "omitSmsAccountCheck", GWEN_DialogProperty_Value, 0, 0))
    flags|=AH_USER_FLAGS_TAN_OMIT_SMS_ACCOUNT;

  return flags;
}




void _userToGuiText(GWEN_DIALOG *dlg, const char *widgetName, const AB_USER *user, _USER_GETCHARVALUE_FN fn)
{
  const char *s;

  s=fn(user);
  GWEN_Dialog_SetCharProperty(dlg, widgetName, GWEN_DialogProperty_Value, 0, s?s:"", 0);
}



int _guiTextToUserDeleSpaces(GWEN_DIALOG *dlg, const char *widgetName,
                             AB_USER *user, _USER_SETCHARVALUE_FN fn,
                             const char *errMsgIfMissing)
{
  const char *s;

  s=GWEN_Dialog_GetCharProperty(dlg, widgetName, GWEN_DialogProperty_Value, 0, NULL);
  if (s && *s) {
    GWEN_BUFFER *tbuf;

    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(tbuf, s);
    GWEN_Text_CondenseBuffer(tbuf);
    _removeAllSpaces((uint8_t *)GWEN_Buffer_GetStart(tbuf));
    if (user)
      fn(user, GWEN_Buffer_GetStart(tbuf));
    GWEN_Buffer_free(tbuf);
  }
  else {
    DBG_ERROR(NULL, "Missing input from widget %s", widgetName);
    if (errMsgIfMissing) {
      GWEN_Gui_ShowError(I18N("Error on Input"), "%s", errMsgIfMissing);
      GWEN_Dialog_SetIntProperty(dlg, widgetName, GWEN_DialogProperty_Focus, 0, 1, 0);
      return GWEN_ERROR_INVALID;
    }
    if (user)
      fn(user, NULL);
  }
  return 0;
}



int _guiTextToUserKeepSpaces(GWEN_DIALOG *dlg, const char *widgetName,
                             AB_USER *user, _USER_SETCHARVALUE_FN fn,
                             const char *errMsgIfMissing)
{
  const char *s;

  s=GWEN_Dialog_GetCharProperty(dlg, widgetName, GWEN_DialogProperty_Value, 0, NULL);
  if (s && *s) {
    GWEN_BUFFER *tbuf;

    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(tbuf, s);
    GWEN_Text_CondenseBuffer(tbuf);
    if (user)
      fn(user, GWEN_Buffer_GetStart(tbuf));
    GWEN_Buffer_free(tbuf);
  }
  else {
    DBG_ERROR(NULL, "Missing input from widget %s", widgetName);
    if (errMsgIfMissing) {
      GWEN_Gui_ShowError(I18N("Error on Input"), "%s", errMsgIfMissing);
      GWEN_Dialog_SetIntProperty(dlg, widgetName, GWEN_DialogProperty_Focus, 0, 1, 0);
      return GWEN_ERROR_INVALID;
    }
    if (user)
      fn(user, NULL);
  }
  return 0;
}



void _removeAllSpaces(uint8_t *s)
{
  uint8_t *d;

  d=s;
  while (*s) {
    if (*s>33)
      *(d++)=*s;
    s++;
  }
  *d=0;
}





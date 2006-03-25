/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Mon Mar 01 2004
 copyright   : (C) 2004 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

/** @file 
 * @short The main interface of the aqbanking library
 */

#ifndef AQBANKING_BANKING_H
#define AQBANKING_BANKING_H

/** @addtogroup G_AB_BANKING Main Interface
 */
/*@{*/
/**
 * Object to be operated on by functions in this group (@ref AB_BANKING).
 */
typedef struct AB_BANKING AB_BANKING;
/*@}*/


#include <gwenhywfar/inherit.h>
#include <gwenhywfar/types.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/stringlist.h>
#include <gwenhywfar/plugindescr.h>

#include <aqbanking/error.h> /* for AQBANKING_API */
#include <aqbanking/version.h>

/* outsourced */
#include <aqbanking/banking_virt.h>
#include <aqbanking/banking_imex.h>
#include <aqbanking/banking_info.h>
#include <aqbanking/banking_ob.h>
#include <aqbanking/banking_simple.h>

#include <aqbanking/provider.h>

#define AB_PM_LIBNAME    "aqbanking"
#define AB_PM_SYSCONFDIR "sysconfdir"
#define AB_PM_DATADIR    "datadir"



#ifdef __cplusplus
extern "C" {
#endif



/** @addtogroup G_AB_BANKING Main Interface
 *
 * @short This group contains the main API function group.
 *
 * <p>
 * A program should first call @ref AB_Banking_Init to allow AqBanking
 * to load its configuration files and initialize itself.
 * </p>
 * After that you may call any other function of this group (most likely
 * the program will request a list of managed account via
 * @ref AB_Banking_GetAccounts).
 * </p>
 * <p>
 * When the program has finished its work it should call @ref AB_Banking_Fini
 * as the last function of AqBanking (just before calling
 * @ref AB_Banking_free).
 * </p>
 */
/*@{*/

/** @name Extensions supported by the application
 *
 */
/*@{*/
#define AB_BANKING_EXTENSION_NONE             0x00000000
/**
 * If this flag is set then the application allows nesting progress
 * dialogs. If the application does it might be best to use the toplevel
 * progress widget and just add a progress bar to it.
 * If this flag is not set then AqBanking keeps track of the nesting level
 * and dismisses further calls to @ref AB_Banking_ProgressStart until
 * @ref AB_Banking_ProgressEnd has been called often enough to reset the
 * nesting counter. I.e. the application will not receive nesting calls
 * to @ref AB_Banking_ProgressStart.
 */
#define AB_BANKING_EXTENSION_NESTING_PROGRESS 0x00000001
/*@}*/


/**
 * This object is prepared to be inherited (using @ref GWEN_INHERIT_SETDATA).
 */
GWEN_INHERIT_FUNCTION_LIB_DEFS(AB_BANKING, AQBANKING_API)



/** @name Constructor, Destructor, Init, Fini
 *
 */
/*@{*/

/**
 * <p>
 * Creates an instance of AqBanking. Though AqBanking is quite object
 * oriented (and thus allows multiple instances of AB_BANKING to co-exist)
 * you should avoid having multiple AB_BANKING objects in parallel.
 * </p>
 * <p>
 * This is just because the backends are loaded dynamically and might not like
 * to be used with multiple instances of AB_BANKING in parallel.
 * </p>
 * <p>
 * You should later free this object using @ref AB_Banking_free.
 * </p>
 * <p>
 * This function does not actually load the configuration file or setup
 * AqBanking, that is performed by @ref AB_Banking_Init.
 * </p>
 * <p>
 * Please note that this function sets a new handler for incoming SSL
 * certificates using GWEN_NetTransportSSL_SetAskAddCertFn2(), so if you
 * want to handle this in your application yourself you must overwrite this
 * handler @b after calling this function here.
 * </p>
 * <p>
 * Please note: This function internally calls @ref AB_Banking_newExtended
 * with the value 0 for <i>extensions</i>. So if your program supports any
 * extension you should call @ref AB_Banking_newExtended instead of this
 * function.
 * </p>
 * @return new instance of AB_BANKING
 *
 * @param appName name of the application which wants to use AqBanking.
 * This allows AqBanking to separate settings and data for multiple
 * applications.
 *
 * @param dname Path for the directory containing the user data of
 * AqBanking. You should in most cases present a NULL for this
 * parameter, which means AqBanking will choose the default user
 * data folder which is "$HOME/.banking".  The configuration file
 * "settings.conf" file is searched for in this folder. NOTE:
 * Versions of AqBanking before 1.2.0.16 used this argument to
 * specify the path and name (!) of the configuration file,
 * whereas now this specifies only the path. It is now impossible
 * to specify the name; aqbanking will always use its default name
 * "settings.conf". For AqBanking < 1.2.0.16, the default
 * configuration file was "$HOME/.aqbanking.conf". This file is
 * now also searched for, but if it exists it will be moved to the
 * new default path and name upon AB_Banking_Fini. The new path
 * will be "$HOME/.banking/settings.conf".
 */
AQBANKING_API
AB_BANKING *AB_Banking_new(const char *appName, const char *dname);

/**
 * This does the same as @ref AB_Banking_new. In addition, the application
 * may state here the extensions it supports.
 * See @ref AB_BANKING_EXTENSION_NONE and following.
 * This is used to keep the number of callbacks to the application small
 * (otherswise whenever we add a flag which changes the expected behaviour
 * of a GUI callback we would have to introduce a new callback in order to
 * maintain binary compatibility).
 */
AQBANKING_API
AB_BANKING *AB_Banking_newExtended(const char *appName,
                                   const char *dname,
                                   GWEN_TYPE_UINT32 extensions);


/**
 * Destroys the given instance of AqBanking. Please note that if
 * @ref AB_Banking_Init has been called on this object then
 * @ref  AB_Banking_Fini should be called before this function.
 */
AQBANKING_API 
void AB_Banking_free(AB_BANKING *ab);

/**
 * Initializes AqBanking. This actually reads the configuration file,
 * thus loading account settings and backends as needed.
 * @return 0 if ok, error code otherwise (see @ref AB_ERROR)
 * @param ab banking interface
 */
AQBANKING_API 
int AB_Banking_Init(AB_BANKING *ab);

/**
 * Deinitializes AqBanking thus allowing it to save its data and to unload
 * backends.
 * @return 0 if ok, error code otherwise (see @ref AB_ERROR)
 * @param ab banking interface
 */
AQBANKING_API 
int AB_Banking_Fini(AB_BANKING *ab);

/**
 * Saves all data. You may call this function periodically (especially
 * after doing setup stuff).
 * @return 0 if ok, error code otherwise (see @ref AB_ERROR)
 * @param ab banking interface
 */
int AB_Banking_Save(AB_BANKING *ab);

/*@}*/



/** @name Working With Backends
 *
 * <p>
 * Working with backends - as far as the frontend is concerned - is very
 * simple.
 * </p>
 * <p>
 * An application typically does this upon initial setup:
 * <ul>
 *  <li>
 *    get a list of available backends (@ref AB_Banking_GetActiveProviders)
 *  </li>
 *  <li>
 *    present this list to the user, let him select which backend(s) to
 *    activate/deactivate
 *  </li>
 *  <li>
 *    activate (@ref AB_Banking_ActivateProvider) or
 *    deactivate (@ref AB_Banking_DeactivateProvider) the backends
 *    accordingly
 *  </li>
 *  <li>
 *    optionally: allow the user to setup a selected backend
 *    (@ref AB_Banking_FindWizard to get the required setup wizard) and
 *    then run that wizard)
 *  </li>
 * </ul>
 * </p>
 * <p>
 * Please note that for security reasons a backend is only used when
 * it has explicitly been activated. So if any backend is added to AqBanking
 * by installing it you will still need to activate it to make use of it.
 * </p>
 * <p>
 * However, the activation state is preserved across shutdowns of
 * AqBanking, so a backend remains active until it is explicitly deactivated.
 * </p>
 */
/*@{*/

/**
 * Returns a list of the names of currently active providers.
 */
AQBANKING_API 
const GWEN_STRINGLIST *AB_Banking_GetActiveProviders(const AB_BANKING *ab);

/**
 * <p>
 * Activates a backend. It remains active (even across shutdowns of
 * AqBanking) until it is explicitly deactivated (using
 * @ref AB_Banking_DeactivateProvider).
 * </p>
 * <p>
 * This function automatically imports all accounts presented by the
 * backend.
 * </p>
 * @return 0 if ok, error code otherwise (see @ref AB_ERROR)
 * @param ab banking interface
 * @param backend name of the backend (such as "aqhbci". You can retrieve
 * such a backend either from the list of active backends
 * (@ref AB_Banking_GetActiveProviders) or from an plugin description
 * retrieved via @ref AB_Banking_GetProviderDescrs (call
 * @ref GWEN_PluginDescription_GetName on that plugin description).
 */
AQBANKING_API 
int AB_Banking_ActivateProvider(AB_BANKING *ab, const char *backend);

/**
 * Deactivates a backend. This is the default state for backends until they
 * have explicitly been activated by @ref AB_Banking_ActivateProvider.
 * @return 0 if ok, error code otherwise (see @ref AB_ERROR)
 * @param ab banking interface
 * @param backend name of the backend (such as "aqhbci". You can retrieve
 * such a name either from the list of active backends
 * (@ref AB_Banking_GetActiveProviders) or from an plugin description
 * retrieved via @ref AB_Banking_GetProviderDescrs (call
 * @ref GWEN_PluginDescription_GetName on that plugin description).
 */
AQBANKING_API 
int AB_Banking_DeactivateProvider(AB_BANKING *ab, const char *backend);


/**
 * <p>
 * This function checks whether the given backend is currently active.
 * It returns 0 if the backend is inactive.
 *
 */
AQBANKING_API 
int AB_Banking_IsProviderActive(AB_BANKING *ab, const char *backend);


/**
 * This function simpifies wizard handling. It searches for a wizard for
 * the given frontends.
 * @param ab pointer to the AB_BANKING object
 * @param backend no longer used
 * @param frontends This is a semicolon separated list of acceptable frontends
 * The following lists merely are suggestions:
 * <table>
 *  <tr>
 *    <td>KDE Applications</td>
 *    <td>kde;qt;gtk;gnome</td>
 *  </tr>
 *  <tr>
 *    <td>QT Applications</td>
 *    <td>qt;kde;gtk;gnome</td>
 *  </tr>
 *  <tr>
 *    <td>GNOME Applications</td>
 *    <td>gnome;gtk;qt;kde</td>
 *  </tr>
 *  <tr>
 *    <td>GTK Applications</td>
 *    <td>gtk;gnome;qt;kde</td>
 *  </tr>
 * </table>
 * You can always add an asterisk ("*") to the list to accept any other
 * frontend (or pass a NULL pointer to accept the first valid frontend).
 */
AQBANKING_API
int AB_Banking_FindWizard(AB_BANKING *ab,
                          const char *backend,
                          const char *frontends,
                          GWEN_BUFFER *pbuf);

/**
 * This function simpifies debugger handling. It searches for a debugger for
 * the given backend and the given frontends.
 * @param ab pointer to the AB_BANKING object
 * @param backend name of the backend (such as "aqhbci". You can retrieve
 * such a name either from the list of active backends
 * (@ref AB_Banking_GetActiveProviders) or from an plugin description
 * retrieved via @ref AB_Banking_GetProviderDescrs (call
 * @ref GWEN_PluginDescription_GetName on that plugin description).
 * @param frontends This is a semicolon separated list of acceptable frontends
 * The following lists merely are suggestions:
 * <table>
 *  <tr>
 *    <td>KDE Applications</td>
 *    <td>kde;qt;gtk;gnome</td>
 *  </tr>
 *  <tr>
 *    <td>QT Applications</td>
 *    <td>qt;kde;gtk;gnome</td>
 *  </tr>
 *  <tr>
 *    <td>GNOME Applications</td>
 *    <td>gnome;gtk;qt;kde</td>
 *  </tr>
 *  <tr>
 *    <td>GTK Applications</td>
 *    <td>gtk;gnome;qt;kde</td>
 *  </tr>
 * </table>
 * You can always add an asterisk ("*") to the list to accept any other
 * frontend (or pass a NULL pointer to accept the first valid frontend).
 */
AQBANKING_API
int AB_Banking_FindDebugger(AB_BANKING *ab,
			    const char *backend,
			    const char *frontends,
                            GWEN_BUFFER *pbuf);


/*@}*/



/** @name Application Data
 *
 * Applications may let AqBanking store global application specific data.
 * In addition, account specific data can also be stored using
 * @ref AB_Account_GetAppData.
 */
/*@{*/
/**
 * Returns the application name as given to @ref AB_Banking_new.
 * @param ab pointer to the AB_BANKING object
 */
AQBANKING_API 
const char *AB_Banking_GetAppName(const AB_BANKING *ab);

/**
 * Returns the escaped version of the application name. This name can
 * safely be used to create file paths since all special characters (like
 * '/', '.' etc) are escaped.
 * @param ab pointer to the AB_BANKING object
 */
AQBANKING_API 
const char *AB_Banking_GetEscapedAppName(const AB_BANKING *ab);

/**
 * Returns a GWEN_DB_NODE which can be used to store/retrieve data for
 * the currently running application. The group returned MUST NOT be
 * freed !
 * AqBanking is able to separate and store the data for every application.
 * @param ab pointer to the AB_BANKING object
 */
AQBANKING_API 
GWEN_DB_NODE *AB_Banking_GetAppData(AB_BANKING *ab);

/**
 * Returns a GWEN_DB_NODE which can be used to store/retrieve data shared
 * across multiple applications.
 * @param ab pointer to the AB_BANKING object
 * @param name name of the share. Special names are those of frontends and
 * backends, so these names MUST NOT be used for applications (but they may be
 * used by the corresponding frontends, e.g. the QT frontend QBanking used
 * "qbanking" to store some frontend specific settings).
 */
AQBANKING_API 
GWEN_DB_NODE *AB_Banking_GetSharedData(AB_BANKING *ab, const char *name);

/**
 * Returns the name of the user folder for AqBanking's data.
 * Normally this is something like "/home/me/.banking".
 * @return 0 if ok, error code otherwise (see @ref AB_ERROR)
 * @param ab pointer to the AB_BANKING object
 * @param buf GWEN_BUFFER to append the path name to
 */
AQBANKING_API 
int AB_Banking_GetUserDataDir(const AB_BANKING *ab, GWEN_BUFFER *buf);

/**
 * Returns the name of the user folder for application data.
 * Normally this is something like "/home/me/.banking/apps".
 * Your application may choose to create folders below this one to store
 * user data. If you only add AqBanking to an existing program to add
 * home banking support you will most likely use your own folders and thus
 * won't need this function.
 * @return 0 if ok, error code otherwise (see @ref AB_ERROR)
 * @param ab pointer to the AB_BANKING object
 * @param buf GWEN_BUFFER to append the path name to
 */
AQBANKING_API 
int AB_Banking_GetAppUserDataDir(const AB_BANKING *ab, GWEN_BUFFER *buf);

/**
 * Returns the path to a folder to which shared data can be stored.
 * This might be used by multiple applications if they wish to share some
 * of their data, e.g. QBankManager and AqMoney3 share their transaction
 * storage so that both may work with it.
 * Please note that this folder does not necessarily exist, but you are free
 * to create it.
 */
AQBANKING_API 
int AB_Banking_GetSharedDataDir(const AB_BANKING *ab,
                                const char *name,
                                GWEN_BUFFER *buf);

/** Returns the void pointer that was stored by
 * AB_Banking_SetUserData(). This might be useful for passing data to
 * the callback functions.
 *
 * On the other hand, we strongly encourage using the GWEN_INHERIT
 * macros to store non-trivial data structures in this object. 
 *
 * @param ab Pointer to the AB_BANKING object
 */
AQBANKING_API
void *AB_Banking_GetUserData(AB_BANKING *ab);

/** Save the void pointer that can be retrieved by
 * AB_Banking_GetUserData(). This might be useful for passing data to
 * the callback functions.
 *
 * On the other hand, we strongly encourage using the GWEN_INHERIT
 * macros to store non-trivial data structures in this object. 
 *
 * @param ab Pointer to the AB_BANKING object
 * @param user_data Arbitrary pointer to be stored in the AB_BANKING
 */
AQBANKING_API
void AB_Banking_SetUserData(AB_BANKING *ab, void *user_data);

/*@}*/






/** @name Plugin Handling
 *
 */
/*@{*/
/**
 * Returns a list2 of provider descriptions. You must free this list after
 * using it via @ref GWEN_PluginDescription_List2_freeAll.
 * Please note that a simple @ref GWEN_PluginDescription_List2_free would
 * not suffice, since that would only free the list but not the objects
 * stored within the list !
 * @param ab pointer to the AB_BANKING object
 */
AQBANKING_API 
GWEN_PLUGIN_DESCRIPTION_LIST2 *AB_Banking_GetProviderDescrs(AB_BANKING *ab);


/**
 * Returns a list2 of wizard descriptions.
 * You must free this list after using it via
 * @ref GWEN_PluginDescription_List2_freeAll.
 * Please note that a simple @ref GWEN_PluginDescription_List2_free would
 * not suffice, since that would only free the list but not the objects
 * stored within the list !
 * @param ab pointer to the AB_BANKING object
 */
AQBANKING_API 
GWEN_PLUGIN_DESCRIPTION_LIST2 *AB_Banking_GetWizardDescrs(AB_BANKING *ab);


/**
 * Returns a list2 of debugger descriptions for the given backend.
 * You must free this list after using it via
 * @ref GWEN_PluginDescription_List2_freeAll.
 * Please note that a simple @ref GWEN_PluginDescription_List2_free would
 * not suffice, since that would only free the list but not the objects
 * stored within the list !
 * @param ab pointer to the AB_BANKING object
 * @param pn name of the backend (such as "aqhbci". You can retrieve
 * such a name either from the list of active backends
 * (@ref AB_Banking_GetActiveProviders) or from an plugin description
 * retrieved via @ref AB_Banking_GetProviderDescrs (call
 * @ref GWEN_PluginDescription_GetName on that plugin description).
 */
AQBANKING_API 
GWEN_PLUGIN_DESCRIPTION_LIST2 *AB_Banking_GetDebuggerDescrs(AB_BANKING *ab,
                                                            const char *pn);
/*@}*/



/** @name Handling SSL Certificates
 *
 */
/*@{*/

/**
 * This influences the behaviour of AqBanking when new certificates are
 * received. If the returned value is !=0 then the user will be asked for
 * every single certificate received.
 */
AQBANKING_API
int AB_Banking_GetAlwaysAskForCert(const AB_BANKING *ab);

/**
 * This influences the behaviour of AqBanking when new certificates are
 * received. If the given value is !=0 then the user will be asked for
 * every single certificate received.
 */
AQBANKING_API 
void AB_Banking_SetAlwaysAskForCert(AB_BANKING *ab, int i);
/*@}*/




/*@}*/ /* addtogroup */


#ifdef __cplusplus
}
#endif



#endif




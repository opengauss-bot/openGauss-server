/* -------------------------------------------------------------------------
 *
 * win32env.cpp
 *	  putenv() and unsetenv() for win32, that updates both process
 *	  environment and the cached versions in (potentially multiple)
 *	  MSVCRT.
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/common/port/win32env.cpp
 *
 * -------------------------------------------------------------------------
 */

#include "c.h"
#include "securec.h"

int pgwin32_putenv(const char* envval)
{
    char* envcpy = NULL;
    char* cp = NULL;

    /*
     * Each version of MSVCRT has its own _putenv() call in the runtime
     * library.
     *
     * mingw always uses MSVCRT.DLL, but if we are in a Visual C++
     * environment, attempt to update the environment in all MSVCRT modules
     * that are currently loaded, to work properly with any third party
     * libraries linked against a different MSVCRT but still relying on
     * environment variables.
     *
     * Also separately update the system environment that gets inherited by
     * subprocesses.
     */
#ifdef _MSC_VER
    typedef int(_cdecl * PUTENVPROC)(const char*);
    static struct {
        char* modulename;
        HMODULE hmodule;
        PUTENVPROC putenvFunc;
    } rtmodules[] = {{"msvcrt", 0, NULL}, /* Visual Studio 6.0 / mingw */
        {"msvcr70", 0, NULL},             /* Visual Studio 2002 */
        {"msvcr71", 0, NULL},             /* Visual Studio 2003 */
        {"msvcr80", 0, NULL},             /* Visual Studio 2005 */
        {"msvcr90", 0, NULL},             /* Visual Studio 2008 */
        {"msvcr100", 0, NULL},            /* Visual Studio 2010 */
        {"msvcr110", 0, NULL},            /* Visual Studio 2012 */
        {NULL, 0, NULL}};
    int i;

    for (i = 0; rtmodules[i].modulename; i++) {
        if (rtmodules[i].putenvFunc == NULL) {
            if (rtmodules[i].hmodule == 0) {
                /* Not attempted before, so try to find this DLL */
                rtmodules[i].hmodule = GetModuleHandle(rtmodules[i].modulename);
                if (rtmodules[i].hmodule == NULL) {
                    /*
                     * Set to INVALID_HANDLE_VALUE so we know we have tried
                     * this one before, and won't try again.
                     */
#ifdef WIN32
                    rtmodules[i].hmodule = (HMODULE)INVALID_HANDLE_VALUE;
#else
                    rtmodules[i].hmodule = INVALID_HANDLE_VALUE;
#endif
                    continue;
                } else {
                    rtmodules[i].putenvFunc = (PUTENVPROC)GetProcAddress(rtmodules[i].hmodule, "_putenv");
                    if (rtmodules[i].putenvFunc == NULL) {
                        CloseHandle(rtmodules[i].hmodule);
#ifdef WIN32
                        rtmodules[i].hmodule = (HMODULE)INVALID_HANDLE_VALUE;
#else
                        rtmodules[i].hmodule = INVALID_HANDLE_VALUE;
#endif
                        continue;
                    }
                }
            } else {
                /*
                 * Module loaded, but we did not find the function last time.
                 * We're not going to find it this time either...
                 */
                continue;
            }
        }
        /* At this point, putenvFunc is set or we have exited the loop */
        rtmodules[i].putenvFunc(envval);
    }
#endif /* _MSC_VER */

    /*
     * Update the process environment - to make modifications visible to child
     * processes.
     *
     * Need a copy of the string so we can modify it.
     */
    envcpy = strdup(envval);
    if (envcpy == NULL) {
        return -1;
    }
    cp = strchr(envcpy, '=');
    if (cp == NULL) {
        free(envcpy);
        envcpy = NULL;
        return -1;
    }
    *cp = '\0';
    cp++;
    if (strlen(cp)) {
        /*
         * Only call SetEnvironmentVariable() when we are adding a variable,
         * not when removing it. Calling it on both crashes on at least
         * certain versions of MingW.
         */
        if (!SetEnvironmentVariable(envcpy, cp)) {
            free(envcpy);
            envcpy = NULL;
            return -1;
        }
    }
    free(envcpy);
    envcpy = NULL;

    /* Finally, update our "own" cache */
    return _putenv(envval);
}

void pgwin32_unsetenv(const char* name)
{
    char* envbuf = NULL;

    envbuf = (char*)malloc(strlen(name) + 2);
    if (envbuf == NULL) {
        return;
    }

    errno_t rc = sprintf_s(envbuf, strlen(name) + 2, "%s=", name);
    if (rc == -1) {
        printf("ERROR at %s : %d : The destination buffer or format is a NULL pointer or the invalid parameter handle "
               "is invoked..\n",
            __FILE__,
            __LINE__);
        exit(1);
    }
    pgwin32_putenv(envbuf);
    free(envbuf);
    envbuf = NULL;
}

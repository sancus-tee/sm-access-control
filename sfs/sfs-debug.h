/**
 * printdebug macros to support pretty printing and toggling of fine grained
 * debug/error level output (defined in Makefile)
 */
#ifndef SFS_DEBUG_H
#define SFS_DEBUG_H

#ifndef NODEBUG
    // include generic printdebug macros
    #include "../common.h"
#else
    #define puts(str)
    #define printf_int(str, i)
    #define printf_int_int(str, i, j)
    #define printf_str(str)
#endif

//#define SFS_DEBUG

/******** GENERIC LEVEL (toggable with NODEBUG) *********/

#ifndef NODEBUG
    #define FS                              GREEN "\t[sfs] " NONE
    #define FCT(fct_name)                   BOLD fct_name NONE ": "
    #define printd(str)                     printdebug(FS str)
    #define printdi(str, i)                 printdebug_int(FS str "\n", i)
    #define printdii(str, i, j)             printdebug_int_int(FS str "\n", i, j)
    #define printd_n(str, name) \
    do { \
        printdebug_str(FS str); \
        printdebug_int(" '%c'", name); \
    } while(0)
    #define printd_name(str, name) \
    do { \
        printd_n(str, name); \
        printdebug_str("\n"); \
    } while(0)

    #define FS_ERR                          FS RED
    #define printe(str)                     printerr(FS_ERR str)
    #define printe_int(str, int)            printerr_int(FS_ERR str "\n", int)
    #define printe_int_int(str, i, j)       printerr_int_int(FS_ERR str "\n", i, j)
#else
    #define BOLD
    #define NONE
    #define printd(str)                     
    #define printdi(str, i)                 
    #define printdii(str, i, j)            
    #define printd_n(str, name)
    #define printd_name(str, name) 
    #define printe(str)                    
    #define printe_int(str, int)           
    #define printe_int_int(str, i, j)
#endif

/********************** DEBUG LEVEL *********************/

#define DEBUG_STR                       "DEBUG::"

#ifdef SFS_DEBUG
#define printd_debug(str)               printd(DEBUG_STR str)
#define printdi_debug(str, i)           printdi(DEBUG_STR str, i)
#define printdii_debug(str, i, j)       printdii(DEBUG_STR str, i, j)
#define printdname_debug(str, name)     printd_name(DEBUG_STR str, name)
#else
#define printd_debug(str)               
#define printdi_debug(str, i)      
#define printdii_debug(str, i, j)     
#define printdname_debug(str, name)    
#endif //SFS_DEBUG

/********************** INFO LEVEL **********************/

#define INFO_STR                        "INFO::"

#if defined(SFS_INFO) || defined(SFS_DEBUG)
#define printd_info(str)                printd(INFO_STR str)
#define printdii_info(str, i, j)        printdii(INFO_STR str, i, j)
#define printdi_info(str, i)            printdi(INFO_STR str, i)
#define printdname_info(str, name)      printd_name(INFO_STR str, name)
#else
#define printd_info(str)               
#define printdi_info(str, i)          
#define printdii_info(str, i, j)
#define printdname_info(str, name)    
#endif

/********************** WARNING LEVEL *******************/

#if defined(SFS_WARNING) || defined(SFS_INFO) || defined(SFS_DEBUG)
#define WARNING_STR                     YELLOW "WARNING" NONE "::"
#define printd_warning(str)             printd(WARNING_STR str)
#define printdi_warning(str, i)         printdi(WARNING_STR str, i)
#define printdii_warning(str, i, j)     printdii(WARNING_STR str, i, j)
#define printdname_warning(str, name)   printd_name(WARNING_STR str, name)
#else
#define printd_warning(str)               
#define printdi_warning(str, i)
#define printdii_warning(str, i, j)    
#define printdname_warning(str, name)    
#endif

/********************** ERROR LEVEL *********************/

#define ERROR_STR                       "ERROR::"

#ifndef NODEBUG
    // #if defined(SFS_ERROR) || defined(SFS_WARNING) || defined(SFS_INFO) || defined(SFS_DEBUG)
    #define printerror(str)                 printe(ERROR_STR str)
    #define printerror_int(str, int)        printe_int(ERROR_STR str, int)
    #define printerror_int_int(str, i, j)   printe_int_int(ERROR_STR str, i, j)
#else
    #define printerror(str)
    #define printerror_int(str, int)
    #define printerror_int_int(str, i, j)
#endif

#endif //SFS_DEBUG_H

/* win32_compat.h
 *
 * compatibility stuff for Windows
 *
 */

#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H 1

#ifdef	__cplusplus
extern "C" {
#endif

#define _POSIX_

#include <io.h>

#define snprintf _snprintf
#define lstat stat
#define fsync(fd) _commit(fd)
#define mkdir(dir,mode) mkdir(dir)


#ifdef	__cplusplus
}
#endif

#endif /* WIN32_COMPAT_H */

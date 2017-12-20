#ifndef DISABLE_INTL

#include <libintl.h>
#include <locale.h>
#define _(String) gettext( String )
#define N_(String) gettext_noop( String )
#define gettext_noop( String )String

#else

#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)


#endif
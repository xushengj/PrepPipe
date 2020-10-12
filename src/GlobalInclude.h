#ifndef GLOBALINCLUDE_H
#define GLOBALINCLUDE_H

#include <QList>

/// the type for all indices and size that may interact with Qt containers; int in Qt5, qsizetype in Qt6
using indextype = decltype (std::declval<QList<int>>().size());

#endif // GLOBALINCLUDE_H

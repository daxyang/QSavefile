#ifndef QSAVEFILE_GLOBAL_H
#define QSAVEFILE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QSAVEFILE_LIBRARY)
#  define QSAVEFILESHARED_EXPORT Q_DECL_EXPORT
#else
#  define QSAVEFILESHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QSAVEFILE_GLOBAL_H

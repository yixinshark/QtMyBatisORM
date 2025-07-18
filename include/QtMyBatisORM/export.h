#pragma once

#include <QtCore/qglobal.h>

// Export macros for Windows/Unix compatibility
#if defined(QtMyBatisORM_EXPORTS)
#  define QTMYBATISORM_EXPORT Q_DECL_EXPORT
#else
#  define QTMYBATISORM_EXPORT Q_DECL_IMPORT
#endif 
#pragma once

#include "QELogger.h"

// ==========================
// LOGS SIMPLES (sin format)
// ==========================

#define QE_LOG_INFO(msg) \
    QELogger::Get().Info((msg))

#define QE_LOG_WARN(msg) \
    QELogger::Get().Warning((msg))

#define QE_LOG_ERROR(msg) \
    QELogger::Get().Error((msg))

#define QE_LOG_DEBUG(msg) \
    QELogger::Get().Debug((msg))

// Con categoría

#define QE_LOG_INFO_CAT(cat, msg) \
    QELogger::Get().Info((msg), (cat))

#define QE_LOG_WARN_CAT(cat, msg) \
    QELogger::Get().Warning((msg), (cat))

#define QE_LOG_ERROR_CAT(cat, msg) \
    QELogger::Get().Error((msg), (cat))

#define QE_LOG_DEBUG_CAT(cat, msg) \
    QELogger::Get().Debug((msg), (cat))

// ==========================
// LOGS FORMATEADOS (C++20)
// ==========================

#define QE_LOG_INFO_F(fmt, ...) \
    QELogger::Get().InfoFormat("", (fmt), __VA_ARGS__)

#define QE_LOG_WARN_F(fmt, ...) \
    QELogger::Get().WarningFormat("", (fmt), __VA_ARGS__)

#define QE_LOG_ERROR_F(fmt, ...) \
    QELogger::Get().ErrorFormat("", (fmt), __VA_ARGS__)

#define QE_LOG_DEBUG_F(fmt, ...) \
    QELogger::Get().DebugFormat("", (fmt), __VA_ARGS__)

// Con categoría

#define QE_LOG_INFO_CAT_F(cat, fmt, ...) \
    QELogger::Get().InfoFormat((cat), (fmt), __VA_ARGS__)

#define QE_LOG_WARN_CAT_F(cat, fmt, ...) \
    QELogger::Get().WarningFormat((cat), (fmt), __VA_ARGS__)

#define QE_LOG_ERROR_CAT_F(cat, fmt, ...) \
    QELogger::Get().ErrorFormat((cat), (fmt), __VA_ARGS__)

#define QE_LOG_DEBUG_CAT_F(cat, fmt, ...) \
    QELogger::Get().DebugFormat((cat), (fmt), __VA_ARGS__)

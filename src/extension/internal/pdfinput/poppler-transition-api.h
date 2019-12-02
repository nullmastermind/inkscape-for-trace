// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO short description
 *//*
 * Authors:
 *   see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_POPPLER_TRANSITION_API_H
#define SEEN_POPPLER_TRANSITION_API_H

#include <glib/poppler-features.h>

#if POPPLER_CHECK_VERSION(0, 83, 0)
#define _POPPLER_CONST_83 const
#else
#define _POPPLER_CONST_83
#endif

#if POPPLER_CHECK_VERSION(0, 82, 0)
#define _POPPLER_CONST_82 const
#else
#define _POPPLER_CONST_82
#endif

#if POPPLER_CHECK_VERSION(0, 76, 0)
#define _POPPLER_NEW_PARSER(xref, obj) Parser(xref, obj, gFalse)
#else
#define _POPPLER_NEW_PARSER(xref, obj) Parser(xref, new Lexer(xref, obj), gFalse)
#endif

#if POPPLER_CHECK_VERSION(0, 83, 0)
#define _POPPLER_NEW_GLOBAL_PARAMS(args...) std::unique_ptr<GlobalParams>(new GlobalParams(args))
#else
#define _POPPLER_NEW_GLOBAL_PARAMS(args...) new GlobalParams(args)
#endif


#if POPPLER_CHECK_VERSION(0, 72, 0)
#define getCString c_str
#endif

#if POPPLER_CHECK_VERSION(0,71,0)
typedef bool GBool;
#define gTrue true
#define gFalse false
#endif

#if POPPLER_CHECK_VERSION(0,70,0)
#define _POPPLER_CONST const
#else
#define _POPPLER_CONST
#endif

#if POPPLER_CHECK_VERSION(0,69,0)
#define _POPPLER_DICTADD(dict, key, obj) (dict).dictAdd(key, std::move(obj))
#elif POPPLER_CHECK_VERSION(0,58,0)
#define _POPPLER_DICTADD(dict, key, obj) (dict).dictAdd(copyString(key), std::move(obj))
#else
#define _POPPLER_DICTADD(dict, key, obj) (dict).dictAdd(copyString(key), &obj)
#endif

#if POPPLER_CHECK_VERSION(0,58,0)
#define POPPLER_NEW_OBJECT_API
#define _POPPLER_FREE(obj)
#define _POPPLER_CALL(ret, func) (ret = func())
#define _POPPLER_CALL_ARGS(ret, func, ...) (ret = func(__VA_ARGS__))
#define _POPPLER_CALL_ARGS_DEREF _POPPLER_CALL_ARGS
#else
#define _POPPLER_FREE(obj) (obj).free()
#define _POPPLER_CALL(ret, func) (*func(&ret))
#define _POPPLER_CALL_ARGS(ret, func, ...) (func(__VA_ARGS__, &ret))
#define _POPPLER_CALL_ARGS_DEREF(...) (*_POPPLER_CALL_ARGS(__VA_ARGS__))
#endif

#if POPPLER_CHECK_VERSION(0, 29, 0)
#define POPPLER_EVEN_NEWER_NEW_COLOR_SPACE_API
#endif

#if POPPLER_CHECK_VERSION(0, 25, 0)
#define POPPLER_EVEN_NEWER_COLOR_SPACE_API
#endif

#endif

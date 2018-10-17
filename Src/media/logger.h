/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ELOG_H__
#define __ELOG_H__

#include <stdlib.h>
#include <stdio.h>

// #include <log4cxx/logger.h>
// #include <log4cxx/helpers/exception.h>

#define DECLARE_LOGGER() /*\
 static log4cxx::LoggerPtr logger;*/

 #define DEFINE_LOGGER(namespace, logName) /*\
 log4cxx::LoggerPtr namespace::logger = log4cxx::Logger::getLogger( logName );*/

#define ELOG_MAX_BUFFER_SIZE 10000

#define SPRINTF_ELOG_MSG(buffer, fmt, ...) \
    char buffer[ELOG_MAX_BUFFER_SIZE] = {0}; \
    _snprintf( buffer, ELOG_MAX_BUFFER_SIZE, fmt, __VA_ARGS__ );

// older versions of log4cxx don't support tracing
#ifdef LOG4CXX_TRACE
#define ELOG_TRACE2(logger, fmt, ...) /*\
    if (logger->isTraceEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, __VA_ARGS__ ); \
        LOG4CXX_TRACE( logger, __tmp ); \
    }*/
#else
#define ELOG_TRACE2(logger, fmt, ...)/* \
    if (logger->isDebugEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, __VA_ARGS__ ); \
        LOG4CXX_DEBUG( logger, __tmp ); \
    }*/
#endif

#define ELOG_DEBUG2(logger, fmt, ...) /*\
    if (logger->isDebugEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, __VA_ARGS__ ); \
        LOG4CXX_DEBUG( logger, __tmp ); \
    }*/

#define ELOG_INFO2(logger, fmt, ...) /*\
    if (logger->isInfoEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, __VA_ARGS__ ); \
        LOG4CXX_INFO( logger, __tmp ); \
    }*/

#define ELOG_WARN2(logger, fmt, ...) /*\
    if (logger->isWarnEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, __VA_ARGS__ ); \
        LOG4CXX_WARN( logger, __tmp ); \
    }*/

#define ELOG_ERROR2(logger, fmt, ...) /*\
    if (logger->isErrorEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, __VA_ARGS__ ); \
        LOG4CXX_ERROR( logger, __tmp ); \
    }*/

#define ELOG_FATAL2(logger, fmt, ...)/* \
    if (logger->isFatalEnabled()) { \
        SPRINTF_ELOG_MSG( __tmp, fmt, __VA_ARGS__ ); \
        LOG4CXX_FATAL( logger, __tmp ); \
    }*/


#define ELOG_TRACE(fmt, ...) \
    ELOG_TRACE2( logger, fmt, __VA_ARGS__ );

#define ELOG_DEBUG(fmt, ...) \
    ELOG_DEBUG2( logger, fmt, __VA_ARGS__ );

#define ELOG_INFO(fmt, ...) \
    ELOG_INFO2( logger, fmt, __VA_ARGS__ );

#define ELOG_WARN(fmt, ...) \
    ELOG_WARN2( logger, fmt, __VA_ARGS__ );

#define ELOG_ERROR(fmt, ...) \
    ELOG_ERROR2( logger, fmt, __VA_ARGS__ );

#define ELOG_FATAL(fmt, ...) \
    ELOG_FATAL2( logger, fmt, __VA_ARGS__ );

#endif  /* __ELOG_H__ */

/*****************************************************************************
*
* libobjcryst       Complex Modeling Initiative
*                   (c) 2015 Brookhaven Science Associates,
*                   Brookhaven National Laboratory.
*                   All rights reserved.
*
* File coded by:    Pavol Juhas
*
* See AUTHORS.txt for a list of people who contributed.
* See LICENSE.txt for license information.
*
******************************************************************************
*
* Macro definitions for
*   LIBOBJCRYST_VERSION,
*   LIBOBJCRYST_VERSION_MAJOR,
*   LIBOBJCRYST_VERSION_MINOR,
*   LIBOBJCRYST_VERSION_MICRO,
*   LIBOBJCRYST_VERSION_PATCH,
*   LIBOBJCRYST_VERSION_STR,
*   LIBOBJCRYST_VERSION_DATE
*   LIBOBJCRYST_GIT_SHA
*
* Declaration of libobjcryst_version_info struct for version data resolved
* at runtime.
*
*****************************************************************************/

#ifndef LIBOBJCRYST_VERSION_HPP_INCLUDED
#define LIBOBJCRYST_VERSION_HPP_INCLUDED

#define LIBOBJCRYST_VERSION_MAJOR ${LIBOBJCRYST_VERSION_MAJOR}
#define LIBOBJCRYST_VERSION_MINOR ${LIBOBJCRYST_VERSION_MINOR}
#define LIBOBJCRYST_VERSION_MICRO ${LIBOBJCRYST_VERSION_MICRO}
// number of git commits since the last release tag
#define LIBOBJCRYST_VERSION_PATCH ${LIBOBJCRYST_VERSION_PATCH}

// LIBOBJCRYST_VERSION is an integer representation of a full version:
//
// LIBOBJCRYST_VERSION / 1000000000 is the major version number
// LIBOBJCRYST_VERSION / 1000000 % 1000 is the minor version number
// LIBOBJCRYST_VERSION / 1000 % 1000 is the micro version number
// LIBOBJCRYST_VERSION % 500 is the patch number
//
// alpha and beta releases have smaller LIBOBJCRYST_VERSION than
// a finalized release.  Numerical comparison of LIBOBJCRYST_VERSION
// values from two pre-releases may be inaccurate.

#define LIBOBJCRYST_VERSION ${LIBOBJCRYST_VERSION}

// LIBOBJCRYST_VERSION_STR is a string form of LIBOBJCRYST_VERSION

#define LIBOBJCRYST_VERSION_STR "${LIBOBJCRYST_VERSION_STR}"

// LIBOBJCRYST_VERSION_DATE is the commit date of LIBOBJCRYST_VERSION

#define LIBOBJCRYST_VERSION_DATE "${LIBOBJCRYST_VERSION_DATE}"

// LIBOBJCRYST_GIT_SHA is a full git commit hash for the current version

#define LIBOBJCRYST_GIT_SHA "${LIBOBJCRYST_GIT_SHA}"

// libobjcryst_version_info will hold runtime version data, which may be
// different from client compile-time values.

struct libobjcryst_version_info {

    static const long long version;
    static const char* version_str;
    static const int major;
    static const int minor;
    static const int micro;
    static const int patch;
    static const char* date;
    static const char* git_sha;

};

#endif  // LIBOBJCRYST_VERSION_HPP_INCLUDED

// vim:ft=cpp:

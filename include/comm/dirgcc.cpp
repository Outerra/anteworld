/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is COID/comm module.
 *
 * The Initial Developer of the Original Code is
 * PosAm.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Brano Kemen
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include "dir.h"



#ifndef SYSTYPE_WIN

#include <fnmatch.h>
#include <errno.h>
#include <utime.h>
#include <dlfcn.h>
#include <unistd.h>

#define xstat64 stat64

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
directory::~directory()
{
    close();
}

////////////////////////////////////////////////////////////////////////////////
directory::directory()
{
    _dir = 0;
}

////////////////////////////////////////////////////////////////////////////////
char directory::separator()             { return '/'; }
const char* directory::separator_str()  { return "/"; }

////////////////////////////////////////////////////////////////////////////////
opcd directory::open( const token& path, const token& filter )
{
    close();

    _curpath = path;
    if( _curpath.last_char() == '/' )
        _curpath.resize(-1);

    _dir = opendir( _curpath.ptr() );
    if(!_dir)
        return ersFAILED;

    _curpath << '/';
    _baselen = _curpath.len();

    stat64(_curpath.ptr(), &_st);

    _pattern = filter ? filter : token("*");
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
void directory::close()
{
    if(_dir)
        closedir(_dir);
    _dir = 0;
}

////////////////////////////////////////////////////////////////////////////////
bool directory::is_entry_open() const
{
    return _dir != 0;
}

////////////////////////////////////////////////////////////////////////////////
bool directory::is_entry_directory() const
{
    return S_ISDIR(_st.st_mode);
}

////////////////////////////////////////////////////////////////////////////////
bool directory::is_entry_regular() const
{
    return S_ISREG(_st.st_mode);
}

////////////////////////////////////////////////////////////////////////////////
bool directory::is_directory( ushort mode )
{
    return S_ISDIR(mode);
}

////////////////////////////////////////////////////////////////////////////////
bool directory::is_regular( ushort mode )
{
    return S_ISREG(mode);
}

////////////////////////////////////////////////////////////////////////////////
int directory::is_valid(zstring path)
{
    xstat st;
    return xstat64(no_trail_sep(path), &st) == 0
        ? (is_regular(st.st_mode) ? 1 : 2)
        : 0;
}

////////////////////////////////////////////////////////////////////////////////
bool directory::is_valid_file(zstring arg)
{
    if(!arg)
        return false;

    xstat st;
    return xstat64(no_trail_sep(arg), &st) == 0 && is_regular(st.st_mode);
}

////////////////////////////////////////////////////////////////////////////////
bool directory::is_valid_dir(const char* arg)
{
    directory::xstat st;
    return xstat64(arg, &st) == 0 && directory::is_directory(st.st_mode);
}

////////////////////////////////////////////////////////////////////////////////
bool directory::subpath(token root, token& path)
{
    while (root && path) {
        token r = root.cut_left_group(DIR_SEPARATORS);
        token p = path.cut_left_group(DIR_SEPARATORS);

        if (r != p)
            break;
    }

    return root.is_empty();
}

////////////////////////////////////////////////////////////////////////////////
opcd directory::mkdir( zstring name, mode_t mode )
{
    if(!::mkdir(name.c_str(), mode))  return 0;
    if( errno == EEXIST )  return 0;
    return ersFAILED;
}

////////////////////////////////////////////////////////////////////////////////
opcd directory::move_file(zstring src, zstring dst)
{
    //TODO directories
    if(0 == ::rename(src.c_str(), dst.c_str()))
        return 0;
    return ersIO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
charstr directory::get_cwd()
{
    charstr buf;
    uints size = PATH_MAX;

    while (size < 1024 && !getcwd(buf.get_buf(size - 1), size))
    {
        size <<= 1;
        buf.reset();
    }
    buf.correct_size();

    treat_trailing_separator(buf, true);
    return buf;
}

////////////////////////////////////////////////////////////////////////////////
charstr directory::get_program_path()
{
    charstr path = "/proc/";
    path << ::getpid() << "/exe";

    charstr buf;
    ::readlink(path.c_str(), buf.get_buf(PATH_MAX), PATH_MAX);

    buf.correct_size();
    return buf;
}

////////////////////////////////////////////////////////////////////////////////
uints directory::get_module_path_func(const void* fn, charstr& dst, bool append)
{
    Dl_info info;
    dladdr((void*)fn, &info);

    if (append)
        dst << info.dli_fname;
    else
        dst = info.dli_fname;

    return reinterpret_cast<uints>(info.dli_fbase);
}

////////////////////////////////////////////////////////////////////////////////
int directory::chdir( zstring name )
{
    return ::chdir(name.c_str());
}

////////////////////////////////////////////////////////////////////////////////
const directory::xstat* directory::next()
{
    dirent* dire = readdir(_dir);
    if(!dire)
        return 0;

    if( 0 == fnmatch(_pattern.ptr(), dire->d_name, 0) )
    {
        _curpath.resize( _baselen );
        _curpath << dire->d_name;
        if(stat64(_curpath.ptr(), &_st) == 0)
            return &_st;
    }

    return next();
}

////////////////////////////////////////////////////////////////////////////////
charstr directory::get_home_dir()
{
    return "~/";
}

////////////////////////////////////////////////////////////////////////////////
opcd directory::truncate( zstring fname, uint64 size )
{
    return truncate(fname.c_str(), (off_t)size) == 0
        ? 0
        : ersFAILED;
}

////////////////////////////////////////////////////////////////////////////////
opcd directory::set_file_times(zstring fname, timet actime, timet modtime)
{
    struct utimbuf ut;
    ut.actime = actime.t;
    ut.modtime = modtime.t;

    return utime(fname.c_str(), &ut) == 0 ? 0 : ersFAILED;
}

////////////////////////////////////////////////////////////////////////////////
// this is mostly copypasta of ::list_file_paths
// TODO: probably can be greatly optimized
void directory::find_files(
    const token& path,
    const token& extension,
    bool recursive,
    bool return_also_folders,
    const coid::function<void(const find_result& file_info)>& fn)
{
    directory dir;

    if (recursive) {
        if (dir.open(path, "*.*") != ersNOERR)
            return;
    }
    else {
        charstr filter = "*.";
        filter << extension;
        if (dir.open(path, filter) != ersNOERR)
            return;
    }

    while (dir.next()) {
        if (dir.is_entry_regular()) {
            if ((!recursive) || (extension == '*') || dir.get_last_file_name_token().cut_right_back('.').cmpeqi(extension)) {
                const xstat* stat = dir.get_stat();
                find_result result;
                result._path = dir.get_last_full_path();
                result._last_modified = stat->st_mtime;
                result._size = stat->st_size;
                result._flags = 0;
                fn(result);
            }
        }
        else if (dir.is_entry_subdirectory()) {
            if (return_also_folders) {
                const xstat* stat = dir.get_stat();
                find_result result;
                result._path = dir.get_last_full_path();
                result._last_modified = stat->st_mtime;
                result._size = 0;
                result._flags = 0;
                fn(result);
            }

            if (recursive) {
                directory::find_files(dir.get_last_full_path(), extension, recursive, return_also_folders, fn);
            }
        }
    }
}

COID_NAMESPACE_END


#endif //SYSTYPE_MSVC

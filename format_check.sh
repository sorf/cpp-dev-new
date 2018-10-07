#!/bin/bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SOURCE_FOLDERS="source/include source/lib source/test test_package"

if [ -z "$CLANG_FORMAT" ]; then
    CLANG_FORMAT=clang-format-6.0
fi

FULL_PATH_SOURCE_FOLDERS=""
for source_folder in $SOURCE_FOLDERS; do
    if [ -d "$SCRIPT_DIR/$source_folder" ]; then
        FULL_PATH_SOURCE_FOLDERS="$FULL_PATH_SOURCE_FOLDERS $SCRIPT_DIR/$source_folder"
    fi
done

SOURCE_FILES=$(find $FULL_PATH_SOURCE_FOLDERS -maxdepth 1 -iname *.hpp -o -iname *.cpp)
SOURCE_FILES_FAILED_CHECK=""
for source_file in $SOURCE_FILES; do
    echo -n "Checking: $source_file ... "
    if ! diff -q $source_file <($CLANG_FORMAT -style=file $source_file) > /dev/null; then
        if [ ! -z "$SOURCE_FILES_FAILED_CHECK" ]; then
            SOURCE_FILES_FAILED_CHECK="$SOURCE_FILES_FAILED_CHECK\n"
        fi
        SOURCE_FILES_FAILED_CHECK="$SOURCE_FILES_FAILED_CHECK\t$source_file"
        echo "FAIL"
    else
        echo OK
    fi
done

if [ ! -z "$SOURCE_FILES_FAILED_CHECK" ]; then
    echo -e "Files have failed the formatting check. Run 'bash format_fix.sh'"
    exit 1
fi
exit 0

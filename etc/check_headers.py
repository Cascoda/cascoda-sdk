#!/usr/bin/python

import os, datetime, argparse, sys, re

exclude_matches = [
    "[\\/]\.", # any top level .git or .vscode etc
    "__pycache__",
    "build"
]

filetype_matches = [
    ".*\.py$",
    ".*\.(c|h|cpp|hpp)$",
]

header = \
f"""
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright (c) {datetime.date.today().year} Cascoda Limited
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list
   of conditions and the following disclaimer.

2. Redistributions in binary form, except as embedded into a Cascoda Limited.
   integrated circuit in a product or a software update for such product, must
   reproduce the above copyright notice, this list of  conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of Cascoda Limited nor the names of its contributors may be used to
   endorse or promote products derived from this software without specific prior written
   permission.

4. This software, whether provided in binary or any other form must not be decompiled,
   disassembled, reverse engineered or otherwise modified.

 5. This software, in whole or in part, must only be used with a Cascoda Limited circuit.

THIS SOFTWARE IS PROVIDED BY CASCODA LIMITED "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CASCODA LIMITED OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

header_match = [
    "Copyright \\(c\\) ([0-9]{4})(-[0-9]{4})?",
    "/\*[^\n/]*Cascoda[^\n/]*\*/"
]

max_depth = 32

depth = 0
verbose = False
def verbose_print(s):
    if verbose:
        print(("  " * depth) + str(s))

def check_directory(path):
    if not os.path.exists(path):
        return f"Path does not exist '{path}'"
    if not os.path.isdir(path):
        return f"Path is not a directory '{path}'"
    if any(map(lambda e: re.search(e, path), exclude_matches)):
        return f"Excluding '{path}'"
    elems = os.listdir(path)
    global depth
    if depth > max_depth:
        print("Depth limit exceeded")
        return "Depth limit exceeded"
    depth = depth+1
    for elem in elems:
        newpath = os.path.join(path, elem)
        if os.path.isdir(newpath):
            verbose_print(check_directory(newpath))
        elif os.path.isfile(newpath):
            verbose_print(check_file(newpath))
    depth = depth-1

def check_file(path):
    if not os.path.exists(path):
        return f"Path does not exist '{path}'"
    if not os.path.isfile(path):
        return f"Path is not a file '{path}'"
    if any(map(lambda e: re.search(e, path), exclude_matches)):
        return f"Excluding '{path}'"
    for matcher in filetype_matches:
        if not re.search(matcher, path):
            continue

        with open(path, "r+") as fp:
            fp.seek(0)
            contents = fp.read()
            if any(map(lambda m: re.search(m, contents), header_match)):
                return f"Path {path} has header"
            print(f"Path {path} missing header")
            return f"Path {path} missing header"

        return f"Path '{path}' matched '{matcher}'"
    return f"Path {path} not matched"

if __name__ == "__main__":
    # optional parameters
    parser = argparse.ArgumentParser()
    # input (files etc.)
    parser.add_argument("-exclude", "--exclude", "-e", action='append', type=str,
                        default=exclude_matches, help="Exclude matches",
                        nargs='?', const="name", required=False)
    parser.add_argument("-include", "--include", "-i", action='append', type=str,
                        default=filetype_matches, help="Include matches",
                        nargs='?', const="name", required=False)
    parser.add_argument("-headermatch", "--headermatch", "-m", action='append', type=str,
                        default=header_match, help="Include matches",
                        nargs='?', const="name", required=False)
    parser.add_argument("-verbose", "--verbose", "-v", action='store_true',
                        default=False, help="Enable verbose output", required=False)
    parser.add_argument("-depth", "--depth", "-d", type=int, required=False, 
                        default=max_depth, help="Max directory traversal depth")
    parser.add_argument("base_dir", nargs=1)
    # (args) supports batch scripts providing arguments
    print(sys.argv)
    args = parser.parse_args()
    print("exclude           : " + str(args.exclude))
    print("include           : " + str(args.include))
    print("headermatch       : " + str(args.headermatch))
    print("verbose           : " + str(args.verbose))
    print("depth             : " + str(args.depth))
    print("base dir          : " + str(args.base_dir))
    exclude_matches = args.exclude
    filetype_matches = args.include
    header_match = args.headermatch
    verbose = args.verbose
    max_depth = args.depth
    base_dir = args.base_dir[0]
    gitignore_path = os.path.join(base_dir, ".gitignore")
    if os.path.exists(gitignore_path) and os.path.isfile(gitignore_path):
        with open(gitignore_path) as f:
            for line in f.readlines():
                line = line.replace('*', '.*').replace('\n', "")
                exclude_matches.append(line)
                print(f"Adding exclude '{line}' from gitignore")
    else:
        print("no .gitignore found")
    verbose_print(check_directory(base_dir))

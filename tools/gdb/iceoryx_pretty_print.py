# iceoryx pretty print for gdb
#
# This file should be sourced in '.gdbinit'
#
# Use 'info pretty-printer' to list all registered printer
# Use 'whatis foo' to get the type of foo
# Use 'source path/to/iceoryx_pretty_print.py' in the gdb console to add the pretty printer
# Use 'print /r foo' to skip the pretty print
#
# Python gdb module
# https://www.programcreek.com/python/index/598/gdb
# https://www.programcreek.com/python/example/10167/gdb.Value
# https://sourceware.org/gdb/onlinedocs/gdb/Values-From-Inferior.html
#
# Tutorial for pretty printer
# Basic types
# https://undo.io/resources/gdb-watchpoint/here-quick-way-pretty-print-structures-gdb/
# https://tromey.com/blog/?p=524
# Nested types like array/map
# https://undo.io/resources/gdb-watchpoint/debugging-pretty-printers-gdb-part2/
# https://tromey.com/blog/?p=546
# Advanced
# https://undo.io/resources/gdb-watchpoint/debugging-pretty-printers-gdb-part3/
#
# Additional material
# https://sourceware.org/gdb/onlinedocs/gdb/Writing-a-Pretty_002dPrinter.html#Writing-a-Pretty_002dPrinter
# https://sourceware.org/gdb/onlinedocs/gdb/Pretty-Printing-API.html#Pretty-Printing-API
# https://sourceware.org/gdb/onlinedocs/gdb/gdb_002eprinting.html#gdb_002eprinting
# https://getdocs.org/Gdb/Type-Printing-API
#
# TODO check if we need to implement xmethods
# https://sourceware.org/gdb/current/onlinedocs/gdb/Xmethods-In-Python.html#Xmethods-In-Python
# https://sourceware.org/gdb/current/onlinedocs/gdb/Xmethod-API.html#Xmethod-API
# https://sourceware.org/gdb/current/onlinedocs/gdb/Writing-an-Xmethod.html#Writing-an-Xmethod

#import re

class IoxCxxStringPrinter:
    "Print an iox::cxx::string"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        content = str(self.val['m_rawstring'])[1:int(self.val['m_rawstringSize'])+1] # remove the quotes or \000 from the string
        size = str(self.val['m_rawstringSize'])
        capacity = str(self.val.type).replace('iox::cxx::string<', '')[:-1]
        return '"' + content + '"' + ' (size: ' + size  + '; capacity: ' + capacity + ')'

class IoxCxxOptionalPrinter:
    "Print an iox::cxx::optional"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val['m_hasValue']:
            typename = str(self.val.type)[len('iox::cxx::optional<'):-1]
            typedPointer = self.val['m_data'].address.cast(gdb.lookup_type(typename).pointer())
            return '{' + str(typedPointer.dereference()) + '}'
        else:
            return "iox::cxx::nullopt"

def iox_pretty_print(val):
    # TODO check why this does not work
    #if re.search('iox::cxx::string<\d+>', text): return IoxCxxStringPrinter(val)
    if str(val.type).startswith('iox::cxx::string'): return IoxCxxStringPrinter(val)
    if str(val.type).startswith('iox::cxx::optional'): return IoxCxxOptionalPrinter(val)
    return None

gdb.pretty_printers.append(iox_pretty_print)

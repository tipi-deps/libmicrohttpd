#!/bin/bash

#
#   Generate header insert for HTTP methods
#

#   Copyright (c) 2015-2019 Karlson2k (Evgeny Grin) <k2k@yandex.ru>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

wget -nv http://www.iana.org/assignments/http-methods/methods.csv -O methods.csv || exit
echo Generating...
echo '/**
 * @defgroup methods HTTP methods
 * HTTP methods (as strings).
 * See: http://www.iana.org/assignments/http-methods/http-methods.xml
 * Registry export date: '$(date -u +%Y-%m-%d)'
 * @{
 */

/* Main HTTP methods. */' > header_insert_methods.h && \
gawk -e 'BEGIN {FPAT = "([^,]*)|(\"[^\"]+\")"}
FNR > 1 {
  gsub(/^\[|^"\[|\]"$|\]$/, "", $4)
  gsub(/\]\[/, "; ", $4)
  if (substr($4, 1, 7) == "RFC7231") {
    if ($2 == "yes")
    { safe_m = "Safe.    " }
    else
    { safe_m = "Not safe." }
    if ($3 == "yes")
    { indem_m = "Idempotent.    " }
    else
    { indem_m = "Not idempotent." }
    print "/* " safe_m " " indem_m " " $4 ". */"
    print "#define MHD_HTTP_METHOD_" toupper(gensub(/-/, "_", "g", $1)) " \""$1"\""
  }
}' methods.csv >> header_insert_methods.h && \
echo '
/* Additional HTTP methods. */' >> header_insert_methods.h && \
gawk -e 'BEGIN {FPAT = "([^,]*)|(\"[^\"]+\")"}
FNR > 1 {
  gsub(/^\[|^"\[|\]"$|\]$/, "", $4)
  gsub(/\]\[/, "; ", $4)
  if (substr($4, 1, 7) != "RFC7231") {
    if ($2 == "yes")
    { safe_m = "Safe.    " }
    else
    { safe_m = "Not safe." }
    if ($3 == "yes")
    { indem_m = "Idempotent.    " }
    else
    { indem_m = "Not idempotent." }
    print "/* " safe_m " " indem_m " " $4 ". */"
    print "#define MHD_HTTP_METHOD_" toupper(gensub(/-/, "_", "g", $1)) " \""$1"\""
  }
}' methods.csv >> header_insert_methods.h && \
echo OK && \
rm methods.csv || exit

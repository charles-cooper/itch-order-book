#!/bin/sh
for file in $(find . | grep -P "\.(cpp|h)$")
do
  echo $file
  clang-format -style="{ BasedOnStyle: Google , BreakBeforeBraces: Linux , NamespaceIndentation: All } " < $file > tmp
  mv tmp $file
done

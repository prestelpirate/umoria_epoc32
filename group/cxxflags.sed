/\/src\/[^ ]*\.cpp$/s#\$(GCC\(U\{0,1\}\)\(REL\|DEB\))#$(GCC\1\2) -fno-exceptions -fno-rtti -include PRELUDE#

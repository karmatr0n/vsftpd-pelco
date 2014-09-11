--- vsf_findlibs.sh.orig	2012-03-27 20:17:41.000000000 -0600
+++ vsf_findlibs.sh	2014-08-21 03:46:05.000000000 -0500
@@ -73,5 +73,9 @@
   echo "-lssl -lcrypto";
 fi
 
+if locate_library /usr/local/lib/libpcre.so; then
+  echo "-lpcre";
+fi
+
 exit 0;
 

--- sysutil.c.orig	2011-02-15 01:31:28.000000000 -0600
+++ sysutil.c	2011-04-26 12:31:10.000000000 -0500
@@ -965,9 +965,29 @@
 }
 
 int
-vsf_sysutil_mkdir(const char* p_dirname, const unsigned int mode)
+    vsf_sysutil_mkdir(const char* p_dirname, const unsigned int mode)
 {
-  return mkdir(p_dirname, mode);
+    /* IFUNAM  */
+    char tmp[strlen(p_dirname)+1];
+    char *p = NULL;
+    size_t len;
+    snprintf(tmp, sizeof(tmp),"%s",p_dirname);
+
+    len = strlen(tmp);
+    if(tmp[len - 1] == '/')
+        tmp[len - 1] = 0;
+
+    for(p = tmp +1; *p; p++) {
+        if(*p == '/') {
+            *p = 0;
+            mkdir(tmp, mode);
+            *p = '/';
+        }
+    }
+    p++;
+    mkdir(tmp, mode);
+    /* IFUNAM */
+    return 0;
 }

 int

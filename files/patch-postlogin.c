--- postlogin.c.orig	2012-09-18 04:59:37.000000000 -0500
+++ postlogin.c	2014-09-11 05:02:54.000000000 -0500
@@ -27,6 +27,16 @@
 #include "ssl.h"
 #include "vsftpver.h"
 #include "opts.h"
+/* Requirements of ORGANIZATION */
+#include <stdio.h>
+#include <sys/types.h>
+#include <string.h>
+#include <strings.h>
+#include <pcre.h>
+#include <sys/stat.h>
+#include <stdbool.h>
+#include <syslog.h>
+/* End of requirements of ORGANIZATION */
 
 /* Private local functions */
 static void handle_pwd(struct vsf_session* p_sess);
@@ -1003,6 +1013,93 @@
   handle_upload_common(p_sess, 0, 0);
 }
 
+/* ORGANIZATION Magic */
+char *
+extract_path(const char *root_dir, const char *filename) {
+    // 0140908110012
+    char *regex = "([0-9]{4})([0-9]{2})([0-9]{2})([0-9]{2})([0-9]{2})";
+    const char *error;
+    int   erroffset, rc;
+    int   ovector[200];
+
+    pcre *re = pcre_compile(regex, 0, &error, &erroffset, 0); 
+    if (!re)
+    	syslog(LOG_LOCAL0 | LOG_DEBUG, "pcre_compile failed (offset: %d), %s, %s\n", erroffset, error, filename);
+
+    // Building a new path based on the ip_address + fields from filename + \0
+    // Example: prefix_dir/132.248.7.154/2011/12/31/14/02
+    char *prefix_dir = "/var/ftp/cameras/";
+    unsigned int offset = 0;
+    size_t len = strlen(filename);
+    size_t path_len = strlen(root_dir) + 35;
+    char path[path_len];
+    size_t subdir_len;
+    strlcpy(path, prefix_dir, sizeof(path));
+    strlcat(path, root_dir, sizeof(path));
+    while (offset < len && (rc = pcre_exec(re, 0, filename, len, offset, 0, ovector, sizeof(ovector))) >= 0)
+    {   int i;
+        for(i = 1; i < rc; ++i)
+        {
+            strlcat(path, "/", sizeof(path));
+
+            subdir_len = (size_t)snprintf(NULL, 0,"%.*s",  ovector[2*i+1] - ovector[2*i], filename + ovector[2*i]);
+            char subdir[subdir_len];
+            snprintf(subdir, (subdir_len +1), "%.*s",  ovector[2*i+1] - ovector[2*i], filename + ovector[2*i]);
+            strlcat(path, subdir, sizeof(path));
+        }
+        offset = ovector[1];
+    }
+    pcre_free(re);
+
+    char *new_path = (char*)malloc(path_len * sizeof(char));
+    strcpy(new_path, path);
+
+    return new_path;
+}
+
+char *
+extract_filename(const char *current_filename) {
+  char *new_filename = (char*)malloc(6 * sizeof(char));
+    strcpy(new_filename, current_filename+12);
+    return new_filename;
+}
+
+// Verify if the filename is assigned with the pattern used by Pelco Cameras
+bool
+pelco_filename(const char *filename) {
+  // IMG20140908110012.jpg
+  char *regex = "^(\\/)?IMG"            // Prefix used by the pelco cameras
+                "([0-9]{4})"            // Year:   YYYY
+                "(0[1-9]|1[0-2])"       // Month:  mm
+                "(0[1-9]|1|2|3[0-9])"   // Day:    dd
+                "((0|1[0-9])|2[0-4])"   // Hour:   HH
+                "([0-5][0-9])"          // Min:    MM
+                "(\\d+)\\.jpg$";        // Seconds + Extension
+  const char *error;
+  int erroffset;
+
+  pcre *re = pcre_compile(regex, PCRE_CASELESS, &error, &erroffset, NULL);
+
+  if (!re)
+    syslog(LOG_LOCAL0 | LOG_DEBUG, "pcre_compile failed (offset: %d), %s, %s\n", erroffset, error, filename);
+
+  pcre_extra *reExtra = pcre_study(re, 0, &error);
+  if (error != NULL) 
+    syslog(LOG_LOCAL0 | LOG_DEBUG, "pcre_study failed: %s\n", error);
+
+  int ovector[30];
+  int rc = pcre_exec(re, reExtra, filename, strlen(filename), 0, 0, ovector, 30);
+  pcre_free(re);
+  pcre_free(reExtra);
+
+  if (rc < 0) 
+    return false; 
+  else
+    return true;
+}
+
+/* Ending of ORGANIZATION Magic */
+
 static void
 handle_upload_common(struct vsf_session* p_sess, int is_append, int is_unique)
 {
@@ -1021,13 +1118,43 @@
   {
     return;
   }
+
   resolve_tilde(&p_sess->ftp_arg_str, p_sess);
   p_filename = &p_sess->ftp_arg_str;
+
   if (is_unique)
   {
     get_unique_filename(&s_filename, p_filename);
     p_filename = &s_filename;
   }
+
+  /* ORGANIZATION Magic to save our *very special* files */
+
+  int mkdir_retval = 0;
+  int chdir_retval = 0;
+  static struct mystr q_filename;
+  str_copy(&q_filename, p_filename);
+
+  if (pelco_filename(str_getbuf(&q_filename))) {
+    str_replace_text(&q_filename, "IMG", "");
+    const char *remote_file = str_getbuf(&q_filename);
+    struct mystr new_file;
+    str_alloc_text(&new_file, extract_filename(remote_file));
+    str_copy(&p_sess->ftp_arg_str, &new_file);
+
+    const char *remote_ip = str_getbuf(&p_sess->remote_ip_str);
+    const char *path = extract_path(remote_ip, remote_file);
+
+    struct mystr new_path = INIT_MYSTR;
+    str_alloc_text(&new_path, path);
+
+    // Those variables will be used to send error messages to
+    // the client if we found troubles to create directories
+    // or to change to the new directory.
+    mkdir_retval = str_mkdir(&new_path, 0777);
+    chdir_retval = str_chdir(&new_path);
+  }
+
   vsf_log_start_entry(p_sess, kVSFLogEntryUpload);
   str_copy(&p_sess->log_str, &p_sess->ftp_arg_str);
   prepend_path_to_filename(&p_sess->log_str);
@@ -1036,6 +1163,20 @@
     vsf_cmdio_write(p_sess, FTP_NOPERM, "Permission denied.");
     return;
   }
+
+  /* ORGANIZATION Magic */
+  if (mkdir_retval != 0)
+  {
+    vsf_cmdio_write(p_sess, FTP_FILEFAIL, "Create directory operation failed");
+    return;
+  }
+
+  if (chdir_retval != 0)
+  {
+    vsf_cmdio_write(p_sess, FTP_FILEFAIL, "Failed to change directory.");
+  }
+  /* Ending of ORGANIZATION Magic */
+
   /* NOTE - actual file permissions will be governed by the tunable umask */
   /* XXX - do we care about race between create and chown() of anonymous
    * upload?

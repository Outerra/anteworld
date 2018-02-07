// headers from c4e, chromium repository version 102269

//needed changes: removed exports on abstract classes causing problems with delayed dll load
Index: v8.h
===================================================================
--- v8.h	(revision 3287)
+++ v8.h	(working copy)
@@ -1082,7 +1082,7 @@
    */
   V8EXPORT bool IsExternalAscii() const;
 
-  class V8EXPORT ExternalStringResourceBase {  // NOLINT
+  class ExternalStringResourceBase {  // NOLINT
    public:
     virtual ~ExternalStringResourceBase() {}
 
@@ -1111,7 +1111,7 @@
    * ExternalStringResource to manage the life cycle of the underlying
    * buffer.  Note that the string data must be immutable.
    */
-  class V8EXPORT ExternalStringResource
+  class ExternalStringResource
       : public ExternalStringResourceBase {
    public:
     /**
@@ -1145,7 +1145,7 @@
    * Use String::New or convert to 16 bit data for non-ASCII.
    */
 
-  class V8EXPORT ExternalAsciiStringResource
+  class ExternalAsciiStringResource
       : public ExternalStringResourceBase {
    public:
     /**

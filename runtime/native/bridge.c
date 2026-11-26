/* SPDX-License-Identifier: MIT
 * Uses only the public Mono embedding ABI exported by an already loaded module.
 * No memory scanning, runtime replacement, game file edits or loader bypass.
 */
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <sddl.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

static HMODULE self;
static void *domain, *thread, *execute;
static void *(__cdecl *root_domain)(void);
static void *(__cdecl *thread_attach)(void *);
static void (__cdecl *thread_detach)(void *);
static void *(__cdecl *assembly_open)(void *, const char *);
static void *(__cdecl *assembly_image)(void *);
static void *(__cdecl *class_from_name)(void *, const char *, const char *);
static void *(__cdecl *class_method)(void *, const char *, int);
static void *(__cdecl *string_new)(void *, const char *);
static void *(__cdecl *invoke)(void *, void *, void **, void **);
static char *(__cdecl *string_utf8)(void *);
static void (__cdecl *free_mono)(void *);

#define API(target, name) do { FARPROC p = GetProcAddress(module, name); \
 if (!p || sizeof(p) != sizeof(target)) return FALSE; \
 memcpy(&target, &p, sizeof(target)); } while (0)
static BOOL bind_api(HMODULE module) {
 API(root_domain,"mono_get_root_domain");
 API(thread_attach,"mono_thread_attach"); API(thread_detach,"mono_thread_detach");
 API(assembly_open,"mono_domain_assembly_open"); API(assembly_image,"mono_assembly_get_image");
 API(class_from_name,"mono_class_from_name"); API(class_method,"mono_class_get_method_from_name");
 API(string_new,"mono_string_new"); API(invoke,"mono_runtime_invoke");
 API(string_utf8,"mono_string_to_utf8"); API(free_mono,"mono_free");
 return TRUE;
}
static BOOL loaded_mono(void) {
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
 if(snapshot==INVALID_HANDLE_VALUE)return FALSE;
 MODULEENTRY32W item={0};item.dwSize=sizeof(item);BOOL found=FALSE;
 if(Module32FirstW(snapshot,&item))do {
  if(GetProcAddress(item.hModule,"mono_get_root_domain") && bind_api(item.hModule)){found=TRUE;break;}
 }while(Module32NextW(snapshot,&item));
 CloseHandle(snapshot);return found;
}
static void *open_sibling(const wchar_t *filename) {
 wchar_t path[32768];char utf8[131072];
 DWORD n=GetModuleFileNameW(self,path,32768);if(!n||n>32600)return NULL;
 wchar_t *slash=wcsrchr(path,L'\\');if(!slash)return NULL;wcscpy(slash+1,filename);
 if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,path,-1,utf8,sizeof(utf8),NULL,NULL))return NULL;
 return assembly_open(domain,utf8);
}
static const char *initialize(void) {
 if(execute){thread=thread_attach(domain);return thread?NULL:"MONO_ATTACH_FAILED";}
 if(!loaded_mono())return "MONO_EXPORTS_UNAVAILABLE";
 domain=root_domain();if(!domain)return "MONO_NOT_READY";
 if(!thread)thread=thread_attach(domain);
 if(!thread)return "MONO_ATTACH_FAILED";
 if(!open_sibling(L"0Harmony.dll"))return "HARMONY_LOAD_FAILED";
 void *assembly=open_sibling(L"Sora.Runtime.dll");if(!assembly)return "PAYLOAD_LOAD_FAILED";
 void *image=assembly_image(assembly);if(!image)return "PAYLOAD_IMAGE_FAILED";
 void *klass=class_from_name(image,"Sora.Runtime","Entry");if(!klass)return "ENTRY_NOT_FOUND";
 execute=class_method(klass,"Execute",1);return execute?NULL:"METHOD_NOT_FOUND";
}
static void command_attached(const char *request,char *response,size_t length) {
 if(!execute && !strcmp(request,"OFF")){snprintf(response,length,"OFF");return;}
 const char *error=initialize();if(error){snprintf(response,length,"%s",error);return;}
 void *text=string_new(domain,request), *exception=NULL, *args[1]={text};
 if(!text){snprintf(response,length,"ALLOCATION_FAILED");return;}
 void *answer=invoke(execute,NULL,args,&exception);
 if(exception || !answer){snprintf(response,length,"MANAGED_EXCEPTION");return;}
 char *utf8=string_utf8(answer);snprintf(response,length,"%s",utf8?utf8:"INVALID_RESPONSE");
 if(utf8)free_mono(utf8);
}
static void command(const char *request,char *response,size_t length) {
 command_attached(request,response,length);
 // Never leave an embedding thread attached while blocking on a native pipe:
 // cooperative Mono collectors can otherwise wait forever for that thread.
 if(thread){thread_detach(thread);thread=NULL;}
}
static BOOL pipe_security(SECURITY_ATTRIBUTES *security,PSECURITY_DESCRIPTOR *descriptor) {
 HANDLE token=NULL;DWORD count=0;
 if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return FALSE;
 GetTokenInformation(token,TokenUser,NULL,0,&count);
 TOKEN_USER *user=HeapAlloc(GetProcessHeap(),0,count);
 BOOL ok=user && GetTokenInformation(token,TokenUser,user,count,&count);
 wchar_t *sid=NULL, sddl[512];
 if(ok)ok=ConvertSidToStringSidW(user->User.Sid,&sid);
 if(ok){swprintf(sddl,512,L"O:%lsD:P(A;;GA;;;%ls)",sid,sid);
  ok=ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl,SDDL_REVISION_1,descriptor,NULL);}
 if(sid)LocalFree(sid);if(user)HeapFree(GetProcessHeap(),0,user);CloseHandle(token);
 if(!ok)return FALSE;
 *security=(SECURITY_ATTRIBUTES){sizeof(*security),*descriptor,FALSE};return TRUE;
}
static DWORD WINAPI serve(void *unused) {
 (void)unused;
 SECURITY_ATTRIBUTES security;PSECURITY_DESCRIPTOR descriptor=NULL;
 if(!pipe_security(&security,&descriptor))return 1;
 wchar_t name[128];swprintf(name,128,L"\\\\.\\pipe\\SoraWW.Runtime.1.%lu",GetCurrentProcessId());
 HANDLE pipe=CreateNamedPipeW(name,PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE,
  PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,
  1,256,32,0,&security);
 LocalFree(descriptor);if(pipe==INVALID_HANDLE_VALUE)return 2;
 for(;;){
  BOOL connected=ConnectNamedPipe(pipe,NULL)||GetLastError()==ERROR_PIPE_CONNECTED;
  if(connected){
   for(;;){char request[16]={0}, response[192]={0};DWORD bytes=0,written=0;
    if(!ReadFile(pipe,request,sizeof(request)-1,&bytes,NULL)||!bytes)break;
    request[bytes]=0;
    if(strcmp(request,"ON")&&strcmp(request,"OFF"))snprintf(response,sizeof(response),"INVALID_COMMAND");
    else command(request,response,sizeof(response));
    if(!WriteFile(pipe,response,(DWORD)strlen(response),&written,NULL))break;
   }
  }
  char ignored[192];command("OFF",ignored,sizeof(ignored));
  DisconnectNamedPipe(pipe);
  // Keep the native worker for reconnect, detached from the managed runtime.
 }
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void *reserved) {
 (void)reserved;
 if(reason==DLL_PROCESS_ATTACH){self=module;DisableThreadLibraryCalls(module);
  HANDLE worker=CreateThread(NULL,0,serve,NULL,0,NULL);if(!worker)return FALSE;CloseHandle(worker);}
 return TRUE;
}

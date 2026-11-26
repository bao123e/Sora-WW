/* SPDX-License-Identifier: MIT */
#ifndef SORA_LIVE_H
#define SORA_LIVE_H
static HANDLE live_process,live_job;
static DWORD live_sequence,live_pid;
static BOOL live_enabled,live_pending;
static ULONGLONG live_started,live_last_update;
static FILETIME live_write;
static wchar_t live_state[32768],live_result[32768],live_command[32768];

static void live_controls(void){
 if(!ui_ready)return;
 SetWindowTextW(ui_general_controls[4],live_pending?ui_text(L"Đang áp dụng…",L"Applying…"):live_enabled?ui_text(L"Tắt",L"Turn off"):ui_text(L"Bật trong game",L"Enable in session"));
 EnableWindow(ui_general_controls[4],!live_pending&&!trial_process&&matches==1);
}
static void live_shutdown(void){
 if(live_job){CloseHandle(live_job);live_job=NULL;}
 if(live_process){CloseHandle(live_process);live_process=NULL;}
 live_enabled=live_pending=FALSE;live_pid=0;
}
static BOOL live_request(BOOL enabled){
 wchar_t text[64];swprintf(text,64,L"%lu:%u",(unsigned long)++live_sequence,enabled?1u:0u);
 return WritePrivateProfileStringW(L"Live",L"Request",text,live_command);
}
static void live_fail(const wchar_t *code){
 live_shutdown();swprintf(live_notice,256,ui_text(L"No Cooldown chưa áp dụng: %ls",L"No Cooldown was not applied: %ls"),code);
 live_controls();if(ui_ready)InvalidateRect(ui_body,NULL,FALSE);
}
static void live_toggle(void){
 if(live_pending)return;
 if(matches!=1||!game_pid||!valid_game_path(game_path)){live_fail(L"GAME_SESSION_REQUIRED");return;}
 if(!trial_state_dir(live_state)||swprintf(live_result,32768,L"%ls\\live-result.ini",live_state)<0||swprintf(live_command,32768,L"%ls\\live-command.ini",live_state)<0){live_fail(L"STATE_DIRECTORY_FAILED");return;}
 if(live_process&&live_pid!=game_pid)live_shutdown();
 if(!live_request(!live_enabled)){live_fail(L"COMMAND_WRITE_FAILED");return;}
 if(!live_process){
  wchar_t helper[32768],command[100000];DWORD n=GetModuleFileNameW(NULL,helper,32768);
  if(!n||n>32600){live_fail(L"HELPER_MISSING");return;}
  wchar_t *slash=wcsrchr(helper,L'\\');if(!slash){live_fail(L"HELPER_MISSING");return;}wcscpy(slash+1,L"Sora-Live.exe");
  if(GetFileAttributesW(helper)==INVALID_FILE_ATTRIBUTES){live_fail(L"EXTRACT_COMPLETE_RUNTIME_ZIP");return;}
  DeleteFileW(live_result);ZeroMemory(&live_write,sizeof(live_write));
  if(swprintf(command,100000,L"\"%ls\" --pid %lu --game \"%ls\" --parent %lu --state \"%ls\"",helper,(unsigned long)game_pid,game_path,(unsigned long)GetCurrentProcessId(),live_state)<0){live_fail(L"PATH_TOO_LONG");return;}
  live_job=CreateJobObjectW(NULL,NULL);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={0};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if(!live_job||!SetInformationJobObject(live_job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){live_fail(L"HELPER_START_FAILED");return;}
  STARTUPINFOW startup={0};startup.cb=sizeof(startup);PROCESS_INFORMATION process={0};
  if(!CreateProcessW(helper,command,NULL,NULL,FALSE,CREATE_NO_WINDOW|CREATE_SUSPENDED,NULL,NULL,&startup,&process)){live_fail(L"HELPER_START_FAILED");return;}
  if(!AssignProcessToJobObject(live_job,process.hProcess)){TerminateProcess(process.hProcess,1);CloseHandle(process.hThread);CloseHandle(process.hProcess);live_fail(L"HELPER_START_FAILED");return;}
  live_process=process.hProcess;live_pid=game_pid;
  if(ResumeThread(process.hThread)==(DWORD)-1){CloseHandle(process.hThread);live_fail(L"HELPER_START_FAILED");return;}CloseHandle(process.hThread);
 }
 live_pending=TRUE;live_started=live_last_update=GetTickCount64();
 wcscpy(live_notice,ui_text(L"Đang nối với phiên game…",L"Connecting to the running session…"));live_controls();InvalidateRect(ui_body,NULL,FALSE);
}
static void live_poll(void){
 if(!live_process){live_controls();return;}
 if(!game_pid||live_pid!=game_pid){live_fail(L"SESSION_ENDED");return;}
 WIN32_FILE_ATTRIBUTE_DATA file;
 if(GetFileAttributesExW(live_result,GetFileExInfoStandard,&file)&&CompareFileTime(&file.ftLastWriteTime,&live_write)!=0){
  live_write=file.ftLastWriteTime;live_last_update=GetTickCount64();
  DWORD sequence=GetPrivateProfileIntW(L"Live",L"Sequence",0,live_result);
  if(sequence==live_sequence){
   wchar_t code[160];GetPrivateProfileStringW(L"Live",L"Code",L"NO_RESULT",code,160,live_result);
   if(!wcscmp(code,L"OFF")){live_shutdown();wcscpy(live_notice,ui_text(L"No Cooldown đã tắt; dùng xử lý gốc của game.",L"No Cooldown is off; the game's original behavior is active."));}
   else if(!wcsncmp(code,L"HOOKS_READY:",12)){
    live_pending=FALSE;live_enabled=TRUE;
    swprintf(live_notice,256,ui_text(L"Đã gắn hook No Cooldown · %ls\nHãy thử kỹ năng; hiệu lực thực tế chưa được xác nhận.",L"No Cooldown hooks active · %ls\nTry a skill; gameplay effects remain unverified."),code+12);
   }else{live_fail(code);return;}
   live_controls();InvalidateRect(ui_body,NULL,FALSE);
  }
 }
 if(live_process&&WaitForSingleObject(live_process,0)==WAIT_OBJECT_0){live_fail(L"CONTROLLER_EXITED");return;}
 if(live_process&&((live_pending&&GetTickCount64()-live_started>20000)||GetTickCount64()-live_last_update>20000))live_fail(L"RUNTIME_RESPONSE_TIMEOUT");
 live_controls();
}
#endif

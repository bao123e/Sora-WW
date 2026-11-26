/* SPDX-License-Identifier: MIT */
#ifndef WUWA_TRIAL_H
#define WUWA_TRIAL_H
static HANDLE trial_process,trial_job;
static BOOL trial_then_launch;
static ULONGLONG trial_started;
static wchar_t trial_result[32768];

static BOOL trial_state_dir(wchar_t *out){
 if(!config_path[0])return FALSE;wcscpy(out,config_path);wchar_t *slash=wcsrchr(out,L'\\');if(!slash)return FALSE;*slash=0;return TRUE;
}
static void trial_busy_ui(BOOL busy){
 if(!ui_ready)return;for(int i=0;i<7;i++)EnableWindow(ui_general_controls[i],!busy&&i!=4&&i!=5);InvalidateRect(ui_body,NULL,FALSE);
}
static void trial_error(const wchar_t *code){
 const wchar_t *message;
 if(!wcscmp(code,L"CLOSE_GAME_FIRST"))message=ui_text(L"Đóng game trước khi chuẩn bị hoặc hoàn tác gói thử.",L"Close the game before preparing or removing the trial package.");
 else if(!wcscmp(code,L"TRIAL_SUSPENDED"))message=ui_text(L"Đã dừng bản vá 0.4 sau báo cáo văng game. Hãy dùng Hoàn tác.",L"The 0.4 patch is suspended after a reported crash. Use Remove trial.");
 else if(!wcscmp(code,L"UNSUPPORTED_BUILD")||!wcscmp(code,L"UNSUPPORTED_ASSEMBLY"))message=ui_text(L"Bản game đã khác gói 3.8 đã kiểm tra. Chưa áp dụng bản vá.",L"This build differs from the inspected 3.8 package. No patch was applied.");
 else if(!wcscmp(code,L"UNOWNED_OR_CHANGED_PACKAGE"))message=ui_text(L"Gói thử có sẵn không khớp dữ liệu tool. Không tự ghi đè hoặc xóa.",L"The existing trial package is untracked or changed. It was left intact.");
 else if(!wcscmp(code,L"OTHER_INSTALL_ACTIVE"))message=ui_text(L"Hãy Hoàn tác gói thử cũ trước khi đổi sang bản cài đặt khác.",L"Remove the previous trial package before using another installation.");
 else if(!wcscmp(code,L"INVALID_GAME_PATH"))message=ui_text(L"Chọn Client\\Binaries\\Win64\\Client-Win64-Shipping.exe.",L"Choose Client\\Binaries\\Win64\\Client-Win64-Shipping.exe.");
 else message=ui_text(L"Không chuẩn bị được gói thử. Kiểm tra quyền ghi hoặc dùng Hoàn tác.",L"Could not prepare the trial package. Check write access or use Remove trial.");
 swprintf(launch_notice,256,L"%ls [%ls]",message,code);
}
static BOOL trial_start(BOOL then_launch){
 if(trial_process){wcscpy(launch_notice,ui_text(L"Đang chuẩn bị; hãy chờ kết quả.",L"Preparation is in progress; please wait."));return TRUE;}
 int state_check=game_process_state();if(state_check!=0){trial_error(state_check<0?L"PROCESS_CHECK_FAILED":L"CLOSE_GAME_FIRST");return TRUE;}
 wchar_t state[32768],helper[32768],command[66560];
 if(!trial_state_dir(state)||!game_path[0]){trial_error(L"INVALID_GAME_PATH");return TRUE;}
 DWORD n=GetModuleFileNameW(NULL,helper,32768);if(!n||n>=32700){trial_error(L"HELPER_MISSING");return TRUE;}
 wchar_t *slash=wcsrchr(helper,L'\\');if(!slash){trial_error(L"HELPER_MISSING");return TRUE;}wcscpy(slash+1,L"Sora-Gameplay.exe");
 if(GetFileAttributesW(helper)==INVALID_FILE_ATTRIBUTES){wcscpy(launch_notice,ui_text(L"Thiếu Sora-Gameplay.exe. Hãy giải nén đầy đủ ZIP mới.",L"Sora-Gameplay.exe is missing. Extract the complete new ZIP."));return TRUE;}
 if(swprintf(trial_result,32768,L"%ls\\gameplay-result.ini",state)<0){trial_error(L"PATH_TOO_LONG");return TRUE;}DeleteFileW(trial_result);
 if(swprintf(command,66560,L"\"%ls\" --exe \"%ls\" --state \"%ls\"",helper,game_path,state)<0){trial_error(L"PATH_TOO_LONG");return TRUE;}
 trial_job=CreateJobObjectW(NULL,NULL);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={0};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
 if(!trial_job||!SetInformationJobObject(trial_job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){if(trial_job)CloseHandle(trial_job);trial_job=NULL;trial_error(L"HELPER_START_FAILED");return TRUE;}
 STARTUPINFOW startup={0};startup.cb=sizeof(startup);PROCESS_INFORMATION process={0};
 if(!CreateProcessW(helper,command,NULL,NULL,FALSE,CREATE_NO_WINDOW|CREATE_SUSPENDED,NULL,NULL,&startup,&process)){CloseHandle(trial_job);trial_job=NULL;trial_error(L"HELPER_START_FAILED");return TRUE;}
 if(!AssignProcessToJobObject(trial_job,process.hProcess)){TerminateProcess(process.hProcess,1);CloseHandle(process.hThread);CloseHandle(process.hProcess);CloseHandle(trial_job);trial_job=NULL;trial_error(L"HELPER_START_FAILED");return TRUE;}
 trial_process=process.hProcess;trial_then_launch=then_launch;trial_started=GetTickCount64();
 if(ResumeThread(process.hThread)==(DWORD)-1){CloseHandle(process.hThread);trial_shutdown();trial_error(L"HELPER_START_FAILED");return TRUE;}CloseHandle(process.hThread);
 wcscpy(launch_notice,ui_text(L"Đang kiểm tra và chuẩn bị gói thử… Menu vẫn dùng được.",L"Checking and preparing the trial package… The menu remains responsive."));trial_busy_ui(TRUE);return TRUE;
}
static BOOL trial_before_launch(void){
 if(trial_process)return trial_start(TRUE);
 wchar_t state[32768],manifest[32768],target[32768];
 if(trial_state_dir(state)&&swprintf(manifest,32768,L"%ls\\gameplay-trial.json",state)>0&&GetFileAttributesW(manifest)!=INVALID_FILE_ATTRIBUTES)return trial_start(TRUE);
 wchar_t base[32768];wcscpy(base,game_path);wchar_t *slash=wcsrchr(base,L'\\');if(slash)*slash=0;
 if(base[0]&&swprintf(target,32768,L"%ls\\..\\..\\Content\\Paks\\zz_SoraWW_Experimental_P.pak",base)>0&&GetFileAttributesW(target)!=INVALID_FILE_ATTRIBUTES)return trial_start(TRUE);

 return FALSE;
}
static void trial_shutdown(void){
 if(trial_job){CloseHandle(trial_job);trial_job=NULL;}if(trial_process){CloseHandle(trial_process);trial_process=NULL;}
}
static void trial_poll(void){
 if(!trial_process)return;
 if(GetTickCount64()-trial_started>240000){trial_shutdown();trial_busy_ui(FALSE);trial_error(L"PREPARATION_TIMEOUT");return;}
 if(WaitForSingleObject(trial_process,0)!=WAIT_OBJECT_0)return;
 DWORD exit=1;GetExitCodeProcess(trial_process,&exit);trial_shutdown();trial_busy_ui(FALSE);
 wchar_t code[80];GetPrivateProfileStringW(L"Trial",L"Code",L"NO_RESULT",code,80,trial_result);
 if(exit||(wcscmp(code,L"PREPARED_UNVERIFIED")&&wcscmp(code,L"REMOVED"))){trial_error(code);return;}
 if(!wcscmp(code,L"REMOVED"))wcscpy(launch_notice,ui_text(L"Đã hoàn tác gói thử do tool tạo.",L"The tool's trial package has been removed."));
 else wcscpy(launch_notice,ui_text(L"Đã chuẩn bị gói thử. Hiệu lực trong game chưa được xác nhận.",L"Trial package prepared. In-game effects remain unverified."));
 if(trial_then_launch){if(game_process_state()==0&&valid_game_path(game_path))launch_game_direct();else trial_error(L"CLOSE_GAME_FIRST");}
 if(ui_ready)InvalidateRect(ui_body,NULL,FALSE);
}
static BOOL trial_save(BOOL cd,BOOL damage){
 wchar_t value[8];swprintf(value,8,L"%u",(cd?1u:0u)|(damage?2u:0u));return config_path[0]&&WritePrivateProfileStringW(L"GameTrial",L"Flags",value,config_path);
}
static void trial_toggle(int id){
 (void)id;if(trial_process)return;trial_cd=trial_damage=FALSE;trial_save(FALSE,FALSE);trial_error(L"TRIAL_SUSPENDED");ui_apply_language();
 if(ui_ready)InvalidateRect(ui_body,NULL,FALSE);
}
static void trial_remove(void){
 if(trial_process)return;if(game_process_state()!=0){trial_error(L"CLOSE_GAME_FIRST");if(ui_ready)InvalidateRect(ui_body,NULL,FALSE);return;}
 if(!trial_save(FALSE,FALSE)){trial_error(L"SETTINGS_SAVE_FAILED");return;}trial_cd=trial_damage=FALSE;ui_apply_language();trial_start(FALSE);
}
#endif

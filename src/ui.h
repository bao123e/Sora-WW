/* SPDX-License-Identifier: MIT */
#ifndef WUWA_UI_H
#define WUWA_UI_H
#define UI_TRIAL_CD 240
#define UI_TRIAL_DAMAGE 241
#define UI_TRIAL_REMOVE 242
#define UI_BROWSE_GAME 230
#define UI_LAUNCH_GAME 231
#define UI_GENERAL 200
#define UI_KEYS 201
#define UI_PIN 202
#define UI_HIDE 203
#define UI_CHANGE_KEY 204
#define UI_LANGUAGE 205
#define UI_OPACITY 206
#define UI_VI 210
#define UI_EN 211
#define UI_ALPHA_85 220
#define UI_ALPHA_90 221
#define UI_ALPHA_94 222
#define UI_ALPHA_100 223
static int ui_language,ui_tab,ui_scroll[2],ui_opacity=94;
static BOOL ui_pinned=TRUE,ui_ready,ui_dragging;
static int ui_drag_start,ui_drag_offset;
static HWND ui_body,ui_hover,ui_buttons[10];
static HWND ui_general_controls[7],ui_key_controls[4];
static HBRUSH ui_field_brush,ui_status_brush;
static HFONT ui_small_font;
static WNDPROC ui_original_button;
static const wchar_t *ui_text(const wchar_t *vi,const wchar_t *en){return ui_language?en:vi;}
static COLORREF ui_bg(void){return RGB(14,16,19);}
static void ui_fill(HDC dc,RECT rect,COLORREF color){HBRUSH brush=CreateSolidBrush(color);FillRect(dc,&rect,brush);DeleteObject(brush);}
static void ui_round(HDC dc,RECT rect,COLORREF color,int radius){HBRUSH b=CreateSolidBrush(color);HPEN p=CreatePen(PS_SOLID,1,color);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,rect.left,rect.top,rect.right,rect.bottom,radius,radius);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);}
static void ui_label(HDC dc,const wchar_t *text,RECT rect,COLORREF color,UINT flags){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,text,-1,&rect,flags);}
static void ui_section(HDC dc,const wchar_t *text,int y,int width){
 RECT label={0,y,width,y+24};SIZE size;GetTextExtentPoint32W(dc,text,(int)wcslen(text),&size);ui_label(dc,text,label,RGB(225,226,229),DT_LEFT|DT_SINGLELINE|DT_VCENTER);
 HPEN pen=CreatePen(PS_SOLID,2,RGB(61,64,72));HGDIOBJ old=SelectObject(dc,pen);MoveToEx(dc,size.cx+12,y+12,NULL);LineTo(dc,width,y+12);SelectObject(dc,old);DeleteObject(pen);
}
static int ui_content_height(void){return ui_tab?470:1136;}
static int ui_scroll_max(void){RECT r;GetClientRect(ui_body,&r);int maximum=ui_content_height()-r.bottom;return maximum>0?maximum:0;}
static void ui_layout(void);
static void ui_scroll_to(int next){int maximum=ui_scroll_max();if(next<0)next=0;if(next>maximum)next=maximum;if(next!=ui_scroll[ui_tab]){ui_scroll[ui_tab]=next;ui_layout();InvalidateRect(ui_body,NULL,FALSE);}}
static void ui_thumb(RECT *track,RECT *thumb){
 RECT r;GetClientRect(ui_body,&r);*track=(RECT){r.right-12,4,r.right-2,r.bottom-4};int height=track->bottom-track->top;
 int length=(r.bottom*height)/ui_content_height();if(length<32)length=32;if(length>height)length=height;
 int maximum=ui_scroll_max();int offset=maximum?ui_scroll[ui_tab]*(height-length)/maximum:0;
 *thumb=(RECT){track->left,track->top+offset,track->right,track->top+offset+length};
}
static void ui_draw_body(HDC dc){
 RECT r;GetClientRect(ui_body,&r);ui_fill(dc,r,ui_bg());SelectObject(dc,font);int saved=SaveDC(dc);IntersectClipRect(dc,0,0,r.right-18,r.bottom);SetViewportOrgEx(dc,16,14-ui_scroll[ui_tab],NULL);int width=r.right-46;
 if(!ui_tab){
  ui_section(dc,ui_text(L"Kết nối game",L"Game connection"),0,width);
  RECT panel={0,36,width,98};ui_round(dc,panel,RGB(22,26,32),6);
  COLORREF dot=matches==1?RGB(72,198,145):unverified||matches>1?RGB(239,180,72):RGB(122,132,146);
  ui_round(dc,(RECT){12,49,20,57},dot,8);
  ui_label(dc,ui_text(L"Tiến trình",L"Process"),(RECT){0,112,170,138},RGB(145,150,159),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,matches==1?L"Client-Win64-Shipping.exe":L"—",(RECT){178,112,width,138},RGB(223,226,232),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
  ui_label(dc,ui_text(L"Chế độ",L"Mode"),(RECT){0,150,170,176},RGB(145,150,159),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_text(L"Mở game và nhận diện cửa sổ",L"Launch and window detection"),(RECT){178,150,width,176},RGB(223,226,232),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,game_path[0]?game_path:ui_text(L"Chưa chọn EXE",L"No EXE selected"),(RECT){0,236,width,264},RGB(165,174,188),DT_SINGLELINE|DT_VCENTER|DT_PATH_ELLIPSIS);
  ui_label(dc,launch_notice,(RECT){0,274,width,328},RGB(185,204,225),DT_WORDBREAK);
  ui_section(dc,ui_text(L"Khôi phục bản thử 0.4",L"Recover from the 0.4 trial"),348,width);
  ui_label(dc,L"No Cooldown",(RECT){0,388,178,418},RGB(206,216,232),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_text(L"Chặn sát thương",L"Local damage guard"),(RECT){0,436,178,466},RGB(206,216,232),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,L"Damage Multiplier",(RECT){0,484,178,514},RGB(128,134,145),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_text(L"Chưa hỗ trợ",L"Unavailable"),(RECT){178,484,width,514},RGB(145,150,159),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_text(L"Bản vá 0.4 đã tạm dừng sau báo cáo văng game.\nĐóng game, bấm Khôi phục / Mở game để gỡ gói cũ trước khi mở.",L"The 0.4 patch is suspended after a reported crash.\nClose the game, then Recover / Open game removes the old package first."),(RECT){0,568,width,632},RGB(233,179,60),DT_WORDBREAK);
  ui_section(dc,ui_text(L"Giao diện",L"Appearance"),666,width);
  ui_label(dc,ui_text(L"Độ đậm menu",L"Menu opacity"),(RECT){0,706,170,736},RGB(145,150,159),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_text(L"Luôn ở trên cùng",L"Always on top"),(RECT){0,754,170,782},RGB(145,150,159),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_pinned?ui_text(L"Đang ghim",L"Pinned"):ui_text(L"Không ghim",L"Unpinned"),(RECT){178,754,width,782},RGB(223,226,232),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_text(L"Dùng nút Ghim trên thanh tiêu đề để thay đổi.",L"Use Pin in the title bar to change this."),(RECT){0,796,width,822},RGB(128,134,145),DT_SINGLELINE|DT_VCENTER);
  ui_section(dc,ui_text(L"Điều khiển",L"Controls"),854,width);
  ui_label(dc,ui_text(L"Phím mở / ẩn menu",L"Show / hide menu"),(RECT){0,894,178,922},RGB(145,150,159),DT_SINGLELINE|DT_VCENTER);
  wchar_t binding[128];key_name(menu_key,binding,128);ui_label(dc,binding,(RECT){178,894,width,922},RGB(225,230,239),DT_SINGLELINE|DT_VCENTER);
  ui_section(dc,ui_text(L"Trạng thái thử nghiệm",L"Trial status"),998,width);
  ui_label(dc,ui_text(L"Hiệu lực trong game chưa được xác nhận.",L"In-game effects have not been verified."),(RECT){0,1034,width,1062},RGB(233,179,60),DT_SINGLELINE|DT_VCENTER);
  ui_label(dc,ui_text(L"Nút × chỉ ẩn menu. Chọn Thoát để đóng ứng dụng.\nCác thiết lập được lưu cho lần mở tiếp theo.",L"The × button hides the menu. Use Exit to close the app.\nYour preferences are remembered for the next launch."),(RECT){0,1072,width,1120},RGB(145,150,159),DT_WORDBREAK);
 }else{
  ui_section(dc,ui_text(L"Phím menu",L"Menu shortcut"),0,width);
  ui_label(dc,ui_text(L"Hiện / ẩn menu",L"Show / hide menu"),(RECT){0,50,172,82},RGB(165,170,178),DT_SINGLELINE|DT_VCENTER);
  ui_section(dc,ui_text(L"Cách sử dụng",L"How to use"),260,width);
  ui_label(dc,ui_text(L"1. Bấm ô phím rồi nhấn phím hoặc tổ hợp bạn muốn.\n2. Có thể kết hợp Ctrl, Alt hoặc Shift.\n3. Bấm Lưu để áp dụng và giữ cho lần mở sau.\n\nMặc định điền lại F1; chỉ áp dụng sau khi bấm Lưu.\nPhím đang được dùng sẽ không thay thế phím cũ.",L"1. Click the shortcut field and press your key combination.\n2. Ctrl, Alt and Shift modifiers are supported.\n3. Save applies the shortcut and remembers it.\n\nDefault fills in F1; Save is required to apply it.\nUnavailable shortcuts leave your existing shortcut intact."),(RECT){0,302,width,458},RGB(156,163,174),DT_WORDBREAK);
 }
 RestoreDC(dc,saved);if(ui_scroll_max()>0){RECT track,thumb;ui_thumb(&track,&thumb);ui_fill(dc,track,RGB(9,10,12));ui_round(dc,thumb,ui_dragging?RGB(126,131,138):RGB(77,80,86),8);}
}
static void ui_paint_body(HWND hwnd,HDC supplied){
 PAINTSTRUCT paint;HDC target=supplied?supplied:BeginPaint(hwnd,&paint);RECT r;GetClientRect(hwnd,&r);HDC memory=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,r.right>0?r.right:1,r.bottom>0?r.bottom:1);HGDIOBJ old=SelectObject(memory,bitmap);ui_draw_body(memory);BitBlt(target,0,0,r.right,r.bottom,memory,0,0,SRCCOPY);SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);if(!supplied)EndPaint(hwnd,&paint);
}
static void ui_main_paint(HWND hwnd,HDC supplied){
 PAINTSTRUCT paint;HDC target=supplied?supplied:BeginPaint(hwnd,&paint);RECT r;GetClientRect(hwnd,&r);HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,r.right>0?r.right:1,r.bottom>0?r.bottom:1);HGDIOBJ old=SelectObject(dc,bitmap);
 ui_fill(dc,r,ui_bg());ui_fill(dc,(RECT){0,0,r.right,30},RGB(42,72,111));SelectObject(dc,font);
 ui_label(dc,L"Sora WW  0.4.1",(RECT){32,0,r.right-125,30},RGB(242,246,251),DT_SINGLELINE|DT_VCENTER);
 HPEN logo=CreatePen(PS_SOLID,2,RGB(220,236,255));HGDIOBJ previous=SelectObject(dc,logo);MoveToEx(dc,9,10,NULL);LineTo(dc,13,21);LineTo(dc,17,12);LineTo(dc,21,21);LineTo(dc,25,10);SelectObject(dc,previous);DeleteObject(logo);
 HPEN separator=CreatePen(PS_SOLID,1,RGB(59,62,69));previous=SelectObject(dc,separator);MoveToEx(dc,14,80,NULL);LineTo(dc,r.right-14,80);MoveToEx(dc,14,r.bottom-50,NULL);LineTo(dc,r.right-14,r.bottom-50);SelectObject(dc,previous);DeleteObject(separator);
 BitBlt(target,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);if(!supplied)EndPaint(hwnd,&paint);
}
static void ui_paint_hotkey(HWND hwnd,HDC supplied){
 PAINTSTRUCT paint;HDC dc=supplied?supplied:BeginPaint(hwnd,&paint);RECT r;GetClientRect(hwnd,&r);ui_fill(dc,r,RGB(29,47,69));
 struct WKey key=word_key((WORD)SendMessageW(hwnd,HKM_GETHOTKEY,0,0));wchar_t text[160];
 if(key.vk)key_name(key,text,160);else swprintf(text,160,L"%ls%ls%ls%ls",key.mods&MOD_CONTROL?L"Ctrl+":L"",key.mods&MOD_ALT?L"Alt+":L"",key.mods&MOD_SHIFT?L"Shift+":L"",ui_text(L"Bấm phím…",L"Press a key…"));
 SelectObject(dc,font);ui_label(dc,text,(RECT){12,0,r.right-8,r.bottom},RGB(235,240,248),DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
 if(GetFocus()==hwnd){HPEN pen=CreatePen(PS_SOLID,1,RGB(51,148,236));HGDIOBJ old=SelectObject(dc,pen),brush=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,0,0,r.right,r.bottom);SelectObject(dc,old);SelectObject(dc,brush);DeleteObject(pen);}
 if(!supplied)EndPaint(hwnd,&paint);
}
static BOOL ui_draw_button(DRAWITEMSTRUCT *item){
 if(item->CtlType!=ODT_BUTTON)return FALSE;int id=(int)item->CtlID;RECT r=item->rcItem;HDC dc=item->hDC;BOOL hover=ui_hover==item->hwndItem,down=(item->itemState&ODS_SELECTED)!=0;
 COLORREF base=RGB(30,51,77),text=RGB(229,234,242);ui_fill(dc,r,id==UI_PIN||id==UI_HIDE?RGB(42,72,111):ui_bg());
 if(id==UI_GENERAL||id==UI_KEYS){BOOL active=(id==UI_KEYS)==(ui_tab==1);text=active?RGB(232,237,245):RGB(145,150,159);if(hover)ui_fill(dc,r,RGB(23,28,36));if(active)ui_fill(dc,(RECT){0,r.bottom-2,r.right,r.bottom},RGB(43,153,242));}
 else {if(id==UI_PIN)base=ui_pinned?RGB(48,105,168):RGB(48,76,111);if(id==UI_HIDE)base=hover?RGB(185,62,75):RGB(42,72,111);else if(down)base=RGB(28,69,111);else if(hover)base=RGB(44,79,119);RECT inside={r.left+1,r.top+1,r.right-1,r.bottom-1};ui_round(dc,inside,base,6);}
 wchar_t label[160];GetWindowTextW(item->hwndItem,label,160);SelectObject(dc,id==UI_HIDE?title_font:font);BOOL dropdown=id==UI_LANGUAGE||id==UI_OPACITY;
 RECT label_rect=r;if(dropdown){label_rect.left+=12;label_rect.right-=36;}ui_label(dc,label,label_rect,text,(dropdown?DT_LEFT:DT_CENTER)|DT_SINGLELINE|DT_VCENTER);
 if(dropdown){ui_fill(dc,(RECT){r.right-32,1,r.right-1,r.bottom-1},RGB(38,77,121));POINT arrow[]={{r.right-24,r.bottom/2-4},{r.right-10,r.bottom/2-4},{r.right-17,r.bottom/2+5}};HBRUSH brush=CreateSolidBrush(RGB(239,245,252));HGDIOBJ b=SelectObject(dc,brush),p=SelectObject(dc,GetStockObject(NULL_PEN));Polygon(dc,arrow,3);SelectObject(dc,b);SelectObject(dc,p);DeleteObject(brush);}
 if(item->itemState&ODS_FOCUS){RECT focus={4,4,r.right-4,r.bottom-4};DrawFocusRect(dc,&focus);}return TRUE;
}
static LRESULT CALLBACK ui_button_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_MOUSEMOVE){if(ui_hover!=hwnd){HWND previous=ui_hover;ui_hover=hwnd;if(previous)InvalidateRect(previous,NULL,FALSE);InvalidateRect(hwnd,NULL,FALSE);}TRACKMOUSEEVENT track={sizeof(track),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);}
 if(msg==WM_MOUSELEAVE&&ui_hover==hwnd){ui_hover=NULL;InvalidateRect(hwnd,NULL,FALSE);}
 return CallWindowProcW(ui_original_button,hwnd,msg,wp,lp);
}
static HWND ui_child(HWND parent,const wchar_t *klass,const wchar_t *text,DWORD style,int id){
 HWND hwnd=CreateWindowExW(0,klass,text,WS_CHILD|WS_VISIBLE|style,0,0,1,1,parent,(HMENU)(INT_PTR)id,instance,NULL);SendMessageW(hwnd,WM_SETFONT,(WPARAM)font,TRUE);
 if(!wcscmp(klass,L"BUTTON")){WNDPROC previous=(WNDPROC)SetWindowLongPtrW(hwnd,GWLP_WNDPROC,(LONG_PTR)ui_button_proc);if(!ui_original_button)ui_original_button=previous;}return hwnd;
}
static void ui_place(HWND hwnd,int x,int y,int w,int h){if(hwnd)MoveWindow(hwnd,x,y,w,h,TRUE);}
static void ui_layout(void){
 if(!ui_ready)return;RECT r;GetClientRect(app,&r);int height=r.bottom-142;if(height<150)height=150;
 ui_place(ui_buttons[0],14,40,118,40);ui_place(ui_buttons[1],140,40,108,40);ui_place(ui_buttons[2],r.right-98,3,60,24);ui_place(ui_buttons[3],r.right-34,0,32,30);ui_place(ui_body,14,88,r.right-28,height);
 int max=ui_scroll_max();if(ui_scroll[ui_tab]>max)ui_scroll[ui_tab]=max;int offset=14-ui_scroll[ui_tab],width=r.right-74;
 ui_place(status_label,44,offset+44,width-46,48);ui_place(ui_general_controls[0],194,offset+704,246,32);ui_place(ui_general_controls[1],194,offset+940,140,32);
 ui_place(ui_general_controls[2],16,offset+192,166,32);ui_place(ui_general_controls[3],194,offset+192,166,32);
 ui_place(ui_general_controls[4],194,offset+386,166,32);ui_place(ui_general_controls[5],194,offset+434,166,32);ui_place(ui_general_controls[6],16,offset+522,228,32);
 ui_place(key_field,194,offset+50,246,32);ui_place(ui_key_controls[0],194,offset+96,116,32);ui_place(ui_key_controls[1],322,offset+96,118,32);ui_place(note_label,16,offset+154,width,78);
 wchar_t binding[256];GetWindowTextW(key_label,binding,256);HDC dc=GetDC(app);SelectObject(dc,font);SIZE size;GetTextExtentPoint32W(dc,binding,(int)wcslen(binding),&size);ReleaseDC(app,dc);int key_width=size.cx+4;if(key_width>260)key_width=260;
 ui_place(key_label,14,r.bottom-36,key_width,26);ui_place(ui_buttons[4],24+key_width,r.bottom-39,88,32);ui_place(ui_buttons[5],122+key_width,r.bottom-39,164,32);ui_place(ui_buttons[6],r.right-94,r.bottom-39,80,32);
 InvalidateRect(ui_body,NULL,FALSE);InvalidateRect(app,NULL,FALSE);
}
static void ui_switch_tab(int tab,BOOL focus_key){
 if(!ui_ready)return;if(tab!=ui_tab&&GetFocus()==key_field)SetFocus(app);ui_tab=tab;
 ShowWindow(status_label,tab?SW_HIDE:SW_SHOW);for(int i=0;i<7;i++)ShowWindow(ui_general_controls[i],tab?SW_HIDE:SW_SHOW);
 ShowWindow(key_field,tab?SW_SHOW:SW_HIDE);ShowWindow(note_label,tab?SW_SHOW:SW_HIDE);for(int i=0;i<2;i++)ShowWindow(ui_key_controls[i],tab?SW_SHOW:SW_HIDE);
 ui_layout();InvalidateRect(ui_buttons[0],NULL,FALSE);InvalidateRect(ui_buttons[1],NULL,FALSE);if(focus_key){SetFocus(key_field);}
}
static void ui_set_alpha(void){SetLayeredWindowAttributes(app,0,(BYTE)(255*ui_opacity/100),LWA_ALPHA);}
static void ui_save_appearance(void){if(config_path[0]){wchar_t opacity[8];swprintf(opacity,8,L"%d",ui_opacity);WritePrivateProfileStringW(L"Appearance",L"Opacity",opacity,config_path);WritePrivateProfileStringW(L"Appearance",L"Language",ui_language?L"en":L"vi",config_path);WritePrivateProfileStringW(L"Appearance",L"Pinned",ui_pinned?L"1":L"0",config_path);}}
static void ui_apply_language(void){
 if(!ui_ready)return;SetWindowTextW(ui_buttons[0],ui_text(L"Tổng quan",L"Overview"));SetWindowTextW(ui_buttons[1],ui_text(L"Phím tắt",L"Keybinds"));SetWindowTextW(ui_buttons[2],ui_text(L"Ghim",L"Pin"));SetWindowTextW(ui_buttons[3],L"×");SetWindowTextW(ui_buttons[4],ui_text(L"Đổi phím",L"Change"));SetWindowTextW(ui_buttons[5],ui_language?L"English":L"Tiếng Việt");SetWindowTextW(ui_buttons[6],ui_text(L"Thoát",L"Exit"));SetWindowTextW(ui_general_controls[1],ui_text(L"Đổi phím",L"Change shortcut"));SetWindowTextW(ui_key_controls[0],ui_text(L"Lưu",L"Save"));SetWindowTextW(ui_key_controls[1],ui_text(L"Mặc định",L"Default"));
 SetWindowTextW(ui_general_controls[2],ui_text(L"Chọn EXE…",L"Choose EXE…"));SetWindowTextW(ui_general_controls[3],ui_text(L"Khôi phục / Mở game",L"Recover / Open game"));
 SetWindowTextW(ui_general_controls[4],ui_text(L"Tạm dừng",L"Suspended"));SetWindowTextW(ui_general_controls[5],ui_text(L"Tạm dừng",L"Suspended"));EnableWindow(ui_general_controls[4],FALSE);EnableWindow(ui_general_controls[5],FALSE);SetWindowTextW(ui_general_controls[6],ui_text(L"Hoàn tác gói thử",L"Remove trial package"));
 wchar_t opacity[12];swprintf(opacity,12,L"%d%%",ui_opacity);SetWindowTextW(ui_general_controls[0],opacity);SetWindowTextW(note_label,ui_text(L"Bấm ô phím và nhấn tổ hợp muốn dùng, sau đó bấm Lưu.",L"Click the shortcut field, press your key combination, then Save."));update_key_text();poll_game();ui_layout();
}
static void ui_popup(HWND button,BOOL language){
 HMENU menu=CreatePopupMenu();if(language){AppendMenuW(menu,MF_STRING|(ui_language?0:MF_CHECKED),UI_VI,L"Tiếng Việt");AppendMenuW(menu,MF_STRING|(ui_language?MF_CHECKED:0),UI_EN,L"English");}
 else {const int values[]={85,90,94,100};const int commands[]={UI_ALPHA_85,UI_ALPHA_90,UI_ALPHA_94,UI_ALPHA_100};for(int i=0;i<4;i++){wchar_t text[12];swprintf(text,12,L"%d%%",values[i]);AppendMenuW(menu,MF_STRING|(ui_opacity==values[i]?MF_CHECKED:0),commands[i],text);}}
 RECT r;GetWindowRect(button,&r);SetForegroundWindow(app);TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_TOPALIGN,r.left,r.bottom,0,app,NULL);DestroyMenu(menu);
}
static BOOL ui_command(int id){
 if(!ui_ready)return FALSE;
 if(id==UI_GENERAL||id==UI_KEYS){ui_switch_tab(id==UI_KEYS,FALSE);return TRUE;}
 if(id==UI_CHANGE_KEY){ui_switch_tab(1,TRUE);return TRUE;}
 if(id==UI_HIDE){ShowWindow(app,SW_HIDE);return TRUE;}
 if(id==UI_PIN){ui_pinned=!ui_pinned;SetWindowPos(app,ui_pinned?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);ui_save_appearance();InvalidateRect(ui_buttons[2],NULL,FALSE);InvalidateRect(ui_body,NULL,FALSE);return TRUE;}
 if(id==UI_LANGUAGE){ui_popup(ui_buttons[5],TRUE);return TRUE;}
 if(id==UI_OPACITY){ui_popup(ui_general_controls[0],FALSE);return TRUE;}
 if(id==UI_VI||id==UI_EN){ui_language=id==UI_EN;ui_save_appearance();ui_apply_language();return TRUE;}
 if(id>=UI_ALPHA_85&&id<=UI_ALPHA_100){const int values[]={85,90,94,100};ui_opacity=values[id-UI_ALPHA_85];ui_set_alpha();ui_save_appearance();ui_apply_language();return TRUE;}return FALSE;
}
static LRESULT CALLBACK ui_body_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:ui_paint_body(hwnd,NULL);return 0;
 case WM_PRINTCLIENT:ui_paint_body(hwnd,(HDC)wp);return 0;
 case WM_MOUSEWHEEL:ui_scroll_to(ui_scroll[ui_tab]-(short)HIWORD(wp)*44/WHEEL_DELTA);return 0;
 case WM_LBUTTONDOWN:{RECT track,thumb;ui_thumb(&track,&thumb);POINT p={(short)LOWORD(lp),(short)HIWORD(lp)};if(ui_scroll_max()>0&&PtInRect(&track,p)){if(PtInRect(&thumb,p)){ui_dragging=TRUE;ui_drag_start=p.y;ui_drag_offset=ui_scroll[ui_tab];SetCapture(hwnd);}else ui_scroll_to(ui_scroll[ui_tab]+(p.y<thumb.top?-150:150));InvalidateRect(hwnd,NULL,FALSE);}return 0;}
 case WM_MOUSEMOVE:if(ui_dragging){RECT track,thumb;ui_thumb(&track,&thumb);int travel=(track.bottom-track.top)-(thumb.bottom-thumb.top);if(travel>0)ui_scroll_to(ui_drag_offset+((short)HIWORD(lp)-ui_drag_start)*ui_scroll_max()/travel);}return 0;
 case WM_LBUTTONUP:if(ui_dragging){ui_dragging=FALSE;ReleaseCapture();InvalidateRect(hwnd,NULL,FALSE);}return 0;
 case WM_CAPTURECHANGED:ui_dragging=FALSE;return 0;
 case WM_COMMAND:case WM_DRAWITEM:case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:return SendMessageW(app,msg,wp,lp);
 }
 return DefWindowProcW(hwnd,msg,wp,lp);
}
static void ui_create(void){
 WNDCLASSW klass={0};klass.hInstance=instance;klass.lpfnWndProc=ui_body_proc;klass.lpszClassName=L"WuWaPCContent";klass.hCursor=LoadCursorW(NULL,IDC_ARROW);RegisterClassW(&klass);
 ui_status_brush=CreateSolidBrush(RGB(22,26,32));ui_field_brush=CreateSolidBrush(RGB(29,47,69));ui_small_font=CreateFontW(-13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 ui_body=CreateWindowExW(WS_EX_CONTROLPARENT,klass.lpszClassName,L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,14,88,682,616,app,NULL,instance,NULL);
 const int ids[]={UI_GENERAL,UI_KEYS,UI_PIN,UI_HIDE,UI_CHANGE_KEY,UI_LANGUAGE,EXIT_APP};for(int i=0;i<7;i++)ui_buttons[i]=ui_child(app,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,ids[i]);
 status_label=ui_child(ui_body,L"STATIC",L"",0,0);ui_general_controls[0]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,UI_OPACITY);ui_general_controls[1]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,UI_CHANGE_KEY);
 ui_general_controls[2]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,UI_BROWSE_GAME);ui_general_controls[3]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,UI_LAUNCH_GAME);
 ui_general_controls[4]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,UI_TRIAL_CD);ui_general_controls[5]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,UI_TRIAL_DAMAGE);ui_general_controls[6]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,UI_TRIAL_REMOVE);
 key_field=ui_child(ui_body,HOTKEY_CLASSW,L"",WS_TABSTOP,0);SendMessageW(key_field,HKM_SETRULES,0,0);SendMessageW(key_field,HKM_SETHOTKEY,key_word(menu_key),0);old_key_proc=(WNDPROC)SetWindowLongPtrW(key_field,GWLP_WNDPROC,(LONG_PTR)key_proc);
 ui_key_controls[0]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,SAVE_KEY);ui_key_controls[1]=ui_child(ui_body,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP,DEFAULT_KEY);note_label=ui_child(ui_body,L"STATIC",L"",0,0);key_label=ui_child(app,L"STATIC",L"",0,0);
 wchar_t language[8];GetPrivateProfileStringW(L"Appearance",L"Language",L"vi",language,8,config_path);ui_language=!wcscmp(language,L"en");ui_opacity=GetPrivateProfileIntW(L"Appearance",L"Opacity",94,config_path);if(ui_opacity<85||ui_opacity>100)ui_opacity=94;ui_pinned=GetPrivateProfileIntW(L"Appearance",L"Pinned",1,config_path)!=0;
 ui_ready=TRUE;ui_set_alpha();SetWindowPos(app,ui_pinned?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);ui_apply_language();ui_switch_tab(0,FALSE);
}
#endif

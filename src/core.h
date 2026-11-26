/* SPDX-License-Identifier: MIT */
#ifndef WUWA_CORE_H
#define WUWA_CORE_H
#include <stddef.h>
#include <stdio.h>
struct WRect {long left,top,right,bottom;};
struct WPoint {long x,y;};
struct WKey {unsigned vk,mods;};
/* Same bit values as Win32 MOD_ALT, MOD_CONTROL, MOD_SHIFT. */
static int key_valid(struct WKey key){
 if(!key.vk||key.vk<8||key.vk>254||(key.mods&~7u))return 0;
 return key.vk!=16&&key.vk!=17&&key.vk!=18&&key.vk!=91&&key.vk!=92;
}
static struct WPoint centered(struct WRect area,long width,long height){
 struct WPoint point={area.left+(area.right-area.left-width)/2,area.top+(area.bottom-area.top-height)/2};return point;
}
static int ascii_equal(const char *a,const char *b){
 while(*a&&*b){unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}return *a==*b;
}
static int known_game_image(const char *name){return ascii_equal(name,"Client-Win64-Shipping.exe");}
struct WKeyState {struct WKey key;int registered;};
struct WKeyOps {void *context;int (*reserve)(void *,struct WKey);void (*release)(void *);int (*persist)(void *,struct WKey);};
enum WKeyResult {KEY_SAVED,KEY_INVALID,KEY_BUSY,KEY_SAVE_FAILED};
static enum WKeyResult key_apply(struct WKeyState *state,struct WKey next,struct WKeyOps ops){
 if(!key_valid(next))return KEY_INVALID;
 if(state->registered)ops.release(ops.context);
 state->registered=0;
 if(!ops.reserve(ops.context,next)){state->registered=ops.reserve(ops.context,state->key);return KEY_BUSY;}
 if(!ops.persist(ops.context,next)){ops.release(ops.context);state->registered=ops.reserve(ops.context,state->key);return KEY_SAVE_FAILED;}
 state->key=next;state->registered=1;return KEY_SAVED;
}
#endif

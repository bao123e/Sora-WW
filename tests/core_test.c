#include "../src/core.h"
#include <assert.h>
struct MockKeys {unsigned active,busy;int disk_failure,reserves,releases,writes;struct WKey stored;};
static int reserve(void *ctx,struct WKey key){struct MockKeys *m=ctx;m->reserves++;if(m->active||m->busy==key.vk)return 0;m->active=key.vk;return 1;}
static void release(void *ctx){struct MockKeys *m=ctx;m->releases++;m->active=0;}
static int persist(void *ctx,struct WKey key){struct MockKeys *m=ctx;m->writes++;if(m->disk_failure)return 0;m->stored=key;return 1;}
static void check_key_changes(void){
 struct MockKeys mock={.active=112,.busy=113};struct WKeyState state={{112,0},1};struct WKeyOps ops={&mock,reserve,release,persist};
 assert(key_apply(&state,(struct WKey){0,0},ops)==KEY_INVALID);assert(mock.reserves==0&&mock.releases==0&&mock.writes==0);
 assert(key_apply(&state,(struct WKey){113,0},ops)==KEY_BUSY);assert(state.key.vk==112&&state.registered&&mock.active==112&&mock.writes==0);
 mock.disk_failure=1;assert(key_apply(&state,(struct WKey){114,0},ops)==KEY_SAVE_FAILED);assert(state.key.vk==112&&state.registered&&mock.active==112);
 mock.disk_failure=0;assert(key_apply(&state,(struct WKey){114,6},ops)==KEY_SAVED);assert(state.key.vk==114&&state.key.mods==6&&state.registered&&mock.active==114&&mock.stored.vk==114);
}
int main(void){
 assert(key_valid((struct WKey){112,0})); /* F1 */
 assert(key_valid((struct WKey){65,6})); /* Ctrl+Shift+A */
 assert(!key_valid((struct WKey){0,0}));assert(!key_valid((struct WKey){16,0}));
 assert(!key_valid((struct WKey){65,8}));assert(!key_valid((struct WKey){91,0}));
 struct WPoint p=centered((struct WRect){0,0,1600,900},700,440);assert(p.x==450&&p.y==230);
 p=centered((struct WRect){-1920,40,0,1120},700,440);assert(p.x==-1310&&p.y==360);
 p=centered((struct WRect){300,200,1900,1100},700,440);assert(p.x==750&&p.y==430);
 assert(known_game_image("Client-Win64-Shipping.exe"));assert(known_game_image("CLIENT-WIN64-SHIPPING.EXE"));
 assert(!known_game_image("launcher.exe"));assert(!known_game_image("Client-Win64-Shipping.exe.bak"));
 check_key_changes();
 puts("PASS key-change conflict/save rollback; hotkey validation, multi-monitor/client centering, exact executable-name filtering");
 return 0;
}
